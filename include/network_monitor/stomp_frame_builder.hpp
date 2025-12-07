#pragma once

#include <string>
#include <string_view>

#include "network_monitor/stomp_frame.hpp"

namespace network_monitor {

class StompFrameBuilder {
 public:
  StompFrameBuilder& SetCommand(StompCommand command);
  StompFrameBuilder& AddHeader(StompHeader header, std::string_view value);
  StompFrameBuilder& SetBody(std::string_view body);

  std::string BuildString();

 private:
  StompCommand command_{StompCommand::Invalid};
  StompFrame::Headers headers_;
  std::string body_;
};

}  // namespace network_monitor
