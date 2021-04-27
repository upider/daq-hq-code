#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>

namespace message_pass
{

#ifndef LOG_INFO
#define LOG_INFO(X)                                      \
	{                                                    \
		time_t now = time(0);                            \
		char *dt = ctime(&now);                          \
		auto dt_str = std::string(dt);                   \
		dt_str.pop_back();                               \
		std::cout << dt_str << " | "                     \
				  << "\033[32mINFO | " << X << "\033[0m" \
				  << "\n";                               \
	}
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(X)                                      \
	{                                                     \
		time_t now = time(0);                             \
		char *dt = ctime(&now);                           \
		auto dt_str = std::string(dt);                    \
		dt_str.pop_back();                                \
		std::cerr << dt_str << " | "                          \
				  << "\033[31mERROR | " << X << "\033[0m" \
				  << "\n";                                \
	}
#endif
#ifndef LOG_DETAIL_INFO
#define LOG_DETAIL_INFO(X)                                                                          \
	{                                                                                               \
		time_t now = time(0);                                                                       \
		char *dt = ctime(&now);                                                                     \
		auto dt_str = std::string(dt);                                                              \
		dt_str.pop_back();                                                                          \
		std::cout << dt_str << " | "                                                                    \
				  << "\033[32mINFO | \033[0m" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
				  << " | "                                                                          \
				  << "\033[32m" << X << "\033[0m"                                                   \
				  << "\n";                                                                          \
	}
#endif
#ifndef LOG_DETAIL_ERROR
#define LOG_DETAIL_ERROR(X)                                                                          \
	{                                                                                                \
		time_t now = time(0);                                                                        \
		char *dt = ctime(&now);                                                                      \
		auto dt_str = std::string(dt);                                                               \
		dt_str.pop_back();                                                                           \
		std::cerr << dt_str << " | "                                                                     \
				  << "\033[31mERROR | \033[0m" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
				  << " | "                                                                           \
				  << "\033[31m" << X << "\033[0m"                                                    \
				  << "\n";                                                                           \
	}
#endif

}

#endif /* LOG_H */