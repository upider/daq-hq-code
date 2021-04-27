/**
 * Project Message Passing
 */


#ifndef _CONSUMERSERVER_H
#define _CONSUMERSERVER_H

namespace message_pass{

template<T>
class ConsumerServer {
public: 
    
/**
 * @param server_ip
 * @param server_port
 * @param topic_server
 * @param io_threads
 */
void ConsumerServer(const std::string& server_ip, int server_port, const std::string& topic_server, size_t io_threads);
    
/**
 * @param req
 */
void send_request(const MessageRequest& req);
    
/**
 * @param key
 */
void del_msg(const std::list<T>& key);
    
/**
 * @param key
 */
void del_msg(const T& key);
    
/**
 * @param topic
 * @param msg
 */
void get_msg(const std::string& topic, IDataMessage<T>* msg);
    
void start();
    
void stop();
    
/**
 * @param auto_req
 * @param low_water_marker
 */
void set_auto_request(bool auto_req, uint16_t low_water_marker);
private: 
    /**
 * 每个topic对应一个接收队列
 */
std::map<std::string,moodycamel::BlockingReaderWriterQueue<void*>> recv_;
    std::map<std::string,moodycamel::BlockingReaderWriterQueue<MessageRequest>> requests_;
    /**
 * ConsumerServer IP
 */
std::string server_ip_;
    /**
 * ConsumerServer Port
 */
int server_port_;
    std::string topic_server_;
    std::atomic_bool running_ = false;
    size_t io_threads_;
    void* zmq_ctx_;
    bool auto_request_ = false;
    uint16_t low_water_marker_;
    memory::MultipleBufferManager buffer_manager_;
    
/**
 * @param topic
 */
void recv(const std:string& topic);
    
void do_send_request();
};

}
#endif //_CONSUMERSERVER_H
