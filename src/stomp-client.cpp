#include "network-monitor/stomp-client.hpp"

#include <array>
#include <ostream>
#include <string_view>

namespace network_monitor {

constexpr std::array<std::pair<StompClientError, std::string_view>, 9>
    kStompClientErrorStrings{{
        {StompClientError::Ok, "Ok"},
        {StompClientError::CouldNotConnectToWebSocketServer,
         "CouldNotConnectToWebSocketServer"},
        {StompClientError::UnexpectedCouldNotCreateValidFrame,
         "UnexpectedCouldNotCreateValidFrame"},
        {StompClientError::CouldNotSendConnectFrame,
         "CouldNotSendConnectFrame"},
        {StompClientError::CouldNotParseMessageAsStompFrame,
         "CouldNotParseMessageAsStompFrame"},
        {StompClientError::CouldNotCloseWebSocketConnection,
         "CouldNotCloseWebSocketConnection"},
        {StompClientError::WebSocketServerDisconnected,
         "WebSocketServerDisconnected"},
        {StompClientError::CouldNotSendSubscribeFrame,
         "CouldNotSendSubscribeFrame"},
        {StompClientError::UndefinedError, "UndefinedError"},
    }};

std::string_view ToStringView(StompClientError command) {
  for (auto [key, value] : kStompClientErrorStrings) {
    if (key == command) {
      return value;
    }
  }

  return "UndefinedError";
}

std::ostream& operator<<(std::ostream& os, StompClientError error) {
  os << ToStringView(error);
  return os;
}

}  // namespace network_monitor
