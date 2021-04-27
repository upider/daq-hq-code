/**
 * Project Message Passing
 */


#ifndef _IDATAMESSAGE_H
#define _IDATAMESSAGE_H

#include <cstddef>

namespace message_pass {

class IDataMessage {
    public:

        virtual ~IDataMessage();

        virtual void* get_data() = 0;

        virtual std::size_t get_size() = 0;

        virtual std::size_t get_key() = 0;

        /**
         * compare key
         * @param data_message
         */
        virtual bool operator<(const IDataMessage& data_message) = 0;

        /**
         * @param data
         * @param size
         */
        virtual void set_data(void* data, std::size_t size) = 0;
};

}
#endif //_IDATAMESSAGE_H
