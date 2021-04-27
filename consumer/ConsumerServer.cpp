/**
 * Project Message Passing
 */


#include "consumer/ConsumerServer.h"

/**
 * ConsumerServer implementation
 * 
 * 每个topic可以对应一个Consumer
 */

namespace message_pass{

/**
 * @param server_ip
 * @param server_port
 * @param topic_server
 * @param io_threads
 */
void ConsumerServer::ConsumerServer(const std::string& server_ip, int server_port, const std::string& topic_server, size_t io_threads) {

}

/**
 * @param req
 */
void ConsumerServer::send_request(const MessageRequest& req) {

}

/**
 * @param key
 */
void ConsumerServer::del_msg(const std::list<T>& key) {

}

/**
 * @param key
 */
void ConsumerServer::del_msg(const T& key) {

}

/**
 * @param topic
 * @param msg
 */
void ConsumerServer::get_msg(const std::string& topic, IDataMessage<T>* msg) {

}

void ConsumerServer::start() {

}

void ConsumerServer::stop() {

}

/**
 * @param auto_req
 * @param low_water_marker
 */
void ConsumerServer::set_auto_request(bool auto_req, uint16_t low_water_marker) {

}

/**
 * @param topic
 */
void ConsumerServer::recv(const std:string& topic) {

}

void ConsumerServer::do_send_request() {

}

}
