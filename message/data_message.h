#ifndef DATAMESSAGE_H
#define DATAMESSAGE_H

#include <cstddef>

namespace message_pass {

class IDataMessage
{
    public:
        /**
         * @brief Construct a new IDataMessage object
         *
         */
        IDataMessage();

        /**
         * @brief Construct a new IDataMessage object
         *
         * @param capacity 数据容量
         */
        IDataMessage(std::size_t capacity);

        IDataMessage(const void* data, std::size_t size, std::size_t capacity);

        /**
         * @brief Destroy the IDataMessage object
         *
         */
        virtual ~IDataMessage();

        /**
         * @brief 得到数据的指针
         *
         * @return void* 数据的指针
         */
        virtual void* data() = 0;

        /**
         * @brief 得到数据的const指针
         *
         * @return const void* 数据的const指针
         */
        virtual const void* const_data() const = 0;

        /**
         * @brief 得到数据大小
         *
         * @return std::size_t 数据大小
         */
        virtual std::size_t size() const = 0;

        /**
         * @brief 设置数据大小
         *
         * @param size 要设置的size
         * @return std::size_t 设置后的size
         */
        virtual std::size_t size(std::size_t size) = 0;

        /**
         * @brief 设置数据大小
         *
         * @param size 要设置的数据大小
         * @return std::size_t 设置后的数据大小
         */
        virtual std::size_t resize(std::size_t size) = 0;

        /**
         * @brief 获得数据容量
         *
         * @return std::size_t 数据容量
         */
        virtual std::size_t cap() const = 0;

        /**
         * @brief 增大数据容量
         *
         * @param size 要增大的值
         * @return std::size_t 增大后的容量
         */
        virtual std::size_t grow(std::size_t size) = 0;

        /**
         * @brief 清空数据
         *
         */
        virtual void clear() = 0;

        /**
         * @brief 缩小容量至size=cap
         *
         * @return std::size_t 缩小后的cap
         */
        virtual std::size_t shrink() = 0;

        /**
         * @brief 增加数据
         *
         * @param data 数据
         * @param size 数据大小
         * @return std::size_t 增加数据后的size
         */
        virtual std::size_t add(const void* data, std::size_t size) = 0;

        /**
         * @brief 获取key值，key值应该在对象构造时或第一次调用key()时确定
         *         key值应该从data中解析到而不是随意设置，这样在网络发送时就不用将key和data分开发送
         *         因此不应该有set_key(std::size_t)或者key(std::size_t)
         *
         * @return std::size_t
         */
        virtual std::size_t key() = 0;
};

}
#endif /* DATAMESSAGE_H */
