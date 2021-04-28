#ifndef _IMULTIDATAMESSAGE_H
#define _IMULTIDATAMESSAGE_H

#include <utility>
#include <cstddef>
#include <vector>

namespace message_pass {

class IMultiDataMessage {
public:
    IMultiDataMessage();
    IMultiDataMessage(const std::vector<std::size_t>& buf_size);
    IMultiDataMessage(std::size_t buf_num, std::size_t buf_size);
    virtual ~IMultiDataMessage();
    /**
     * @param key
     */
    virtual void set_key(std::size_t key) = 0;
    virtual std::size_t get_key() = 0;
    virtual std::size_t get_buf_num() = 0;
    virtual void add_buf(std::size_t buf_size) = 0;
    virtual std::pair<void *, std::size_t> operator[](std::size_t pos) = 0;
};

}

#endif //_IMULTIDATAMESSAGE_H
