#include <cstring>

#include "message/data_message.h"

using namespace message_pass;

class TestDataMessage : virtual public IDataMessage
{
    private:
        char* buffer_;
        std::size_t size_;
        std::size_t capacity_;

    public:
        //请实现3个构造函数
        TestDataMessage() : buffer_() {}

        TestDataMessage(std::size_t capacity): size_(0), capacity_(capacity) {
            buffer_ = new char[capacity];
        }

        TestDataMessage(const void* data, std::size_t size, std::size_t capacity): size_(size), capacity_(capacity) {
            buffer_ = new char[capacity];
            std::memcpy(buffer_, data, size);
        }

        virtual ~TestDataMessage() {
            delete buffer_;
        }

        //其他函数
        virtual void* data() {
            return buffer_;
        }

        virtual const void* const_data() const {
            return buffer_;
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
            return 100 + this->buffer_[0];
        }

        std::string to_string() {
            return std::string(buffer_);
        }
};
