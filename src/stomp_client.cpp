#include "network_monitor/stomp_client.hpp"

#include <array>
#include <ostream>
#include <string_view>

namespace network_monitor {

constexpr auto kStompClientResultStrings =
    std::array<std::pair<StompClientResult, std::string_view>, 9>{{
        {StompClientResult::Ok, "Ok"},
        {StompClientResult::CouldNotConnectToWebSocketServer,
         "CouldNotConnectToWebSocketServer"},
        {StompClientResult::UnexpectedCouldNotCreateValidFrame,
         "UnexpectedCouldNotCreateValidFrame"},
        {StompClientResult::CouldNotSendConnectFrame,
         "CouldNotSendConnectFrame"},
        {StompClientResult::CouldNotParseMessageAsStompFrame,
         "CouldNotParseMessageAsStompFrame"},
        {StompClientResult::CouldNotCloseWebSocketConnection,
         "CouldNotCloseWebSocketConnection"},
        {StompClientResult::WebSocketServerDisconnected,
         "WebSocketServerDisconnected"},
        {StompClientResult::CouldNotSendSubscribeFrame,
         "CouldNotSendSubscribeFrame"},
        {StompClientResult::UndefinedError, "UndefinedError"},
    }};

std::string_view ToStringView(StompClientResult result) {
  for (auto [key, value] : kStompClientResultStrings) {
    if (key == result) {
      return value;
    }
  }

  std::unreachable();
}

std::ostream& operator<<(std::ostream& os, StompClientResult result) {
  os << ToStringView(result);
  return os;
}

}  // namespace network_monitor
