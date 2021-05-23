/**
 * Project Message Passing
 */


#ifndef _MESSAGESOURCE_H
#define _MESSAGESOURCE_H

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <queue>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <zmq.h>
#include <boost/thread.hpp>
#include <librdkafka/rdkafkacpp.h>

#include "log/log.h"
#include "utils/utils.h"
#include "message/request_message.pb.h"
#include "queue/readerwriterqueue.h"
#include "message/data_message.h"

namespace message_pass {

template<typename T>
class MessageSource {
        static_assert(std::is_base_of<IDataMessage, T>::value, "template parameter is not derived from IDataMessage");
    public:
        /**
         * @param topic_server
         * @param topics
         * @param send_batch
         * @param io_threads
         */
        MessageSource(const std::string& topic_server, const std::vector<std::string>& topics, int io_threads);

        ~MessageSource();

        /**
         * @brief Set the fixed recv object
         *
         * @param batch recv batch size
         * @param size recv every message size
         */
        void set_fixed_send(std::size_t batch, std::size_t size);

        /**
         * @brief 将消息放入队列
         *
         * @param topic 目标topic
         * @param msg 消息
         */
        void send(const std::string& topic, T* msg);

        /**
         * @brief 设置MessageSource id，如果不设置则默认使用本机ip
         *
         * @param id
         */
        void set_identity(const std::string& id);

        void start();

        void stop();

    private:

        template <typename T2>
        using brwQueue = moodycamel::BlockingReaderWriterQueue<T2>;

        /**
         * message ready to be sent
         */
        std::map<std::string, brwQueue<T*>*> readys_;

        std::map<std::string, brwQueue<RequestMessage*>*> request_queues_;

        std::string topic_server_;
        std::vector<std::string> topics_;

        void* zmq_ctx_;
        int io_threads_ = 1;
        std::atomic_bool running_{false};
        boost::thread_group recv_request_threads_;
        RdKafka::Conf* kafka_conf_;
        /**
         * @brief MessageSource id 由字符串hash得到(默认本机ip)
         */
        std::size_t identity_;

        bool send_fixed_;
        std::size_t fixed_size_;
        std::size_t fixed_num_;

        /**
         * @brief spdlog logger
         */
        std::shared_ptr<spdlog::logger> logger_;

        /**
         * @brief every topic has a task thread
         */
        boost::thread_group task_threads_;

    private:
        void init(const std::vector<std::string>& topics);

        void send_msg(void* socket, T*) const;
        void store_msg(std::map<std::string, std::map<std::size_t, T*>>& sents, RequestMessage* req, T* msg);
        void delete_msg(std::map<std::string, std::map<std::size_t, T*>>& sents, RequestMessage* req);
        void recover_msg(const std::map<std::string, void *>& sockets, 
                         const std::map<std::string, std::map<std::size_t, T*>>& sents, 
                         RequestMessage* req) const;

        void recv_request(const std::string& topic);

        /**
         * @brief 负责处理get、delete、和recover事件
         *        
         * @param topic topic
         */
        void task_thread(const std::string& topic);

        void delete_recover(const std::string& topic);

        class KafkaEventCb : public RdKafka::EventCb {
            private:
                std::shared_ptr<spdlog::logger> logger_;
            public:
                KafkaEventCb(std::shared_ptr<spdlog::logger> logger): logger_(logger) {}
                void event_cb (RdKafka::Event &event) {
                    switch (event.type()) {
                    case RdKafka::Event::EVENT_ERROR: {
                        logger_->error(RdKafka::err2str(event.err()));
                        break;
                    }
                    case RdKafka::Event::EVENT_STATS: {
                        logger_->info(event.str());
                        break;
                    }
                    case RdKafka::Event::EVENT_LOG: {
                        logger_->info(event.str());
                        break;
                    }
                    case RdKafka::Event::EVENT_THROTTLE: {
                        logger_->error("THROTTLED");
                        break;
                    }
                    default:
                        logger_->info(event.str());
                    }
                }
        };

