#include <fstream>

#include "message_sink/message_sink.h"
#include "utils/cmdline.h"

#include "TestDataMessage.h"

using namespace message_pass;
using namespace std;

vector<string> split(const string &s, const string &seperator)
{
    vector<string> result;
    typedef string::size_type string_size;
    string_size i = 0;

    while (i != s.size())
    {
        //找到字符串中首个不等于分隔符的字母；
        int flag = 0;
        while (i != s.size() && flag == 0)
        {
            flag = 1;
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[i] == seperator[x])
                {
                    ++i;
                    flag = 0;
                    break;
                }
        }

        //找到又一个分隔符，将两个分隔符之间的字符串取出；
        flag = 0;
        string_size j = i;
        while (j != s.size() && flag == 0)
        {
            for (string_size x = 0; x < seperator.size(); ++x)
                if (s[j] == seperator[x])
                {
                    flag = 1;
                    break;
                }
            if (flag == 0)
                ++j;
        }
        if (i != j)
        {
            result.push_back(s.substr(i, j - i));
            i = j;
        }
    }
    return result;
}

// 接收10次消息发送一次deleteRequest，接收第60个消息后直接挂掉，
// 检验恢复后能否把未删除的数据再拿到
int main(int argc, char *argv[])
{
    cmdline::parser cmdline_parser;
    cmdline_parser.add<std::string>("ip", 0, "server ip address");
    cmdline_parser.add<int>("port", 0, "server port");
    cmdline_parser.add<std::string>("topic_server", 0, "topic server address (kafka broker list)");
    cmdline_parser.add<std::string>("topic", 0, "subscribe topic");
    cmdline_parser.add<int>("io_threads", 0, "threads number for zmq");
    cmdline_parser.add<std::string>("sources", 0, "message source identity, source1,source2...");
    cmdline_parser.add<bool>("fault", true, "是否崩溃测试");
    cmdline_parser.parse_check(argc, argv);

    std::string topic = cmdline_parser.get<std::string>("topic");
    std::string source = cmdline_parser.get<std::string>("sources");
    bool fault = cmdline_parser.get<bool>("fault");

    std::shared_ptr<MessageSink<TestDataMessage>>
        cs(new MessageSink<TestDataMessage>(
            cmdline_parser.get<std::string>("ip"),
            cmdline_parser.get<int>("port"),
            cmdline_parser.get<std::string>("topic_server"),
            {topic},
            cmdline_parser.get<int>("io_threads")));

    //提前准备好source
    std::vector<std::string> sources = split(source, ",");
    for(auto source : sources)
    {
        std::cout << "prepare source : " << source << std::endl;
    }
    if (!sources.empty())
    {
        cs->prepare_sources(topic, sources);
    }

    //start 必须在send request之前调用
    cs->start();

    std::atomic_bool run{true};
    std::thread th([&run, cs, topic, sources, fault]()
                   {
                       int i = 0;
                       std::vector<std::size_t> delKeys(10);
                       std::ofstream file;
                       file.open("./recv", ios::out);
                       while (run)
                       {
                           TestDataMessage *msg;

                           cs->send_get_request(topic);

                           bool ret;
                           for (auto source : sources)
                           {
                               do
                               {
                                   ret = cs->get_msg(topic, source, 50, &msg);
                               } while (!ret && run);
                               file << "source ==> " << source << ", key ==> " << msg->key() << "\n";
                               file.flush();
                           }

                           i++;

                           delKeys.push_back(msg->key());
                           if (i == 60 && fault)
                           {
                               SPDLOG_ERROR("FAULT HAPPEND");
                               std::exit(-1);
                           }
                           if (i % 10 == 0)
                           {
                               cs->send_del_request(topic, delKeys);
                               delKeys.clear();
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
