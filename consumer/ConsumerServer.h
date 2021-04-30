/**
 * Project Message Passing
 */

#ifndef _CONSUMERSERVER_H
#define _CONSUMERSERVER_H

#include <string>
#include <vector>
#include <chrono>

#include <zmq.h>
#include <boost/thread.hpp>
#include <librdkafka/rdkafkacpp.h>

#include "log/log.h"
#include "message/RequestMessage.pb.h"
#include "queue/readerwriterqueue.h"

namespace message_pass {

class IMultiDataMessage;

template<typename T>
class ConsumerServer {
    static_assert(std::is_base_of<IMultiDataMessage, T>::value, "template parameter is not derived from IMultiDataMessage");
    public:

        /**
         * @param server_ip
         * @param server_port
         * @param topic_server
         * @param io_threads
         */
        ConsumerServer(const std::string& server_ip, int server_port,  const std::string& topic_server, const std::vector<std::string>& topics, size_t io_threads);

        ~ConsumerServer();

        /**
         * @brief set sources for a topic before start
         *
         * @param topic which subscribed
         * @param sources addresses which send messages to this topic
         */
        void prepare_sources(const std::string& topic, const std::vector<std::string>& sources);

        void send_request(const std::string& topic, const RequestMessage& req);

        /**
         * @param key
         */
        void del_msg(const std::vector<std::size_t>& keys);

        /**
         * @param key
         */
        void del_msg(std::size_t key);

        /**
         * @param topic
         * @param msg
         */
        void get_msg(const std::string& topic, T* msg);

        void start();

        void stop();

        /**
         * @param auto_req
         * @param low_water_marker
         */
        void set_auto_request(bool auto_req, uint16_t low_water_marker);
    private:

        template <typename T2>
        using brwQueue = moodycamel::BlockingReaderWriterQueue<T2>;

        /**
         * 每个<topic, <source, queue>>对应一个接收队列
         */
        std::map<std::string, std::map<std::string, brwQueue<T*>*>> recvs_;

        /**
         * ConsumerServer IP
         */
        std::string server_ip_;
        /**
         * ConsumerServer Port
         */
        int server_port_;

        std::vector<std::string> topics_;
        std::string topic_server_;
        std::atomic_bool running_{false};
        size_t io_threads_;
        void* zmq_ctx_;
        bool auto_request_ = false;
        uint16_t low_water_marker_;
        RdKafka::Conf *kafka_conf_;
        boost::thread_group recv_message_threads_;
        boost::thread_group send_request_threads_;
        std::map<std::string, RdKafka::Producer*> producers_;

        void init( const std::vector<std::string>& topics);

        /**
         * @param topic
         */
        void recv_message(const std::string& topic);

        void do_send_request();

        class KafkaDeliveryReportCb : public RdKafka::DeliveryReportCb {
            public:
                void dr_cb(RdKafka::Message &message) {
                    /* If message.err() is non-zero the message delivery failed permanently
                    * for the message. */
                    if (message.err()) {
                        LOG_ERROR("Message delivery failed: " + message.errstr());
                    } else {
                        LOG_INFO("Message delivered to topic " + message.topic_name());
                    }
                }

