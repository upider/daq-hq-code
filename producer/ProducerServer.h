/**
 * Project Message Passing
 */


#ifndef _PRODUCERSERVER_H
#define _PRODUCERSERVER_H

#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include <zmq.h>
#include <boost/thread.hpp>
#include <librdkafka/rdkafkacpp.h>

#include "log/log.h"
#include "utils/utils.h"
#include "message/RequestMessage.pb.h"
#include "queue/readerwriterqueue.h"
#include "message/IDataMessage.h"

namespace message_pass {

static int partition_cnt = 0;
static int eof_cnt = 0;

template<typename T>
class ProducerServer {
    static_assert(std::is_base_of<IDataMessage, T>::value, "template parameter is not derived from IDataMessage");
    public:
        /**
         * @param topic_server
         * @param topics
         * @param send_batch
         * @param io_threads
         */
        ProducerServer(const std::string& topic_server, const std::vector<std::string>& topics, std::size_t io_threads);

        ~ProducerServer();

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
         * @brief 设置producer id，如果不设置则默认使用本机ip，所以最大长度为15
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

        /**
         * msg has been sent<topic,<sink,message>>
         */
        std::map<std::string, std::map<std::string, std::list<T*>*>> sents_;

        std::map<std::string, brwQueue<RequestMessage>*> gets_;
        std::map<std::string, brwQueue<RequestMessage>*> del_recovers_;

        std::string topic_server_;
        std::vector<std::string> topics_;
        void* zmq_ctx_;
        size_t io_threads_ = 1;
        std::atomic_bool running_{false};
        boost::thread_group recv_request_threads_;
        boost::thread_group send_msg_threads_;
        boost::thread_group del_recover_msg_threads_;
        RdKafka::Conf* kafka_conf_;
        std::size_t identity_;
        bool send_fixed_{false};
        std::size_t fixed_size_;
        std::size_t fixed_num_;

        std::shared_ptr<spdlog::logger> logger_;

    private:
        void init(const std::vector<std::string>& topics);

        /**
         * @param sink
         * @param msg
         */
        void do_send(void* socket, T* msg);

        void send_msg(const std::string& topic);

        void delete_recover(const std::string& topic);

