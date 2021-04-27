#include <memory>
#include <thread>

#include <producer/ProducerServer.h>

using namespace message_pass;

class TestMessage : virtual public IDataMessage
{
public:
    void *get_data()
    {
        return 0;
    }
    size_t get_size()
    {
        return 0;
    }
    bool operator<(const IDataMessage &other)
    {
        return false;
    }
    size_t get_key()
    {
        return 100;
    }
    void set_data(void *data, size_t size) {}
};

class TestMessage2
{
};

int main(void)
{

    std::shared_ptr<ProducerServer<TestMessage>>
        ps(new ProducerServer<TestMessage>("192.168.37.201:9092", {"test"}, 1));
    std::thread start_thread([&]() {
        ps->start();
    });
    start_thread.join();

    sleep(500);
    ps->stop();
    return 0;
}
