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
    
RequestMessage();

RequestMessage(const RequestMessage&);

/**
 * @param cmd
 * @param sink
 */
RequestMessage(const std::string& cmd, const std::string& sink);

RequestMessage& operator=(const RequestMessage& other);

};


}
#endif //_REQUESTMESSAGE_H
