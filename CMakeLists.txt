project(MessagePassing)
cmake_minimum_required(VERSION 3.10)

#add_definitions(-Wall -std=c++11 -rdynamic)
add_definitions(-std=c++11 -rdynamic -g)
find_package(Boost REQUIRED COMPONENTS thread)

#设置编译完成后so库生成的路径为 libs
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/libs/)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/bin/)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${Boost_INCLUDE_DIRS})
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRCS)
link_directories(${PROJECT_SOURCE_DIR}/libs)

add_executable(ProducerTest tests/ProducerTest.cpp ${SRCS})
target_link_libraries(ProducerTest ${Boost_LIBRARIES} zmq rdkafka++ pthread)
