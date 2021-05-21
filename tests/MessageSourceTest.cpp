#include <memory>
#include <vector>
#include <unistd.h>
#include <thread>

#include "message_source/message_source.h"
#include "utils/cmdline.h"
#include "TestDataMessage.h"

using namespace message_pass;

int main(int argc, char *argv[]) {
    cmdline::parser cmdline_parser;
    cmdline_parser.add<std::string>("topic_server", 0, "topic server address (kafka broker list)");
    cmdline_parser.add<std::string>("topic", 0, "subscribe topic");
    cmdline_parser.add<int>("io_threads", 0, "threads number for zmq");
    cmdline_parser.add<std::string>("source_id", 0, "message source identity", false);

    cmdline_parser.parse_check(argc, argv);

    auto topic = cmdline_parser.get<std::string>("topic");

    std::shared_ptr<MessageSource<TestDataMessage>>
    ps(new MessageSource<TestDataMessage>(
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

    SPDLOG_INFO("start sending 1000,000 messages ...");
    for(std::size_t i = 0; i < 1000000; i++) {
        TestDataMessage* msg = new TestDataMessage(8);
        std::memcpy(msg->data(), &i, sizeof(i));
        msg->size(500);
        ps->send(topic, msg);
    }
    SPDLOG_INFO("sending messages stop");

    sleep(6000);
    ps->stop();

    return 0;
}
