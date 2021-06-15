#include <fstream>

#include "message_sink/message_sink.h"
#include "utils/cmdline.h"

#include "TestDataMessage.h"

using namespace message_pass;

// 接收10次消息发送一次deleteRequest，接收第60个消息后，发送deleteRequest再挂掉，
// 检验恢复后能否继续正常运行
int main(int argc, char *argv[])
{
    cmdline::parser cmdline_parser;
    cmdline_parser.add<std::string>("ip", 0, "server ip address");
    cmdline_parser.add<int>("port", 0, "server port");
    cmdline_parser.add<std::string>("topic_server", 0, "topic server address (kafka broker list)");
    cmdline_parser.add<std::string>("topic", 0, "subscribe topic");
    cmdline_parser.add<int>("io_threads", 0, "threads number for zmq");
    cmdline_parser.add<std::string>("source_id", 0, "message source identity");
    cmdline_parser.add<bool>("fault", true, "是否崩溃测试");
    cmdline_parser.parse_check(argc, argv);

    std::string topic = cmdline_parser.get<std::string>("topic");
    std::string source = cmdline_parser.get<std::string>("source_id");
    bool fault = cmdline_parser.get<bool>("fault");

    std::shared_ptr<MessageSink<TestDataMessage>>
        cs(new MessageSink<TestDataMessage>(
            cmdline_parser.get<std::string>("ip"),
            cmdline_parser.get<int>("port"),
            cmdline_parser.get<std::string>("topic_server"),
            {topic},
            cmdline_parser.get<int>("io_threads")));

    //提前准备好source
    if (!source.empty())
    {
        cs->prepare_sources(topic, {source});
    }

    //start 必须在send request之前调用
    cs->start();

    std::atomic_bool run{true};
    std::thread th([&run, cs, topic, source, fault]()
                   {
                       int i = 0;
                       std::vector<std::size_t> delKeys(10);
                       std::ofstream file;
                       file.open("./recv", std::ios::out);
                       while (run)
                       {
                           TestDataMessage *msg;

                           cs->send_get_request(topic);
                           sleep(5);

                           bool ret;
                           do
                           {
                               ret = cs->get_msg(topic, source, 50, &msg);
                           } while (!ret && run);
                           i++;

                           file << "source ==> " << source << ", key ==> " << msg->key() << "\n";
                           file.flush();
                           delKeys.push_back(msg->key());

                           if (i % 10 == 0)
                           {
                               cs->send_del_request(topic, delKeys);
                               delKeys.clear();
                           }

                           if (i == 60 && fault)
                           {
                               SPDLOG_ERROR("FAULT HAPPEND");
                               std::exit(-1);
                           }

                           delete msg;
                       }
                       file.close();
                   });

    std::string line;
    getline(std::cin, line);
    run = false;
    th.join();
    cs->stop();

    return 0;
}
