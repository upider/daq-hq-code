#include <thread>

#include "consumer/ConsumerServer.h"
#include "message/IDataMessage.h"
#include "TestDataMessage.h"

using namespace message_pass;

int main(void)
{
    std::shared_ptr<ConsumerServer<TestDataMessage>>
    cs(new ConsumerServer<TestDataMessage>("192.168.37.204", 9999, "192.168.37.201:9092", {"test"}, 1));
    
    std::string topic{"test"};
    std::string source{"centos204"};
    cs->prepare_sources(topic, {source});
    std::thread start_thread([=]() {
        //start 必须在send request之前调用
        cs->start();
    });
    start_thread.join();

    TestDataMessage* msg;
    cs->send_get_request(topic);

    while(!cs->get_msg(topic, source, 5000, &msg));

    std::cout << msg->size() << "\n";
    std::cout << std::string((char*)msg->data()) << "\n";

    sleep(50);
    cs->stop();

    delete msg;
    return 0;
}
