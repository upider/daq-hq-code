#include "message_sink/message_sink.h"
#include "utils/cmdline.h"

#include "TestDataMessage.h"

using namespace message_pass;

int main(int argc, char *argv[])
{
    cmdline::parser cmdline_parser;
    cmdline_parser.add<std::string>("ip", 0, "server ip address");
    cmdline_parser.add<int>("port", 0, "server port");
    cmdline_parser.add<std::string>("topic_server", 0, "topic server address (kafka broker list)");
    cmdline_parser.add<std::string>("topic", 0, "subscribe topic");
    cmdline_parser.add<std::size_t>("io_threads", 0, "threads number for zmq");
    cmdline_parser.add<std::string>("source_id", 0, "message source identity");
    cmdline_parser.parse_check(argc, argv);

    std::string topic = cmdline_parser.get<std::string>("topic");
    std::string source = cmdline_parser.get<std::string>("source_id");

    std::shared_ptr<MessageSink<TestDataMessage>>
    cs(new MessageSink<TestDataMessage>(
       cmdline_parser.get<std::string>("ip"),
       cmdline_parser.get<int>("port"),
       cmdline_parser.get<std::string>("topic_server"),
       {topic},
       cmdline_parser.get<std::size_t>("io_threads")
       )
    );

    //提前准备好source
    if(!source.empty()) {
        cs->prepare_sources(topic, {source});
    }
    
    //start 必须在send request之前调用
    cs->start();

    std::atomic_bool run{true};
    std::thread th([&run, cs, topic, source](){
        while (run) {
            TestDataMessage* msg;

            cs->send_get_request(topic);

            bool ret;
            do {
                ret = cs->get_msg(topic, source, 50, &msg);
            } while(!ret && run);
            
            if(!ret) {
                return;
            }
            
            SPDLOG_INFO("recv {} bytes message, key is: {}", msg->size(), msg->key());
            cs->send_del_request(topic, msg);
            delete msg;
        }
    });

    sleep(60);
    run = false;
    th.join();
    cs->stop();

    return 0;
}
