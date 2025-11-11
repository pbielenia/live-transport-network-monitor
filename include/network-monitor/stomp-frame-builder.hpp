#pragma once

#include <string>
#include <string_view>

#include "network-monitor/stomp-frame.hpp"

namespace network_monitor {

class StompFrameBuilder {
 public:
  StompFrameBuilder& SetCommand(StompCommand command);
  StompFrameBuilder& AddHeader(StompHeader header, std::string_view value);
  StompFrameBuilder& SetBody(std::string_view body);

  std::string BuildString();

 private:
  StompCommand command_;
  StompFrame::Headers headers_;
  std::string body_;
};

StompFrame MakeFrameConnected(std::string_view version,
                              std::string_view session,
                              std::string_view server,
                              std::string_view heart_beat);
StompFrame MakeFrameError(std::string_view message, std::string_view body = {});
StompFrame MakeFrameReceipt(std::string_view receipt_id);
StompFrame MakeFrameMessage(std::string_view destination,
                            std::string_view message_id,
                            std::string_view subscription,
                            std::string_view ack,
                            std::string_view body,
                            std::string_view content_length,
                            std::string_view content_type);
StompFrame MakeFrameSubscribe(std::string_view destination,
                              std::string_view id,
                              std::string_view ack,
                              std::string_view receipt);

}  // namespace network_monitor
