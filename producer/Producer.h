/**
 * Project Message Passing
 */


#ifndef _PRODUCER_H
#define _PRODUCER_H

#include <string>
#include <memory>

namespace message_pass{

//@template T key
template<T>
class Producer {
public: 
    
Producer();
    
/**
 * @param ps
 */
Producer(std::shared_ptr<ProducerServer<T>> ps);
    
/**
 * @param ps
 */
void set_producer_server(std::shared_ptr<ProducerServer<T>> ps);
    
/**
 * @param topic
 * @param msg
 */
void send(const std::string& topic, IDataMessage<T>* msg);
    
std::shared_ptr<ProducerServer<T>> get_producer_server();
private: 
    std::shared_ptr<ProducerServer<T>> producer_server_;
};

}

#include <Producer.tpp>
#endif //_PRODUCER_H
