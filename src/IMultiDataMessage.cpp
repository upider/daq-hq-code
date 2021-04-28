#include "message/IMultiDataMessage.h"

namespace message_pass {

IMultiDataMessage::IMultiDataMessage() {}
IMultiDataMessage::IMultiDataMessage(std::size_t buf_num, std::size_t buf_size) {}
IMultiDataMessage::IMultiDataMessage(const std::vector<std::size_t> &buf_size) {}
IMultiDataMessage::~IMultiDataMessage() {}

}