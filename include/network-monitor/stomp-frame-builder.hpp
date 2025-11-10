#pragma once

#include "network-monitor/stomp-frame.hpp"

namespace network_monitor {

namespace stomp_frame {

struct BuildParameters {
  BuildParameters(StompCommand command) : command{command} {}

  StompCommand command;
  // TODO: StompFrame::Headers may be used possibly
  StompFrame::Headers headers;
  std::string body;
};

// TODO: make a version involving less copying
// StompFrame Build(BuildParameters&& parameters);

// TODO: rename to `BuildFrame` or `MakeFrame`
// Remark: Remember to verify the result with StompFrame::GetStompResult().
StompFrame Build(const BuildParameters& parameters);

// Server frames.
StompFrame MakeConnectedFrame(const std::string& version,
                              const std::string& session,
                              const std::string& server,
                              const std::string& heart_beat);
StompFrame MakeErrorFrame(const std::string& message,
                          const std::string& body = {});
StompFrame MakeReceiptFrame(const std::string& receipt_id);
StompFrame MakeMessageFrame(const std::string& destination,
                            const std::string& message_id,
                            const std::string& subscription,
                            const std::string& ack,
                            const std::string& body,
                            const std::string& content_length,
                            const std::string& content_type);
StompFrame MakeSubscribeFrame(const std::string& destination,
                              const std::string& id,
                              const std::string& ack,
                              const std::string& receipt);

}  // namespace stomp_frame
}  // namespace network_monitor
