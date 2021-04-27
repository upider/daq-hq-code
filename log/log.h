#ifndef LOG_H
#define LOG_H

#include <iostream>

#ifndef LOG_INFO
#define LOG_INFO(X) \
	std::cout << "\033[32mINFO | " << X << "\033[0m" << "\n";
#endif
#ifndef LOG_ERROR
#define LOG_ERROR(X) \
	std::cerr << "\033[31mERROR | " << X << "\033[0m" << "\n";
#endif
#ifndef LOG_DETAIL_INFO
#define LOG_DETAIL_INFO(X) \
	std::cout << "\033[32mINFO | \033[0m" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
			  << " | " << "\033[32m" << X << "\033[0m" << "\n";
#endif
#ifndef LOG_DETAIL_ERROR
#define LOG_DETAIL_ERROR(X) \
	std::cerr << "\033[31mERROR | \033[0m" << __FILE__ << ":" << __LINE__ << ":" << __FUNCTION__ \
			  << " | " << "\033[31m" << X << "\033[0m" << "\n";
#endif

#endif /* LOG_H */