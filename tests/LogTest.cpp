#include <json/json.h>

#include "log/log.h"

using namespace message_pass;

int main()
{
    auto logger = MessageLogger::get_logger("TEST");

    logger->info("logger test hhhhh!");
    SPDLOG_TRACE("logger test hhhhh!");


    // spdlog::set_pattern("[source %s] [function %!] [line %#] %v");
    Json::Value pattern;
    pattern["time"] = "%Y-%m-%d %H:%M:%S.%f";
    pattern["logger"] = "%n";
    pattern["level"] = "%l";
    pattern["process"] = "%P";
    pattern["thread"] = "%t";
    pattern["func"] = "%!";
    pattern["message"] = "%v";
    Json::StreamWriterBuilder builder;
    const std::string json_pattern = Json::writeString(builder, pattern);

    spdlog::set_pattern(json_pattern);
    spdlog::set_level(spdlog::level::trace);

    SPDLOG_TRACE("This trace");
    SPDLOG_DEBUG("This is debug..");
    SPDLOG_INFO("This is info..");
    SPDLOG_ERROR("This is error..");
    SPDLOG_CRITICAL("This is critical..");
    return 0;
}