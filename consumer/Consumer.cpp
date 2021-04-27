/**
 * Project Message Passing
 */


#include "consumer/Consumer.h"

/**
 * Consumer implementation
 */

namespace message_pass {

Consumer::Consumer() {

}

/**
 * @param cs
 */
template<T>
Consumer::Consumer(std::shared_ptr<ConsumerServer<T>> cs) {

}

/**
 * @param topic
 */
void Consumer::send_request(const std::string& topic) {

}

/**
 * @param msg
 * @param topic
 */
template<T>
void Consumer::get_msg(IDataMessage<T>* msg, const std::string& topic) {

}

/**
 * @param topic
 * @param keys
 */
template<T>
void Consumer::del_msg(const std::string& topic, const std::list<T>& keys) {

}

/**
 * @param topic
 * @param key
 */
template<T>
void Consumer::del_msg(const std::string& topic, const T& key) {

}

/**
 * @param cs
 */
template<T>
void Consumer::set_consumer_server(std::shared_ptr<ConsumerServer<T>> cs) {

}

/**
 * @return std::shared_ptr<ConsumerServer<T>>
 */
template<T>
std::shared_ptr<ConsumerServer<T>> Consumer::get_consumer_server() {
    return consumer_server_;
}

}
