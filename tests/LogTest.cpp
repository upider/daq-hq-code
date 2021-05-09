#include "log/log.h"

using namespace message_pass;

int main()
{
    auto logger = MessageLogger::get_logger("TEST");

    logger->info("logger test hhhhh!");
    return 0;
}