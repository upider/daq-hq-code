/**
 * Project Message Passing
 */


#ifndef _PRODUCERSERVER_H
#define _PRODUCERSERVER_H

#include <type_traits>
#include <string>
#include <map>
#include <vector>
#include <chrono>
#include <sys/types.h>
#include <unistd.h>

#include <zmq.h>
#include <boost/thread.hpp>
#include <librdkafka/rdkafkacpp.h>

#include "log/log.h"
#include "message/RequestMessage.h"
#include "message/IDataMessage.h"
#include "queue/readerwriterqueue.h"

namespace message_pass {

class IDataMessage;

static int partition_cnt = 0;
static int eof_cnt = 0;

template<typename T>
class ProducerServer {
        static_assert(std::is_base_of<IDataMessage, T>::value, "T is not derived from IDataMessage");
    public:
        /**
         * @param topic_server
         * @param topics
         * @param send_batch
         * @param io_threads
         */
        ProducerServer(const std::string& topic_server, const std::vector<std::string>& topics, size_t io_threads);

        ~ProducerServer();

        /**
         * @param topic
         * @param msg
         */
        void send(const std::string& topic, T* msg);

        void start();

        void stop();

    private:

        template <typename T2>
        using brwQueue = moodycamel::BlockingReaderWriterQueue<T2>;

        /**
         * message ready to be sent
         */
        std::map<std::string, brwQueue<T*>*> ready_;

        /**
         * msg has been sent<topic,<sink,message>>
         */
        std::map<std::string, std::map<std::string, std::set<T*>>> sent_;

        std::map<std::string, brwQueue<RequestMessage>*> gets_;
        std::map<std::string, brwQueue<RequestMessage>*> del_recovers_;
        std::map<std::string, std::map<std::string, void*>> sockets_;
        std::string topic_server_;
        std::vector<std::string> topics_;
        void* zmq_ctx_;
        size_t io_threads_ = 1;
        std::atomic_bool running_{false};
        boost::thread_group recv_request_threads_;
        boost::thread_group send_msg_threads_;
        boost::thread_group del_recover_msg_threads_;

