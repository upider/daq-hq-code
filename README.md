# message pass

## branch benchmark
use tests/MessageSinkTest.cpp and tests/MessageSourceTest.cpp

start bin/MessageSourceTest first then start MessageSinkTest, MessageSinkTest will print
recv speed for 1000000 messages.

## goal
pass data in daq dataflow

## dependencies
1. Boost thread
2. jsoncpp
3. Protobuf
4. spdlog
5. librdkafka
6. zmq

## example
tests

## TODO
- [x] recover and delete
- [x] fixed message size send and recv
- [ ] metrics