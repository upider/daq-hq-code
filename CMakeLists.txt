project(MessagePassing)
cmake_minimum_required(VERSION 3.10)

#add_definitions(-Wall -std=c++11 -rdynamic)
# set(CMAKE_CXX_FLAGS "-std=c++11")
# easyloggingpp macros
add_compile_definitions(AUTO_INITIALIZE_EASYLOGGINGPP)
SET(CMAKE_CXX_COMPILER "g++")
SET(CMAKE_C_COMPILER "gcc")
set(CXX_FLAGS -g -Wall -Wextra -Wconversion -std=c++11)

find_package(Boost REQUIRED COMPONENTS thread)
if (Boost_FOUND)
    message(STATUS "Boost version : ${Boost_VERSION}")
    message(STATUS "Boost include path : ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost libraries : ${Boost_LIBRARIES}")
else()
    message(WARNING "Boost package not found -> specify search path via Boost variable")
endif()

set(Protobuf_USE_STATIC_LIBS ON)
find_package(Protobuf REQUIRED)
if (Protobuf_FOUND)
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
link_directories(${PROJECT_SOURCE_DIR}/libs /usr/lib64)

add_executable(IDataMessageTest tests/IDataMessageTest ${SOURCES})
target_link_libraries(IDataMessageTest ${Protobuf_LIBRARIES} pthread)

add_executable(RequestMessageTest tests/RequestMessageTest.cpp ${SOURCES})
target_link_libraries(RequestMessageTest ${Protobuf_LIBRARIES} pthread)

add_executable(ProducerTest tests/ProducerTest.cpp ${SOURCES})
target_link_libraries(ProducerTest ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} zmq rdkafka++ pthread)

add_executable(ConsuemrTest tests/ConsuemrTest.cpp ${SOURCES})
target_link_libraries(ConsuemrTest ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} zmq rdkafka++ pthread)

add_library(messagepass SHARED ${SOURCES})
target_link_libraries(ProducerTest ${Boost_LIBRARIES} ${Protobuf_LIBRARIES} zmq rdkafka++ pthread)