        void recv_request(const std::string& topic);
        
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
                        logger_->error(event.str());
                        break;
                    }
                    case RdKafka::Event::EVENT_LOG: {
                        logger_->error(event.str());
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
ProducerServer<T>::ProducerServer(const std::string& topic_server, 
                                const std::vector<std::string>& topics, 
                                std::size_t io_threads)
    : topic_server_(topic_server), topics_(topics), 
    io_threads_(io_threads), 
    identity_(std::hash<std::string>()(Utils::get_host_ip())), 
    logger_(MessageLogger::get_logger("ProducerServer")),
    kafka_event_cb_(this->logger_)
{
    init(topics_);
}

template<typename T>
void ProducerServer<T>::init(const std::vector<std::string>& topics) {
    //initialize queues
    for(std::string topic : topics) {
        readys_[topic] = new brwQueue<T*>();

        std::map<std::string, std::list<T*>*> msgs;
        sents_[topic] =  msgs;

        gets_[topic] = new brwQueue<RequestMessage>();
        del_recovers_[topic] = new brwQueue<RequestMessage>();
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
ProducerServer<T>::~ProducerServer() {
    //destroy zmq
    zmq_ctx_destroy(zmq_ctx_);
    //destroy queues
    //TODO: delete elements in queue
    for(auto queue : readys_) {
        delete queue.second;
    }
    for(auto queue : gets_) {
        delete queue.second;
    }
    for(auto queue : del_recovers_) {
        delete queue.second;
    }
    for(auto queues : sents_) {
        for(auto queue : queues.second) {
            delete queue.second;
        }
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
void ProducerServer<T>::set_fixed_send(std::size_t batch, std::size_t size) {
    this->send_fixed_ = true;
    this->fixed_num_ = batch;
    this->fixed_size_ = size;
}

/**
 * @param topic
 * @param msg
 */
template<typename T>
void ProducerServer<T>::send(const std::string& topic, T* msg) {
    readys_[topic]->enqueue(msg);
}

template<typename T>
void ProducerServer<T>::start() {
    running_ = true;
    //every topic has a thread
    //start threads to recv requst
    logger_->info("start thread group to recv requst");
    for(auto& topic : topics_) {
        recv_request_threads_.create_thread(std::bind(&ProducerServer::recv_request, this, topic));
    }

    //start threads to send msg
    logger_->info("start thread group to send msg");
    for(auto& topic : topics_) {
        send_msg_threads_.create_thread(std::bind(&ProducerServer::send_msg, this, topic));
    }

    //start threads to delete or recover msg
    // logger_->info("start thread group to delete or recover msg")
    // for(auto& topic : topics_) {
    //     del_recover_msg_threads_.create_thread(std::bind(&ProducerServer::delete_recover, this, topic));
    // }

    logger_->info("start thread group compelte");
}

template<typename T>
void ProducerServer<T>::stop() {
    running_ = false;
    logger_->info("stop thread group to recv requst");
    recv_request_threads_.join_all();
    logger_->info("stop thread group to send msg");
    send_msg_threads_.join_all();
    // logger_->info("start thread group to delete or recover msg")
    // del_recover_msg_threads_.join_all();
    logger_->info("stop thread group compelte");
}

template<typename T>
void ProducerServer<T>::delete_recover(const std::string& topic) {
    logger_->info("start deleting or recovering thread for topic " + topic);
    auto del_recover = del_recovers_[topic];
    auto sent = sents_[topic];
    RequestMessage rmsg;
    while(running_) {
        if(!del_recover->wait_dequeue_timed(rmsg, std::chrono::milliseconds(50))) {
            continue;
        }
        logger_->info("get a del or recover request in queue");
        auto sents_sink = sent[rmsg.sink()];
        if(rmsg.cmd() == RequestMessage::CMD::RequestMessage_CMD_DEL){
            //delete msg
            auto it = sents_sink->begin();
            for(;it != sents_sink->end(); ++it) {
                if((*it)->get_key() == rmsg.key()) {
                    delete *it;
                    sents_sink->erase(it);
                    break;
                }
            }
        } else {
            //recover msg
            if(sents_sink->empty()) {
                continue;
            }
            void* socket = zmq_socket(zmq_ctx_, ZMQ_DEALER);
            zmq_connect(socket, rmsg.sink().c_str());
            for(auto it = sents_sink->begin(); it != sents_sink->end(); ++it) {
                do_send(socket, *it);
            }
        }
    }
    logger_->info("stop delete or recover thread for topic " + topic);
}

template<typename T>
void ProducerServer<T>::recv_request(const std::string& topic) {
    logger_->info("start recv request thread for topic " + topic);
    auto get = gets_[topic];
    auto del_recover = del_recovers_[topic];
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
                // logger_->info("consume time out");
                break;
            }
            case RdKafka::ERR_NO_ERROR: {
                RequestMessage rmsg;
                if(!rmsg.ParseFromString(static_cast<char*>(msg->payload()))) {
                    logger_->error("parse protobuf from a string failed");
                    continue;
                }
                if(rmsg.cmd() == RequestMessage_CMD_GET) {
                    if(!get->enqueue(rmsg)) {
                        logger_->error("get request enqueue failed");
                    }
                } else {
                    if(!del_recover->enqueue(rmsg)) {
                        logger_->error("recover request enqueue failed");
                    }
                }
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

template<typename T>
void ProducerServer<T>::send_msg(const std::string& topic) {
    logger_->info("start sending message for topic " + topic);
    thread_local std::map<std::__cxx11::string, void *> thread_local_sockets;
    brwQueue<RequestMessage>* gets = gets_[topic];
    auto ready = readys_[topic];
    auto sent = sents_[topic];
    //get request for this topic
    RequestMessage req;
    while(running_) {
        //use timed to prevent hanging
        if(!gets->wait_dequeue_timed(req, std::chrono::milliseconds(50))) {
            continue;
        }
        //send message with zmq to req address
        T* msg = new T();
        while(!ready->wait_dequeue_timed(msg, std::chrono::milliseconds(50)) && running_){
            continue;
        }
        auto it = thread_local_sockets.find(req.sink());
        void* socket;
        if(it == thread_local_sockets.end()) {
            socket = zmq_socket (zmq_ctx_, ZMQ_DEALER);
            zmq_setsockopt(socket, ZMQ_ROUTING_ID, &this->identity_, sizeof(this->identity_));
            thread_local_sockets[req.sink()] = socket;
            sent[req.sink()] = new std::list<T*>();
            int rc = zmq_connect(socket, req.sink().c_str());
            assert(rc == 0);

            logger_->info("initialized a zmq socket for sink: " + req.sink());
        } else {
            socket = it->second;
        }
        do_send(socket, msg);
        sent[req.sink()]->push_back(msg);
    }
    for(auto pair_socket : thread_local_sockets) {
        zmq_close(pair_socket.second);
    }
    logger_->info("stop send message thread for topic " + topic);
}

/**
 * @param socket
 * @param msg
 */
template<typename T>
void ProducerServer<T>::do_send(void* socket, T* msg) {
    if(this->send_fixed_) {
        zmq_send_const(socket, msg->const_data(), this->fixed_size_, ZMQ_DONTWAIT);
        return;
    }
    std::size_t size = msg->size();
    zmq_send(socket, &size, 8, ZMQ_SNDMORE);
    zmq_send(socket, msg->const_data(), size, ZMQ_DONTWAIT);
}

/**
 * @brief 设置producer id，如果不设置则默认使用本机ip，所以最大长度为15
 * 
 * @param id 
 */
template<typename T>
void ProducerServer<T>::set_identity(const std::string& id) {
    this->identity_ = std::hash<std::string>()(id);
}

}

#endif //_PRODUCERSERVER_H
