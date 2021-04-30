#ifndef LOG_H
#define LOG_H

#include <iostream>
#include <ctime>

namespace message_pass
{

//the following are UBUNTU/LINUX ONLY terminal color codes.
#define RESET   "\033[0m"
#define BLACK   "\033[30m"      /* Black */
#define RED     "\033[31m"      /* Red */
#define GREEN   "\033[32m"      /* Green */
#define YELLOW  "\033[33m"      /* Yellow */
#define BLUE    "\033[34m"      /* Blue */
#define MAGENTA "\033[35m"      /* Magenta */
#define CYAN    "\033[36m"      /* Cyan */
#define WHITE   "\033[37m"      /* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

#ifndef LOG_INFO
#define LOG_INFO(X)                                                  \
	{                                                                \
		time_t now = time(0);                                        \
		char *dt = ctime(&now);                                      \
		auto dt_str = std::string(dt);                               \
		dt_str.pop_back();                                           \
		std::cout << dt_str << " \033[32mINFO\033[0m " << X << "\n"; \
	}
#endif
#ifndef LOG_WARN
#define LOG_WARN(X)                                                  \
	{                                                                 \
		time_t now = time(0);                                         \
		char *dt = ctime(&now);                                       \
		auto dt_str = std::string(dt);                                \
		dt_str.pop_back();                                            \
		std::cerr << dt_str << " \033[33mWARN\033[0m " << X << "\n"; \
	}
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(X)                                                  \
	{                                                                 \
		time_t now = time(0);                                         \
		char *dt = ctime(&now);                                       \
		auto dt_str = std::string(dt);                                \
		dt_str.pop_back();                                            \
		std::cerr << dt_str << " \033[31mERROR\033[0m " << X << "\n"; \
	}
#endif

#ifndef LOG_DETAIL_INFO
#define LOG_DETAIL_INFO(X)                                              \
	{                                                                   \
		time_t now = time(0);                                           \
		char *dt = ctime(&now);                                         \
		auto dt_str = std::string(dt);                                  \
		dt_str.pop_back();                                              \
		std::cout << dt_str << "\033[32mINFO\033[0m "                   \
				  << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
				  << X << "\n";                                         \
	}
#endif
#ifndef LOG_DETAIL_WARN
#define LOG_DETAIL_WARN(X)                                             \
	{                                                                   \
		time_t now = time(0);                                           \
		char *dt = ctime(&now);                                         \
		auto dt_str = std::string(dt);                                  \
		dt_str.pop_back();                                              \
		std::cerr << dt_str << " \033[33mWARN\033[0m "                 \
				  << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
				  << X << "\n";                                         \
	}
#endif
#ifndef LOG_DETAIL_ERROR
#define LOG_DETAIL_ERROR(X)                                             \
	{                                                                   \
		time_t now = time(0);                                           \
		char *dt = ctime(&now);                                         \
		auto dt_str = std::string(dt);                                  \
		dt_str.pop_back();                                              \
		std::cerr << dt_str << " \033[31mERROR\033[0m "                 \
				  << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
				  << X << "\n";                                         \
	}
#endif

}

#endif /* LOG_H */