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

RequestMessage& RequestMessage::operator=(const RequestMessage& other) {
	this->cmd = other.cmd;
	this->sink = other.sink;
	return *this;
}

}