        RdKafka::Conf *conf_;

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
            public:
                void event_cb (RdKafka::Event &event) {
                    switch (event.type()) {
                    case RdKafka::Event::EVENT_ERROR: {
                        LOG_ERROR(RdKafka::err2str(event.err()));
                        break;
                    }
                    case RdKafka::Event::EVENT_STATS: {
                        LOG_ERROR(event.str());
                        break;
                    }
                    case RdKafka::Event::EVENT_LOG: {
                        LOG_INFO(event.str());
                        break;
                    }
                    case RdKafka::Event::EVENT_THROTTLE: {
                        LOG_ERROR("THROTTLED");
                        break;
                    }
                    default:
                        LOG_INFO(event.str());
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
ProducerServer<T>::ProducerServer(const std::string& topic_server, const std::vector<std::string>& topics, size_t io_threads): topic_server_(topic_server), topics_(topics), io_threads_(io_threads) {
    init(topics_);
}

template<typename T>
void ProducerServer<T>::init(const std::vector<std::string>& topics) {
    //initialize queues
    for(std::string topic : topics) {
        ready_[topic] = new brwQueue<T*>();

        std::map<std::string, std::set<T*>> msgs;
        sent_[topic] =  msgs;

        gets_[topic] = new brwQueue<RequestMessage>();
        del_recovers_[topic] = new brwQueue<RequestMessage>();
    }
    //initialize zmq
    zmq_ctx_ = zmq_ctx_new();
    zmq_ctx_set(zmq_ctx_, ZMQ_IO_THREADS, io_threads_);
    //initialize kafka global conf
    std::string errstr;
    conf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if(conf_->set("enable.partition.eof", "true", errstr) != RdKafka::Conf::CONF_OK) {
        LOG_ERROR(errstr);
        exit(1);
    }
    //group.id=hostname+pid
    std::size_t len = 20;
    char* name = new char[len];
    gethostname(name, len);
    pid_t pid = getpid();
    auto group_id = std::string(name) + "-" + std::to_string(pid);
    LOG_INFO("group id = " + group_id);
    if (conf_->set("group.id",  name, errstr) != RdKafka::Conf::CONF_OK) {
        LOG_ERROR(errstr);
        exit(1);
    }
    delete[] name;
    if (conf_->set("statistics.interval.ms", "50", errstr) != RdKafka::Conf::CONF_OK) {
        LOG_ERROR(errstr);
        exit(1);
    }
    if(conf_->set("metadata.broker.list", topic_server_, errstr)) {
        LOG_ERROR(errstr);
        exit(1);
    }
}

template<typename T>
ProducerServer<T>::~ProducerServer() {
    //destroy zmq
    for(auto sockets : sockets_) {
        for(auto socket : sockets.second) {
            zmq_close(socket.second);
        }
    }
    zmq_ctx_destroy(zmq_ctx_);

    //destroy queues
    //TODO: delete elements in queue
    for(auto queue : ready_) {
        delete queue.second;
    }

    // destroy kafka
    delete conf_;
}

template<typename T>
void ProducerServer<T>::start() {
    running_ = true;
    //every topic has a thread
    //start threads to recv requst
    LOG_INFO("start thread group to recv requst")
    for(auto& topic : topics_) {
        recv_request_threads_.create_thread(std::bind(&ProducerServer::recv_request, this, topic));
    }

    //start threads to send msg
    LOG_INFO("start thread group to send msg")
    for(auto& topic : topics_) {
        send_msg_threads_.create_thread(std::bind(&ProducerServer::send_msg, this, topic));
    }

    //start threads to delete or recover msg
    LOG_INFO("start thread group to delete or recover msg")
    for(auto& topic : topics_) {
        del_recover_msg_threads_.create_thread(std::bind(&ProducerServer::delete_recover, this, topic));
    }

    LOG_INFO("start thread group compelte");
}

template<typename T>
void ProducerServer<T>::stop() {
    running_ = false;
    LOG_INFO("stop thread group to recv requst");
    recv_request_threads_.join_all();
    LOG_INFO("stop thread group to send msg")
    send_msg_threads_.join_all();
    LOG_INFO("start thread group to delete or recover msg")
    del_recover_msg_threads_.join_all();
    LOG_INFO("stop thread group compelte");
}

template<typename T>
void ProducerServer<T>::delete_recover(const std::string& topic) {
    LOG_INFO("start delete or recover thread for topic " + topic);
    while(running_) {

    }
    LOG_INFO("stop delete or recover thread for topic " + topic);
}

template<typename T>
void ProducerServer<T>::recv_request(const std::string& topic) {
    LOG_INFO("start recv request thread for topic " + topic);
    auto ready = ready_[topic];
    std::string errstr;
    RdKafka::KafkaConsumer* consumer = RdKafka::KafkaConsumer::create(conf_, errstr);
    if (!consumer) {
        LOG_ERROR(errstr)
        exit(-1);
    }
    consumer->subscribe({topic});
    while(running_) {
        RdKafka::Message* msg = consumer->consume(500);
        switch (msg->err()) {
            case RdKafka::ERR__TIMED_OUT: {
                LOG_INFO("consume time out");
                break;
            }
            case RdKafka::ERR_NO_ERROR: {
                LOG_INFO(static_cast<const char *>(msg->payload()));
                break;
            }
            case RdKafka::ERR__UNKNOWN_TOPIC: {
                LOG_ERROR("no such topic");
                break;
            }
            case RdKafka::ERR__UNKNOWN_PARTITION: {
                LOG_ERROR("no such partition");
                break;
            }
            default: {
                LOG_ERROR(msg->errstr());
                break;
            }
        }
        
        delete msg;
    }
    consumer->close();
    delete consumer;
    LOG_INFO("stop recv request thread for topic " + topic);
}

template<typename T>
void ProducerServer<T>::send_msg(const std::string& topic) {
    LOG_INFO("start send message thread for topic " + topic);
    auto sockets = sockets_[topic];
    brwQueue<RequestMessage>* gets = gets_[topic];
    auto ready = ready_[topic];
    auto sent = sent_[topic];
    //get request for this topic
    RequestMessage req;
    while(running_) {
        //use timed to prevent hanging
        if(!gets->wait_dequeue_timed(req, std::chrono::milliseconds(50))){
            continue;
        }
        //send message with zmq to req address
        T* msg = new T();
        while(!ready->wait_dequeue_timed(msg, std::chrono::milliseconds(50)) && running_){
            continue;
        }
        auto it = sockets.find(req.sink);
        void* socket;
        if(it == sockets.end()) {
            socket = zmq_socket (zmq_ctx_, ZMQ_DEALER);
            sockets[req.sink] = socket;
            sent[req.sink] = std::set<T*>();
        } else {
            socket = it->second;
        }
        do_send(socket, msg);
        sent[req.sink].emplace(msg);
    }
    LOG_INFO("stop send message thread for topic " + topic);
}

/**
 * @param topic
 * @param msg
 */
template<typename T>
void ProducerServer<T>::send(const std::string& topic, T* msg) {
    ready_[topic].enqueue(msg);
}

/**
 * @param sink
 * @param msg
 */
template<typename T>
void ProducerServer<T>::do_send(void* socket, T* msg) {
    zmq_send_const(socket, msg->get_data(), msg->get_size(), ZMQ_DONTWAIT);
}

}

#endif //_PRODUCERSERVER_H
