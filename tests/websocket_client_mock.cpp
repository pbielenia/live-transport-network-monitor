#include "websocket_client_mock.hpp"

#include <functional>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/system/errc.hpp>
#include <boost/system/error_code.hpp>

#include "network_monitor/stomp_frame.hpp"
#include "network_monitor/stomp_frame_builder.hpp"

namespace network_monitor {

inline boost::system::error_code
    WebSocketClientMock::Config::send_error_code_{};
inline boost::system::error_code
    WebSocketClientMock::Config::close_error_code_{};
inline bool WebSocketClientMock::Config::trigger_disconnection_{false};
inline bool WebSocketClientMock::Config::fail_connecting_websocket_{false};
inline bool WebSocketClientMock::Config::fail_sending_message_{false};
inline std::function<void(const std::string&)>
    WebSocketClientMock::Config::on_message_sent_{
        [](auto /*message*/) { return; }};

inline std::function<std::string()>
    WebSocketClientMockForStomp::Responses::on_frame_connect{};

inline std::string WebSocketClientMock::Results::url{};
inline std::string WebSocketClientMock::Results::endpoint{};
inline std::string WebSocketClientMock::Results::port{};
inline std::vector<std::string>
    WebSocketClientMock::Results::messages_sent_to_websocket_client{};

inline std::string WebSocketClientMockForStomp::username{};
inline std::string WebSocketClientMockForStomp::password{};
inline std::string WebSocketClientMockForStomp::endpoint{};

WebSocketClientMock::WebSocketClientMock(
    std::string url,
    std::string endpoint,
    std::string port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& /*tls_context*/)
    : async_context_{boost::asio::make_strand(io_context)} {
  Results::url = std::move(url);
  Results::endpoint = std::move(endpoint);
  Results::port = std::move(port);
}

void WebSocketClientMock::Connect(
    WebSocketClientMock::OnConnectingDoneCallback on_connecting_done_callback,
    WebSocketClientMock::OnMessageReceivedCallback on_message_received_callback,
    OnDisconnectedCallback on_disconnected_callback) {
  const auto error_code =
      Config::fail_connecting_websocket_
          ? boost::asio::ssl::error::stream_truncated
          : boost::system::errc::make_error_code(boost::system::errc::success);

  connected_ = !error_code.failed();
  on_message_received_callback_ = std::move(on_message_received_callback);
  on_disconnected_callback_ = std::move(on_disconnected_callback);

  if (on_connecting_done_callback) {
    boost::asio::post(async_context_,
                      [this, callback = std::move(on_connecting_done_callback),
                       error_code]() { callback(error_code); });
  }
}

void WebSocketClientMock::Send(const std::string& message,
                               OnMessageSentCallback on_message_sent_callback) {
  if (!connected_ || Config::fail_sending_message_) {
    if (on_message_sent_callback) {
      boost::asio::post(async_context_,
                        [callback = std::move(on_message_sent_callback)]() {
                          callback(boost::asio::error::operation_aborted);
                        });
    }
    return;
  }

  Results::messages_sent_to_websocket_client.push_back(message);

  boost::asio::post(
      async_context_,
      [this, callback = std::move(on_message_sent_callback), message]() {
        if (callback) {
          callback(Config::send_error_code_);
        }
        Config::on_message_sent_(message);
      });
}

void WebSocketClientMock::Close(
    OnConnectionClosedCallback on_connection_closed_callback) {
  connected_ = false;

  if (!on_connection_closed_callback) {
    return;
  }

  const auto result = connected_ ? Config::close_error_code_
                                 : boost::asio::error::operation_aborted;
  boost::asio::post(
      async_context_,
      [this, result, callback = std::move(on_connection_closed_callback)]() {
        callback(result);
      });
}

const std::string& WebSocketClientMock::GetServerUrl() const {
  return Results::url;
}

void WebSocketClientMock::Disconnect() {
  connected_ = false;

  boost::asio::post(async_context_, [this]() {
    if (on_disconnected_callback_) {
      on_disconnected_callback_(boost::asio::error::operation_aborted);
    }
  });
}

void WebSocketClientMock::SendToWebSocketClient(std::string message) {
  if (!on_message_received_callback_) {
    return;
  }

  boost::asio::post(async_context_,
                    [this, message = std::move(message),
                     callback = on_message_received_callback_]() mutable {
                      callback(std::move((message)));
                    });
}

WebSocketClientMockForStomp::WebSocketClientMockForStomp(
    const std::string& url,
    const std::string& endpoint,
    const std::string& port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context)
    : WebSocketClientMock(url, endpoint, port, io_context, tls_context) {
  Config::on_message_sent_ = [this](const auto& message) {
    OnMessageSentToStompServer(message);
  };
}

void WebSocketClientMockForStomp::OnMessageSentToStompServer(
    const std::string& message) {
  const auto frame = StompFrame{message};
  if (frame.GetParseResultCode() != ParseResultCode::Ok) {
    // TODO: log
    // TODO: send error message?
    Disconnect();
    return;
  }

  // TODO: log frame command
  switch (frame.GetCommand()) {
    case StompCommand::Stomp:
    case StompCommand::Connect:
      HandleConnectMessage(frame);
      break;
    case StompCommand::Subscribe: {
      HandleSubscribeMessage(frame);
      break;
    }
    default: {
      // TODO: log not supported frame
      break;
    }
  }
}

void WebSocketClientMockForStomp::HandleConnectMessage(
    const network_monitor::StompFrame& frame) {
  if (!FrameIsValidConnect(frame)) {
    SendToWebSocketClient(StompFrameBuilder()
                              .SetCommand(StompCommand::Error)
                              .SetBody("Authentication failure")
                              .BuildString());
    Disconnect();
    return;
  }

  SendToWebSocketClient(
      Responses::on_frame_connect
          ? Responses::on_frame_connect()
          : StompFrameBuilder()
                .SetCommand(StompCommand::Connected)
                .AddHeader(StompHeader::Version, stomp_version)
                .BuildString());
}

void WebSocketClientMockForStomp::HandleSubscribeMessage(
    const network_monitor::StompFrame& frame) {
  if (!FrameIsValidSubscribe(frame)) {
    // TODO: add log MockStompServer: __func__: Subscribe
    SendToWebSocketClient(StompFrameBuilder()
                              .SetCommand(StompCommand::Error)
                              .SetBody("Subscribe")
                              .BuildString());
    Disconnect();
    return;
  }

  auto receipt_id = frame.GetHeaderValue(StompHeader::Receipt);
  auto subscription_id = frame.GetHeaderValue(StompHeader::Id);
  // TODO: add log MockStompServer: __func__: Sending receipt
  SendToWebSocketClient(StompFrameBuilder()
                            .SetCommand(StompCommand::Receipt)
                            .AddHeader(StompHeader::ReceiptId, receipt_id)
                            .BuildString());
}

bool WebSocketClientMockForStomp::FrameIsValidConnect(
    const network_monitor::StompFrame& frame) {
  if (!frame.HasHeader(StompHeader::Login) ||
      !frame.HasHeader(StompHeader::Passcode)) {
    return false;
  }
  return frame.GetHeaderValue(StompHeader::Login) == username &&
         frame.GetHeaderValue(StompHeader::Passcode) == password;
}

bool WebSocketClientMockForStomp::FrameIsValidSubscribe(
    const network_monitor::StompFrame& frame) {
  return (frame.GetHeaderValue(StompHeader::Destination) == endpoint &&
          !frame.GetHeaderValue(StompHeader::Receipt).empty() &&
          !frame.GetHeaderValue(StompHeader::Id).empty());
}

}  // namespace network_monitor
