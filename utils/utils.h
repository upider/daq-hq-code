#ifndef UTILS_H
#define UTILS_H

#include <string>

namespace message_pass {

class Utils
{
public:
    Utils();
    ~Utils();

    static std::string get_host_ip();
};

}
#endif /* UTILS_H */
