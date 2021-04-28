/**
 * Project Message Passing
 */


#include "message/RequestMessage.h"

/**
 * RequestMessage implementation
 */

namespace message_pass {

RequestMessage::RequestMessage() {}

RequestMessage::RequestMessage(const RequestMessage& other) {}

/**
 * @param cmd
 * @param sink
 */
RequestMessage::RequestMessage(const std::string& cmd, const std::string& sink)
    : cmd(cmd), sink(sink) {}

/**
 * @param cmd
 * @param key
 */
RequestMessage::RequestMessage(const std::string& cmd, std::size_t key)
:cmd(cmd), key(key) {}

/**
 * @param cmd
 * @param sink
 * @param key
 */
RequestMessage::RequestMessage(const std::string& cmd, const std::string& sink, std::size_t key)
: cmd(cmd), sink(sink), key(key) {}

RequestMessage& RequestMessage::operator=(const RequestMessage& other) {
	this->cmd = other.cmd;
	this->sink = other.sink;
	this->key = other.key;
	return *this;
}

}
