#include <memory>
#include <unistd.h>

#include "producer/ProducerServer.h"
#include "message/IMultiDataMessage.h"
#include "TestDataMessage.h"

using namespace message_pass;

int main(void)
{
    std::string topic{"test"};
    std::string id{"centos204"};

    std::shared_ptr<ProducerServer<TestDataMessage>> 
    ps(new ProducerServer<TestDataMessage>("192.168.37.201:9092", {topic}, 1));

    ps->set_identity(id);
    ps->start();

    TestDataMessage* msg = new TestDataMessage("hello world this is IHEP", 24, 24);
    // msg->add("hello world", 11);
    ps->send(topic, msg);

    sleep(30);
    ps->stop();

    delete msg;
    return 0;
}
