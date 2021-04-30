#include <memory>
#include <thread>

#include "consumer/ConsumerServer.h"
#include "message/IMultiDataMessage.h"

using namespace message_pass;

class TestMessage : virtual public IMultiDataMessage
{
public:
    size_t get_buf_num()
    {
        return 0;
    }
    size_t get_key()
    {
        return 100;
    }
    void set_key(std::size_t key) {}
    void add_buf(size_t size) {}
    std::pair<void*, std::size_t> operator[](std::size_t) {
        return std::pair<void*, std::size_t>(nullptr, 0);
    }
};

int main(void)
{

    std::shared_ptr<ConsumerServer<TestMessage>>
        ps(new ConsumerServer<TestMessage>("0.0.0.0", 9999, "192.168.37.201:9092", {"test"}, 1));
    std::thread start_thread([&]() {
        ps->start();
    });
    start_thread.join();

    sleep(500);
    ps->stop();
    return 0;
}