        KafkaEventCb kafka_event_cb_;
};

/**
 * @param topic_server
 * @param topics
 * @param io_threads
 */
//template<typename M, typename = IS_DERIVED_FROM_IDATAMESSAGE<M>>
template<typename T>
MessageSource<T>::MessageSource(const std::string& topic_server,
                                const std::vector<std::string>& topics,
                                int io_threads)
    : topic_server_(topic_server), topics_(topics),
      io_threads_(io_threads),
      identity_(std::hash<std::string>()(Utils::get_host_ip())),
      send_fixed_(false), fixed_size_(0), fixed_num_(0),
      logger_(MessageLogger::get_logger("MessageSource")),
      kafka_event_cb_(this->logger_)
{
    init(topics_);
}

template<typename T>
void MessageSource<T>::init(const std::vector<std::string>& topics) {
    //initialize queues
    for(std::string topic : topics) {
        readys_[topic] = new brwQueue<T*>();

        request_queues_[topic] = new brwQueue<RequestMessage*>();
    }

    //initialize zmq
    zmq_ctx_ = zmq_ctx_new();
    zmq_ctx_set(zmq_ctx_, ZMQ_IO_THREADS, io_threads_);

    //initialize kafka global conf
    std::string errstr;
    kafka_conf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if(kafka_conf_->set("enable.partition.eof", "true", errstr) != RdKafka::Conf::CONF_OK) {
        logger_->error(errstr);
        exit(1);
    }
    //callback
    if(kafka_conf_->set("event_cb", &this->kafka_event_cb_, errstr) != RdKafka::Conf::CONF_OK) {
        logger_->error(errstr);
        exit(1);
    }
    //group.id=hostname+pid
    std::size_t len = 20;
    char* name = new char[len];
    gethostname(name, len);
    pid_t pid = getpid();
    auto group_id = std::string(name) + "-" + std::to_string(pid);
    logger_->info("group id = " + group_id);
    if(kafka_conf_->set("group.id",  name, errstr) != RdKafka::Conf::CONF_OK) {
        logger_->error(errstr);
        exit(1);
    }
    delete[] name;
    //metrics
    // if(kafka_conf_->set("statistics.interval.ms", "5000", errstr) != RdKafka::Conf::CONF_OK) {
    //     logger_->error(errstr);
    //     exit(1);
    // }
    if(kafka_conf_->set("metadata.broker.list", topic_server_, errstr)) {
        logger_->error(errstr);
        exit(1);
    }
}

template<typename T>
MessageSource<T>::~MessageSource() {
    //destroy zmq
    SPDLOG_DEBUG("destroying zmq ...");
    zmq_ctx_destroy(zmq_ctx_);
    SPDLOG_DEBUG("destroy zmq");

    //destroy queues
    for(auto queue : readys_) {
        T* msg;
        bool ret;
        
        while (true) {
            ret = queue.second->try_dequeue(msg);
            if(ret) {
                delete msg;
            } else {
                break;
            }
        }
        
        delete queue.second;
    }

    for(auto queue : request_queues_) {
        RequestMessage* req;
        bool ret;

        while (true) {
            ret = queue.second->try_dequeue(req);
            if(ret) {
                delete req;
            } else {
                break;
            }
        }

        delete queue.second;
    }

    // destroy kafka
    delete kafka_conf_;
}

/**
 * @brief Set the fixed recv object
 *
 * @param batch recv batch size
 * @param size recv every message size
 */
template<typename T>
void MessageSource<T>::set_fixed_send(std::size_t batch, std::size_t size) {
    this->send_fixed_ = true;
    this->fixed_num_ = batch;
    this->fixed_size_ = size;
}

/**
 * @param topic
 * @param msg
 */
template<typename T>
void MessageSource<T>::send(const std::string& topic, T* msg) {
    readys_[topic]->enqueue(msg);
}

template<typename T>
void MessageSource<T>::start() {
    running_ = true;
    //every topic has a thread
    //start threads to recv requst
    logger_->info("start thread group to recv requst");
    for(auto& topic : topics_) {
        recv_request_threads_.create_thread(std::bind(&MessageSource::recv_request, this, topic));
    }

    //start threads to send msg
    logger_->info("start thread group to send msg");
    for(auto& topic : topics_) {
        task_threads_.create_thread(std::bind(&MessageSource::task_thread, this, topic));
    }

    logger_->info("start thread group compelte");
}

template<typename T>
void MessageSource<T>::stop() {
    running_ = false;
    logger_->info("stop thread group to recv requst");
    recv_request_threads_.join_all();
    logger_->info("stop thread group for task");
    task_threads_.join_all();

    logger_->info("stop thread group compelte");
}

template<typename T>
void MessageSource<T>::recv_request(const std::string& topic) {
    logger_->info("start recv request thread for topic " + topic);
    auto request_queue = request_queues_[topic];
    std::string errstr;
    RdKafka::KafkaConsumer* consumer = RdKafka::KafkaConsumer::create(kafka_conf_, errstr);
    if (!consumer) {
        logger_->error(errstr);
        exit(-1);
    }
    consumer->subscribe({topic});
    while(running_) {
        RdKafka::Message* msg = consumer->consume(500);
        if(msg == NULL) {
            continue;
        }
        consumer->commitAsync();

        switch (msg->err()) {
            case RdKafka::ERR__TIMED_OUT: {
                SPDLOG_DEBUG("no request in topic server");
                break;
            }
            case RdKafka::ERR_NO_ERROR: {
                RequestMessage* remote_req = new RequestMessage();
                if(!remote_req->ParseFromString(static_cast<char*>(msg->payload()))) {
                    logger_->error("parse protobuf from a string failed");
                    continue;
                }
                //insert request in request_queue
                request_queue->enqueue(remote_req);
                SPDLOG_DEBUG("insert request in request_queue for topic: {}", topic);
                break;
            }
            case RdKafka::ERR__UNKNOWN_TOPIC: {
                logger_->error("no such topic");
                break;
            }
            case RdKafka::ERR__UNKNOWN_PARTITION: {
                logger_->error("no such partition");
                break;
            }
            default: {
                logger_->warn(msg->errstr());
                break;
            }
        }

        delete msg;
    }
    consumer->close();
    delete consumer;
    logger_->info("stop recv request thread for topic " + topic);
}

/**
 * @brief 负责处理get、delete、和recover事件
 * 
 * @param topic topic
 */
template<typename T>
void MessageSource<T>::task_thread(const std::string& topic) {
    logger_->info("start task thread for topic: {}", topic);

    auto ready = readys_[topic];

    /**
     * @brief zmq_sockets <sink, zmq_socket>
     */
    thread_local std::map<std::string, void *> thread_local_sockets;
    /**
     * @brief 已经发送的数据 <sink, set>
     */
    thread_local std::map<std::string, std::map<std::size_t, T*>> thread_local_sents_;

    auto request_queue = request_queues_[topic];

    while(this->running_) {
        RequestMessage* remote_req;
        //use timed to prevent hanging
        if(!request_queue->wait_dequeue_timed(remote_req, std::chrono::milliseconds(50))) {
            continue;
        }

        switch(remote_req->cmd()) {
            case RequestMessage_CMD_GET : {
                //check zmq socket
                auto it = thread_local_sockets.find(remote_req->sink());
                void* socket;
                if(it == thread_local_sockets.end()) {
                    //if no socket for this sink
                    socket = zmq_socket(zmq_ctx_, ZMQ_DEALER);
                    //设置ZMQ_ROUTING_ID
                    zmq_setsockopt(socket, ZMQ_ROUTING_ID, &this->identity_, sizeof(this->identity_));
                    //设置ZMQ_LINGER，默认要等待所有消息发送后才能关闭zmq socket
                    int linger = 500;
                    zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger));
                    thread_local_sockets[remote_req->sink()] = socket;
                    thread_local_sents_[remote_req->sink()] = std::map<std::size_t, T*>();
                    int rc = zmq_connect(socket, remote_req->sink().c_str());
                    assert(rc == 0);

                    logger_->info("initialized a zmq socket for sink: " + remote_req->sink());
                } else {
                    socket = it->second;
                }

                //send message with zmq to req address
                T* msg;
                while(!ready->wait_dequeue_timed(msg, std::chrono::milliseconds(50)) && running_) {
                    continue;
                }
                send_msg(socket, msg);
                store_msg(thread_local_sents_, remote_req, msg);
                break;
            }
            case RequestMessage_CMD_DEL : {
                delete_msg(thread_local_sents_, remote_req);
                break;
            }
            case RequestMessage_CMD_RECOVER : {
                recover_msg(thread_local_sockets, thread_local_sents_, remote_req);
                break;
            }
            default:
                break;
        }

