#!/usr/bin/env bash

docker build -t centos:cpp_tool_chain -f docker/dockerfile_cpp_tool_chain .
docker build -t centos:cpp_libraries -f docker/dockerfile_cpp_libraries .
docker build -t message_pass:0.1 -f docker/dockerfile ..