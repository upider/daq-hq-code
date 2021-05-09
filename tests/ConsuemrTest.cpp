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
    //start 必须在send request之前调用
    cs->start();

    TestDataMessage* msg;
    cs->send_get_request(topic);

    while(!cs->get_msg(topic, source, 5000, &msg));

    char* str = new char[msg->size() + 1];
    str[msg->size()] = '\0';
    std::memcpy(str, msg->data(), msg->size());
    std::cout << "recv " << msg->size() << " bytes message: " << str << "\n";

    sleep(30);
    cs->stop();

    delete msg;
    return 0;
}
