/**
 * Project Message Passing
 */

#ifndef _MESSAGESINK_H
#define _MESSAGESINK_H

#include <string>
#include <vector>
#include <chrono>

#include <zmq.h>
#include <boost/thread.hpp>
#include <librdkafka/rdkafkacpp.h>

#include "log/log.h"
#include "message/request_message.pb.h"
#include "queue/readerwriterqueue.h"
#include "message/data_message.h"

namespace message_pass {

template<typename T>
class MessageSink {
        static_assert(std::is_base_of<IDataMessage, T>::value, "template parameter is not derived from IDataMessage");
    public:

        /**
         * @param server_ip 要绑定的ip
         * @param server_port 要绑定的port
         * @param topic_server kafka 地址
         * @param io_threads 处理io线程数
         */
        MessageSink(const std::string& server_ip, int server_port,  const std::string& topic_server, const std::vector<std::string>& topics, int io_threads);

        ~MessageSink();

        /**
         * @brief set sources for a topic before start
         *
         * @param topic which subscribed
         * @param sources addresses which send messages to this topic
         */
        void prepare_sources(const std::string& topic, const std::vector<std::string>& sources);

        /**
         * @brief 发送request
         *
         * @param topic 目标topic
         * @param req RequestMessage
         */
        void send_request(const std::string& topic, const RequestMessage& req);

        /**
         * @brief 发送get request
         *
         * @param topic 目标topic
         * @param num 发送请求个数
         */
        void send_get_request(const std::string& topic, std::size_t num = 1);

        /**
         * @brief 发送恢复请求
         *
         * @param topic 目标topic
         */
        void send_recover_request(const std::string& topic);

        /**
         * @brief 发送删除请求
         *
         * @param topic 目标topic
         * @param keys 要删除数据的keys
         */
        void send_del_request(const std::string& topic, const std::vector<std::size_t>& keys);

        /**
         * @brief 发送删除请求
         *
         * @param topic 目标topic
         * @param key 要删除数据的key
         */
        void send_del_request(const std::string& topic, std::size_t key);

        /**
         * @brief 发送删除请求
         *
         * @param topic 目标topic
         * @param msg 要删除的数据
         */
        void send_del_request(const std::string& topic, IDataMessage& msg);

        /**
         * @brief 发送删除请求
         *
         * @param topic 目标topic
         * @param msg 要删除的数据
         */
        void send_del_request(const std::string& topic, IDataMessage* msg);

        /**
         * @brief 轮询从对应topic的不同source中获取消息，每个source等待时间time，如果超时则从下一个source获取
         *        轮询一遍后都没有消息那么返回false
         *
         * @param topic 目标topic
         * @param msg message
         * @param milliseconds_timeout 获取每个source的message的超时时间
         * @param source 开始查询的source
         * @return true 有message返回
         * @return false 无message返回
         */
        bool get_msg(const std::string& topic, T** msg, std::size_t milliseconds_timeout, const std::string& source = "");

        /**
         * @brief 从topic的source获取message
         *
         * @param topic 目标topic
         * @param source source
         * @param milliseconds_timeout 超时时间
         * @param msg message
         * @return true 有message返回
         * @return false 无message返回
         */
        bool get_msg(const std::string& topic, const std::string& source, std::size_t milliseconds_timeout, T** msg);

        /**
         * @brief 从topic的source获取message
         *
         * @param topic 目标topic
         * @param source source的hash值
         * @param milliseconds_timeout 超时时间
         * @param msg message
         * @return true 有message返回
         * @return false 无message返回
         */
        bool get_msg(const std::string& topic, std::size_t source, std::size_t milliseconds_timeout, T** msg);

        /**
         * @brief Set the fixed recv object
         *
         * @param batch recv batch size
         * @param size recv every message size
         */
        void set_fixed_recv(std::size_t batch, std::size_t size);

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
         * 每个<topic, <source, queue>>对应一个接收队列, source使用std::size_t可以为固定长度
         */
        std::map<std::string, std::map<std::size_t, brwQueue<T*>*>> recvs_;

        /**
         * MessageSink IP
         */
        std::string server_ip_;
        /**
         * MessageSink Port
         */
        int server_port_;

        std::vector<std::string> topics_;
        std::string topic_server_;
        std::atomic_bool running_{false};
        int io_threads_;
        void* zmq_ctx_;
        bool auto_request_ = false;
        uint16_t low_water_marker_;
        RdKafka::Conf *kafka_conf_;
        boost::thread_group recv_message_threads_;
        // boost::thread_group send_request_threads_;
        std::map<std::string, RdKafka::Producer*> producers_;
        bool recv_fixed_{false};
        std::size_t fixed_size_;
        std::size_t fixed_num_;
        std::map<std::string, std::string> topic_sink_;

