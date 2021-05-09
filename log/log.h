#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>

#include <json/json.h>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>

namespace message_pass
{

class MessageLogger {
	public:
		MessageLogger() {}
		~MessageLogger() {}

		static auto get_logger(const std::string& logger_name) {
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
			return spdlog::stdout_color_mt(logger_name);
		}
};

}

#endif /* LOG_H */