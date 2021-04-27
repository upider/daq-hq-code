/**
 * Project Message Passing
 */


#ifndef _CONSUMER_H
#define _CONSUMER_H

#include <memory>

namespace message_pass{

template<T>
class ConsumerServer<T>;

template<T>
class Consumer {
public: 
    
Consumer();
    
/**
 * @param cs
 */
Consumer(std::shared_ptr<ConsumerServer<T>> cs);
    
/**
 * @param topic
 */
void send_request(const std::string& topic);
    
/**
 * @param msg
 * @param topic
 */
void get_msg(IDataMessage<T>* msg, const std::string& topic);
    
/**
 * @param topic
 * @param keys
 */
void del_msg(const std::string& topic, const std::list<T>& keys);
    
/**
 * @param topic
 * @param key
 */
void del_msg(const std::string& topic, const T& key);
    
/**
 * @param cs
 */
void set_consumer_server(std::shared_ptr<ConsumerServer<T>> cs);
    
std::shared_ptr<ConsumerServer<T>> get_consumer_server();

private: 
    std:shared_ptr<ConsumerServer<T>> consumer_server_;
};

}
#endif //_CONSUMER_H