                ~KafkaDeliveryReportCb() {}
        };
        KafkaDeliveryReportCb kafka_cb_;
};

/**
 * @brief Construct a new Consumer Server< T>:: Consumer Server object
 *
 * @tparam T
 * @param server_ip
 * @param server_port
 * @param topic_server
 * @param topics
 * @param io_threads
 */
template<typename T>
ConsumerServer<T>::ConsumerServer(const std::string& server_ip, int server_port, const std::string& topic_server, const std::vector<std::string>& topics, size_t io_threads)
    : server_ip_(server_ip), server_port_(server_port), topics_(topics), topic_server_(topic_server), io_threads_(io_threads) {
    init(topics_);
}

template<typename T>
ConsumerServer<T>::~ConsumerServer() {
    //TODO: delete element in queue
    //destory queues
    for(auto& queue_map : recvs_) {
        for (auto& queue_pair : queue_map.second) {
            delete queue_pair.second;
        }
    }
    //destroy kafka
    delete kafka_conf_;
    for(auto producer : producers_) {
        delete producer.second;
    }
}

template<typename T>
void ConsumerServer<T>::init(const std::vector<std::string>& topics) {
    //initialize queues
    for(std::string topic : topics) {
        recvs_[topic] = std::map<std::string, brwQueue<T*>*>();
    }
    //initialize zmq
    zmq_ctx_ = zmq_ctx_new();
    zmq_ctx_set(zmq_ctx_, ZMQ_IO_THREADS, io_threads_);
    //initialize kafka global conf
    std::string errstr;
    kafka_conf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if(kafka_conf_->set("bootstrap.servers", topic_server_, errstr) != RdKafka::Conf::CONF_OK) {
        LOG_ERROR(errstr);
        exit(1);
    }
    if (kafka_conf_->set("dr_cb", &kafka_cb_, errstr) != RdKafka::Conf::CONF_OK) {
        LOG_ERROR(errstr);
        exit(1);
    }
    //initialize kafka producer
    for(auto topic : topics) {
        auto producer = RdKafka::Producer::create(kafka_conf_, errstr);
        if(producer == NULL) {
            LOG_ERROR(errstr);
            exit(-1);
        }
        producers_[topic] = producer;
    }
}

/**
 * @brief set sources for a topic before start
 *
 * @param topic which subscribed
 * @param sources addresses which send messages to this topic
 */
template<typename T>
void ConsumerServer<T>::prepare_sources(const std::string& topic, const std::vector<std::string>& sources) {
    for(auto& source : sources) {
        recvs_[topic][source] = new brwQueue<T*>();
    }
}

/**
 * @brief 
 * 
 * @tparam T 
 * @param topic 
 * @param rmsg 
 */
template<typename T>
void ConsumerServer<T>::send_request(const std::string& topic, const RequestMessage& rmsg) {
    auto producer = producers_[topic];
    std::string str_rmsg;
    rmsg.SerializeToString(&str_rmsg);
retry:
    RdKafka::ErrorCode err = producer->produce(
                                    topic,
                                    RdKafka::Topic::PARTITION_UA,
                                    RdKafka::Producer::RK_MSG_COPY,
                                    /* Value */
                                    const_cast<char *>(str_rmsg.c_str()), str_rmsg.size(),
                                    /* Key */
                                    NULL, 0,
                                    /* Timestamp (defaults to current time) */
                                    0,
                                    /* Per-message opaque value passed to
                                    * delivery report */
                                    NULL);

    if (err != RdKafka::ERR_NO_ERROR) {
        std::string error_msg{"Failed to produce to topic "};
        error_msg.append(topic).append(": ").append(RdKafka::err2str(err));
        LOG_ERROR(error_msg);
        if (err == RdKafka::ERR__QUEUE_FULL) {
            /* If the internal queue is full, wait for
            * messages to be delivered and then retry.
            * The internal queue represents both
            * messages to be sent and messages that have
            * been sent or failed, awaiting their
            * delivery report callback to be called.
            *
            * The internal queue is limited by the
            * configuration property
            * queue.buffering.max.messages */
            producer->poll(1000/*block for max 1000ms*/);
            goto retry;
        }
    } else {
        LOG_INFO("Enqueued message for topic " + topic);
    }

    /* A producer application should continually serve
    * the delivery report queue by calling poll()
    * at frequent intervals.
    * Either put the poll call in your main loop, or in a
    * dedicated thread, or call it after every produce() call.
    * Just make sure that poll() is still called
    * during periods where you are not producing any messages
    * to make sure previously produced messages have their
    * delivery report callback served (and any other callbacks
    * you register). */
    producer->poll(0);
}

/**
 * @param key
 */
template<typename T>
void ConsumerServer<T>::del_msg(const std::vector<std::size_t>& keys) {
}

/**
 * @param key
 */
template<typename T>
void ConsumerServer<T>::del_msg(std::size_t key) {

}

/**
 * @param topic
 * @param msg
 */
template<typename T>
void ConsumerServer<T>::get_msg(const std::string& topic, T* msg) {

}

template<typename T>
void ConsumerServer<T>::start() {
    running_ = true;
    //every topic has a thread

    //start threads to recv requst
    LOG_INFO("start thread group to recv message")
    for(auto& topic : topics_) {
        recv_message_threads_.create_thread(std::bind(&ConsumerServer::recv_message, this, topic));
    }

    LOG_INFO("start thread group compelte");
}

template<typename T>
void ConsumerServer<T>::stop() {
    LOG_INFO("stop thread group to send request")
    send_request_threads_.join_all();
    LOG_INFO("stop thread group to recv message")
    recv_message_threads_.join_all();
    running_ = false;
    LOG_INFO("stop thread group compelte");
}

/**
 * @param auto_req
 * @param low_water_marker
 */
template<typename T>
void ConsumerServer<T>::set_auto_request(bool auto_req, uint16_t low_water_marker) {

}

/**
 * @param topic
 */
template<typename T>
void ConsumerServer<T>::recv_message(const std::string& topic) {
    auto recv_queues = recvs_[topic];
    while (running_) {
        
    }
    
}

template<typename T>
void ConsumerServer<T>::do_send_request() {

}

}
#endif //_CONSUMERSERVER_H
