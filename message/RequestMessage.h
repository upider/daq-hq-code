/**
 * Project Message Passing
 */

#ifndef _REQUESTMESSAGE_H
#define _REQUESTMESSAGE_H

#include <string>

namespace message_pass{

class RequestMessage {
public: 
    std::string cmd;
    std::string sink;
    std::size_t key;
    
RequestMessage();

RequestMessage(const RequestMessage&);

/**
 * @param cmd
 * @param sink
 */
RequestMessage(const std::string& cmd, const std::string& sink);

/**
 * @param cmd
 * @param key
 */
RequestMessage(const std::string& cmd, std::size_t key);

/**
 * @param cmd
 * @param sink
 * @param key
 */
RequestMessage(const std::string& cmd, const std::string& sink, std::size_t key);

RequestMessage& operator=(const RequestMessage& other);

};


}
#endif //_REQUESTMESSAGE_H
