#include "network_monitor/stomp_frame_builder.hpp"

#include <string>
#include <utility>

#include "network_monitor/stomp_frame.hpp"

namespace network_monitor {
namespace {

std::string ParseCommand(StompCommand command) {
  return ToString(command) + '\n';
}

std::string ParseHeaders(const StompFrame::Headers& headers) {
  std::string parsed;

  for (const auto [header, value] : headers) {
    parsed.append(ToString(header));
    parsed.append(":");
    value.empty() ? parsed.append("\"\"") : parsed.append(value);
    parsed.append("\n");
  }
  return parsed;
}

}  // namespace

StompFrameBuilder& StompFrameBuilder::SetCommand(StompCommand command) {
  command_ = command;
  return *this;
}

StompFrameBuilder& StompFrameBuilder::AddHeader(StompHeader header,
                                                std::string_view value) {
  headers_.emplace(header, value);
  return *this;
}

StompFrameBuilder& StompFrameBuilder::SetBody(std::string_view body) {
  body_ = body;
  return *this;
}

std::string StompFrameBuilder::BuildString() {
  std::string content;
  content.append(ParseCommand(command_));
  content.append(ParseHeaders(headers_));
  content.push_back('\n');
  content.append(body_);
  content.push_back('\0');

  return content;
}

}  // namespace network_monitor
