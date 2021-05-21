project(MessagePassing)
cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_COMPILER "g++")
set(CMAKE_C_COMPILER "gcc")
# set(CXX_FLAGS -g -Wall -Wextra -pedantic -pedantic-errors -Werror -Wfatal-errors -Wconversion -Wabi-tag -fPIC -std=c++11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -Wall -Wextra -pedantic -pedantic-errors -Wfatal-errors -Wconversion -fPIC -std=c++11")

find_package(spdlog REQUIRED)
if (spdlog_FOUND)
	message(STATUS "spdlog found")
else()
	message(WARNING "spdlog package not found -> specify search path via spdlog variable")
endif()

find_package(jsoncpp REQUIRED)
if (jsoncpp_FOUND)
	message(STATUS "jsoncpp found")
else()
	message(WARNING "jsoncpp package not found -> specify search path via spdlog variable")
endif()

find_package(Boost REQUIRED COMPONENTS thread)
if (Boost_FOUND)
	message(STATUS "Boost found")
	message(STATUS "Boost version : ${Boost_VERSION}")
	message(STATUS "Boost include path : ${Boost_INCLUDE_DIRS}")
	message(STATUS "Boost libraries : ${Boost_LIBRARIES}")
else()
	message(WARNING "Boost package not found -> specify search path via Boost variable")
endif()

set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf REQUIRED)
if (Protobuf_FOUND)
	message(STATUS "Protobuf found")
	message(STATUS "Protobuf version : ${Protobuf_VERSION}")
	message(STATUS "Protobuf include path : ${Protobuf_INCLUDE_DIRS}")
	message(STATUS "Protobuf libraries : ${Protobuf_LIBRARIES}")
else()
	message(WARNING "Protobuf package not found -> specify search path via PROTOBUF_ROOT variable")
endif()

#设置编译完成后so库生成的路径为 libs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${Protobuf_INCLUDE_DIRS})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SOURCES)
link_directories(${PROJECT_SOURCE_DIR}/libs /usr/local/lib64 /usr/local/lib)

add_executable(DataMessageTest tests/DataMessageTest ${SOURCES})
target_link_libraries(DataMessageTest ${Protobuf_LIBRARIES} pthread)

add_executable(RequestMessageTest tests/RequestMessageTest.cpp ${SOURCES})
target_link_libraries(RequestMessageTest ${Protobuf_LIBRARIES} pthread)

add_executable(LogTest tests/LogTest.cpp ${SOURCES})
target_link_libraries(LogTest ${Protobuf_LIBRARIES} jsoncpp pthread)

add_executable(MessageSourceTest tests/MessageSourceTest.cpp ${SOURCES})
target_link_libraries(MessageSourceTest ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} jsoncpp zmq rdkafka++ pthread)

add_executable(MessageSinkTest tests/MessageSinkTest.cpp ${SOURCES})
target_link_libraries(MessageSinkTest ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} jsoncpp zmq rdkafka++ pthread)

add_library(messagepass SHARED ${SOURCES})