        delete remote_req;
    }

    //destroy zmq sockets
    for(auto it = thread_local_sockets.begin(); it != thread_local_sockets.end(); ++it) {
        zmq_close(it->second);
    }

    for(auto msgs : thread_local_sents_) {
        for(auto msg_pair : msgs.second) {
            delete msg_pair.second;
        }
    }

    logger_->info("stop task thread for topic: {}", topic);
}

template<typename T>
void MessageSource<T>::send_msg(void* socket, T* msg) const {
    if(this->send_fixed_) {
        zmq_send(socket, msg->const_data(), this->fixed_size_, ZMQ_DONTWAIT);
        return;
    }
    std::size_t size = msg->size();
    zmq_send(socket, &size, 8, ZMQ_SNDMORE);
    zmq_send(socket, msg->const_data(), size, ZMQ_DONTWAIT);
}

template<typename T>
void MessageSource<T>::store_msg(std::map<std::string, std::map<std::size_t, T*>>& sents, RequestMessage* req, T* msg) {
    sents[req->sink()][msg->key()] = msg;
}

template<typename T>
void MessageSource<T>::delete_msg(std::map<std::string, std::map<std::size_t, T*>>& sents, RequestMessage* req) {
    auto it = sents[req->sink()].find(req->key());
    if(it != sents[req->sink()].end()) {
        delete it->second;
        sents[req->sink()].erase(it);
    }
}

template<typename T>
void MessageSource<T>::recover_msg(const std::map<std::string, void*>& sockets, 
                                   const std::map<std::string, std::map<std::size_t, T*>>& sents, 
                                   RequestMessage* req) const
{
    auto it = sockets.find(req->sink());
    if(it == sockets.end()) {
        logger_->info("no message to be recovered");
        return;
    }
    
    void* socket = it->second;

    auto& sent_it = sents.find(req->sink())->second;
    if(sent_it.empty()) {
        logger_->info("no message to be recovered");
        return;
    }

    for(auto msg : sent_it) {
        send_msg(socket, msg.second);
    }
}

/**
 * @brief 设置MessageSource id，如果不设置则默认使用本机ip
 *
 * @param id
 */
template<typename T>
void MessageSource<T>::set_identity(const std::string& id) {
    this->identity_ = std::hash<std::string>()(id);
}

}

#endif /* _MESSAGESOURCE_H */
