#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "utils/utils.h"

namespace message_pass
{

Utils::Utils()
{
}

Utils::~Utils()
{
}

std::string Utils::get_host_ip() {
    char hostname[256];
    struct hostent *hostent_addr;

    if (gethostname(hostname, 255) == -1) {
        return "";
    }

    if ((hostent_addr = gethostbyname(hostname)) == NULL) {
        return "";
    }

    return std::string(inet_ntoa(*((struct in_addr *)hostent_addr->h_addr_list[0])));
}

}