#include <memory>
#include <unistd.h>
#include <thread>

#include "message_source/message_source.h"
#include "utils/cmdline.h"

#include "TestDataMessage.h"

using namespace message_pass;

int main(int argc, char *argv[])
{
    cmdline::parser cmdline_parser;
    cmdline_parser.add<std::string>("topic_server", 0, "topic server address (kafka broker list)");
    cmdline_parser.add<std::string>("topic", 0, "subscribe topic");
    cmdline_parser.add<int>("io_threads", 0, "threads number for zmq");
    cmdline_parser.add<std::string>("source_id", 0, "message source identity", false);
    cmdline_parser.parse_check(argc, argv);

    auto topic = cmdline_parser.get<std::string>("topic");

    std::shared_ptr<MessageSource<TestDataMessage>>
    ps(new MessageSource<TestDataMessage>
    (
       cmdline_parser.get<std::string>("topic_server"),
       {topic},
       cmdline_parser.get<int>("io_threads")
    )
    );

    auto id = cmdline_parser.get<std::string>("source_id");
    if(!id.empty()) {
        ps->set_identity(id);
    }

    ps->start();
    std::atomic_bool run{true};
    std::string hello = "hello world";
    std::thread th([&run, ps, topic, hello](){
        std::size_t i = 0;
        std::size_t total = 500;
        while(i < 10) {
            std::size_t size = sizeof(i);
            TestDataMessage* msg = new TestDataMessage(total);
            std::memcpy(static_cast<char*>(msg->data()), &i, size);
            std::memcpy(static_cast<char*>(msg->data())+size, hello.c_str(), hello.size());
            std::memset(static_cast<char*>(msg->data())+hello.size()+size, 0, total-size-hello.size());
            msg->size(total);
            ps->send(topic, msg);
            i++;
            sleep(1);
        }
    });

    sleep(60);
    run = false;
    th.join();
    ps->stop();

    return 0;
}
