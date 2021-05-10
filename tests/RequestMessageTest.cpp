#include "message/request_message.pb.h"

using namespace message_pass;

int main()
{
    RequestMessage rmsg;
    rmsg.set_cmd(RequestMessage_CMD_GET);
    rmsg.set_sink("centos201");
    std::cout << rmsg.SerializeAsString() << "\n";

    std::string str_rmsg;
    rmsg.SerializeToString(&str_rmsg);
    std::cout << str_rmsg << "\n";

    RequestMessage rmsg2;
    rmsg2.ParseFromString(str_rmsg);
    std::cout << rmsg2.cmd() << "\n";
    std::cout << rmsg2.sink() << "\n";

    rmsg2 = rmsg;
    return 0;
}