        std::shared_ptr<spdlog::logger> logger_;

        void init(const std::vector<std::string>& topics);

        /**
         * @param topic
         */
        void recv_message(const std::string& topic);

        void do_send_request();

        class KafkaDeliveryReportCb : public RdKafka::DeliveryReportCb {
            private:
                std::shared_ptr<spdlog::logger> logger_;
            public:
                KafkaDeliveryReportCb(std::shared_ptr<spdlog::logger> logger): logger_(logger) {}
                void dr_cb(RdKafka::Message &message) {
                    /* If message.err() is non-zero the message delivery failed permanently
                    * for the message. */
                    if (message.err()) {
                        // logger_->error("message delivery failed: " + message.errstr());
                    } else {
                        // logger_->info("message delivered to topic " + message.topic_name());
                    }
                }

                ~KafkaDeliveryReportCb() {}
        };
        KafkaDeliveryReportCb kafka_cb_;
};

/**
 * @brief Construct a new MessageSink<T>::MessageSink object
 *
 * @tparam T
 * @param server_ip
 * @param server_port
 * @param topic_server
 * @param topics
 * @param io_threads
 */
template<typename T>
MessageSink<T>::MessageSink(const std::string& server_ip, int server_port,
                            const std::string& topic_server,
                            const std::vector<std::string>& topics,
                            int io_threads)
    : server_ip_(server_ip), server_port_(server_port),
      topics_(topics), topic_server_(topic_server),
      io_threads_(io_threads),
      logger_(MessageLogger::get_logger("MessageSink")),
      kafka_cb_(this->logger_)
{
    init(topics_);
}

template<typename T>
MessageSink<T>::~MessageSink() {
    //destroy zmq
    zmq_ctx_destroy(zmq_ctx_);
    
    //destory queues
    for(auto& queue_map : recvs_) {
        for (auto& queue_pair : queue_map.second) {
            T* msg;
            bool ret;
            
            while (true) {
                ret = queue_pair.second->try_dequeue(msg);
                if(ret) {
                    delete msg;
                } else {
                    break;
                }
            }

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
void MessageSink<T>::init(const std::vector<std::string>& topics) {
    //initialize queues
    for(std::size_t i = 0; i < topics.size(); i++) {
        recvs_[topics[i]] = std::map<std::size_t, brwQueue<T*>*>();
        topic_sink_[topics[i]] = "tcp://" + server_ip_ + ":" + std::to_string(server_port_ + i);
    }
    //initialize zmq
    zmq_ctx_ = zmq_ctx_new();
    zmq_ctx_set(zmq_ctx_, ZMQ_IO_THREADS, io_threads_);
    //initialize kafka global conf
    std::string errstr;
    kafka_conf_ = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    if(kafka_conf_->set("bootstrap.servers", topic_server_, errstr) != RdKafka::Conf::CONF_OK) {
        logger_->error(errstr);
        exit(1);
    }
    if (kafka_conf_->set("dr_cb", &kafka_cb_, errstr) != RdKafka::Conf::CONF_OK) {
        logger_->error(errstr);
        exit(1);
    }
    //metrics
    // if(kafka_conf_->set("statistics.interval.ms", "5000", errstr) != RdKafka::Conf::CONF_OK) {
    //     logger_->error(errstr);
    //     exit(1);
    // }
    //initialize kafka producer
    for(auto topic : topics) {
        auto producer = RdKafka::Producer::create(kafka_conf_, errstr);
        if(producer == NULL) {
            logger_->error(errstr);
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
void MessageSink<T>::prepare_sources(const std::string& topic, const std::vector<std::string>& sources) {
    for(auto source : sources) {
        recvs_[topic][std::hash<std::string>()(source)] = new brwQueue<T*>();
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
void MessageSink<T>::send_request(const std::string& topic, const RequestMessage& rmsg) {
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
        logger_->error(error_msg);
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
        // logger_->info("enqueued reuqest message for topic: {}", topic);
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
 * @brief 发送get request
 *
 * @param topic 目标topic
 * @param num 发送请求个数
 */
template<typename T>
void MessageSink<T>::send_get_request(const std::string& topic, std::size_t num) {
    for (size_t i = 0; i < num; i++) {
        RequestMessage get_request;
        get_request.set_cmd(RequestMessage_CMD_GET);
        get_request.set_sink(topic_sink_[topic]);
        this->send_request(topic, get_request);
    }
}

/**
 * @brief 发送恢复请求
 *
 * @param topic 目标topic
 */
template<typename T>
void MessageSink<T>::send_recover_request(const std::string& topic) {
    RequestMessage get_request;
    get_request.set_cmd(RequestMessage_CMD_RECOVER);
    get_request.set_sink(topic_sink_[topic]);
    this->send_request(topic, get_request);
}

/**
 * @brief 发送删除请求
 *
 * @param topic 目标topic
 * @param keys 要删除数据的keys
 */
template<typename T>
void MessageSink<T>::send_del_request(const std::string& topic, const std::vector<std::size_t>& keys) {
    for(auto key : keys) {
        RequestMessage del_request;
        del_request.set_cmd(RequestMessage_CMD_DEL);
        del_request.set_key(key);
        del_request.set_sink(topic_sink_[topic]);
        this->send_request(topic, del_request);
    }
}

/**
 * @brief 发送删除请求
 *
 * @param topic 目标topic
 * @param key 要删除数据的key
 */
template<typename T>
void MessageSink<T>::send_del_request(const std::string& topic, std::size_t key) {
    RequestMessage del_request;
    del_request.set_cmd(RequestMessage_CMD_DEL);
    del_request.set_key(key);
    del_request.set_sink(topic_sink_[topic]);
    this->send_request(topic, del_request);
}

/**
 * @brief 发送删除请求
 *
 * @param topic 目标topic
 * @param msg 要删除的数据
 */
template<typename T>
void MessageSink<T>::send_del_request(const std::string& topic, IDataMessage& msg) {
    RequestMessage del_request;
    del_request.set_cmd(RequestMessage_CMD_DEL);
    del_request.set_key(msg.key());
    del_request.set_sink(topic_sink_[topic]);
    this->send_request(topic, del_request);
}

/**
 * @brief 发送删除请求
 *
 * @param topic 目标topic
 * @param msg 要删除的数据
 */
template<typename T>
void MessageSink<T>::send_del_request(const std::string& topic, IDataMessage* msg) {
    RequestMessage del_request;
    del_request.set_cmd(RequestMessage_CMD_DEL);
    del_request.set_key(msg->key());
    del_request.set_sink(topic_sink_[topic]);
    this->send_request(topic, del_request);
}

/**
 * @brief 轮询从对应topic的不同source中获取消息，每个source等待时间time，如果超时则从下一个source获取
 *        轮询一遍后都没有消息那么返回false
 *
 * @param topic 目标topic
 * @param msg message
 * @param milliseconds_timeout 获取每个source的message的超时时间
 * @param source 开始查询的source
 * @return true 有message返回
 * @return false 无message返回
 */
template<typename T>
bool MessageSink<T>::get_msg(const std::string& topic, T** msg, std::size_t milliseconds_timeout, const std::string& source) {
    bool ret;
    auto recv_queues = this->recvs_[topic];
    if(source == "") {
        auto it = recv_queues.begin();
        for(; it != recv_queues.end(); ++it) {
            ret = it->second->wait_dequeue_timed(msg, std::chrono::milliseconds(milliseconds_timeout));
            if(ret) {
                break;
            }
        }
    } else {
        auto it = recv_queues.find(std::hash<std::string>()(source));
        for(; it != recv_queues.end(); ++it) {
            ret = it->second->wait_dequeue_timed(msg, std::chrono::milliseconds(milliseconds_timeout));
            if(ret) {
                break;
            }
        }
    }
    return ret;
}

/**
 * @brief 从topic的source获取message
 *
 * @param topic 目标topic
 * @param source source
 * @param milliseconds_timeout 超时时间
 * @param msg message
 * @return true 有message返回
 * @return false 无message返回
 */
template<typename T>
bool MessageSink<T>::get_msg(const std::string& topic, const std::string& source, std::size_t milliseconds_timeout, T** msg) {
    brwQueue<T*>* queue = this->recvs_[topic][std::hash<std::string>()(source)];
    if(queue == nullptr) {
        return false;
    }
    return queue->wait_dequeue_timed(*msg, std::chrono::milliseconds(milliseconds_timeout));
}

/**
 * @brief 从topic的source获取message
 *
 * @param topic 目标topic
 * @param source source的hash值
 * @param milliseconds_timeout 超时时间
 * @param msg message
 * @return true 有message返回
 * @return false 无message返回
 */
template<typename T>
bool MessageSink<T>::get_msg(const std::string& topic, std::size_t source, std::size_t milliseconds_timeout, T** msg) {
    auto queue = this->recvs_[topic][source];
    if(queue == nullptr) {
        return false;
    }
    return queue->wait_dequeue_timed(*msg, std::chrono::milliseconds(milliseconds_timeout));
}

/**
 * @brief Set the fixed recv object
 *
 * @param batch recv batch size
 * @param size recv every message size
 */
template<typename T>
void MessageSink<T>::set_fixed_recv(std::size_t batch, std::size_t size) {
    this->recv_fixed_ = true;
    this->fixed_num_ = batch;
    this->fixed_size_ = size;
}

template<typename T>
void MessageSink<T>::start() {
    running_ = true;
    //every topic has a thread
    //start threads to recv requst
    logger_->info("start thread group to recv message");
    for(auto topic : this->topics_) {
        recv_message_threads_.create_thread(std::bind(&MessageSink::recv_message, this, topic));
    }

    logger_->info("start thread group compelte");

    //启动时先发送recover确保不丢数据
    logger_->info("send recover request after start");
    for(std::size_t i = 0; i < topics_.size(); i++) {
        this->send_recover_request(topics_[i]);
    }
}

template<typename T>
void MessageSink<T>::stop() {
    /* Wait for final messages to be delivered or fail.
    * flush() is an abstraction over poll() which
    * waits for all messages to be delivered. */
    for(auto producer_pair : this->producers_) {
        logger_->info("flushing final messages...");
        producer_pair.second->flush(10 * 1000 /* wait for max 10 seconds */);

        if (producer_pair.second->outq_len() > 0) {
            logger_->error(std::to_string(producer_pair.second->outq_len()) + " message(s) were not delivered");
        }
    }

    running_ = false;
    logger_->info("stop thread group to recv message");
    recv_message_threads_.join_all();
    logger_->info("stop thread group compelte");
}

/**
 * @param auto_req
 * @param low_water_marker
 */
template<typename T>
void MessageSink<T>::set_auto_request(bool auto_req, uint16_t low_water_marker) {

}

/**
 * @brief
 *
 * @tparam T
 * @param topic
 */
template<typename T>
void MessageSink<T>::recv_message(const std::string& topic) {
    logger_->info("start a thread to recv message for topic: " + topic);
    auto& recv_queues = recvs_[topic];
    thread_local void* socket = zmq_socket(zmq_ctx_, ZMQ_ROUTER);
    //ZMQ_RCVTIMEO must be int
    int recv_timeout = 50;
    //设置接收超时时间
    zmq_setsockopt(socket, ZMQ_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));
    int rc = zmq_bind(socket, topic_sink_[topic].c_str());
    assert(rc == 0);

    SPDLOG_DEBUG("start recving ...");
    auto start = std::chrono::steady_clock::now();
    std::size_t cnt = 0;

    int n_bytes;
    if(recv_fixed_) {
        while(running_) {
            //recv zmq routing id
            std::size_t source;
            n_bytes = zmq_recv(socket, &source, 8, 0);
            if(n_bytes == -1) {
                continue;
            }
            //init Message
            T* msg = new T(this->fixed_size_);
            //recv messages
            do {
                n_bytes = zmq_recv(socket, msg->data(), this->fixed_size_, 0);
            } while (n_bytes == -1 && running_);
            if(n_bytes == -1) {
                break;
            }
            recv_queues[source]->enqueue(msg);
        }
    } else {
        while(running_) {
            //recv zmq routing id
            std::size_t source;
            n_bytes = zmq_recv(socket, &source, 8, 0);
            if(n_bytes == -1) {
                continue;
            }
            // SPDLOG_DEBUG("get message from: " + std::to_string(source));

            //recv message size
            std::size_t size = 0;
            do {
                n_bytes = zmq_recv(socket, &size, 8, ZMQ_RCVMORE);
            } while (n_bytes == -1 && running_);

            // SPDLOG_DEBUG("message len: " + std::to_string(size));

            if(n_bytes == -1) {
                break;
            }

            //recv messages
            T* msg = new T(size);
            do
            {
                n_bytes = zmq_recv(socket, msg->data(), size, 0);
            } while (n_bytes == -1 && running_);
            msg->size(n_bytes);

            if(n_bytes == -1) {
                break;
            }
            auto it = recv_queues.find(source);
            if(it == recv_queues.end()) {
                recv_queues[source] = new brwQueue<T*>();
            }
            recv_queues[source]->enqueue(msg);
            if(cnt == 1000000) {
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration<double> diff = end-start;
                double speed = 1000000 / diff.count();
                SPDLOG_DEBUG("recv 1000000 messages used {}s, recv speed {}/s", diff.count(), speed);
            }
        }
    }

    zmq_close(socket);
    logger_->info("stop a thread to recv message for topic: " + topic);
}

}

#endif /* _MESSAGESINK_H */
