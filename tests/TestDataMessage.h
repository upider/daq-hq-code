#include <cstring>

#include "message/data_message.h"

using namespace message_pass;

class TestDataMessage : virtual public IDataMessage
{
    private:
        char* buffer_=NULL;
        std::size_t size_;
        std::size_t capacity_;

    public:
        //请实现3个构造函数
        TestDataMessage() : buffer_() {}

        TestDataMessage(std::size_t capacity): buffer_(new char[capacity]), size_(0), capacity_(capacity) {
            std::memset(buffer_, 0, capacity);
        }

        TestDataMessage(const void* data, std::size_t size, std::size_t capacity): size_(size), capacity_(capacity) {
            buffer_ = new char[capacity];
            std::memcpy(buffer_, data, size);
            std::memset(buffer_+size, 0, capacity-size);
        }

        virtual ~TestDataMessage() {
            delete[] buffer_;
        }

        //其他函数
        virtual void* data() {
            return static_cast<void*>(buffer_);
        }

        virtual const void* const_data() const {
            return static_cast<void*>(buffer_);
        }

        virtual std::size_t size() const {
            return size_;
        }

        virtual std::size_t size(std::size_t size) {
            this->size_ = size;
            return size_;
        }

        virtual std::size_t resize(std::size_t size) {
            this->size_ = size;
            std::memset(&buffer_[size], 0, this->capacity_ - this->size_);
            return this->size_;
        }

        virtual std::size_t cap() const {
            return this->capacity_;
        }

        virtual std::size_t grow(std::size_t size) {
            //TODO: grow
            return this->capacity_;
        }

        virtual void clear() {
            std::memset(buffer_, 0, this->capacity_);
        }

        virtual std::size_t shrink() {
            //TODO: shrink
            return this->size_;
        }

        virtual std::size_t add(const void* data, std::size_t size) {
            //TODO: add
            return this->size_;
        }

        virtual std::size_t key() {
            std::size_t key = 0;
            std::memcpy(&key, this->buffer_ + 0, sizeof(key));
            return key;
        }

        std::string to_string() {
            return std::string(buffer_);
        }
};
