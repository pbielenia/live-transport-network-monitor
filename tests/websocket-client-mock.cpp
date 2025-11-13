#include "websocket-client-mock.hpp"

#include <functional>
#include <queue>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "network-monitor/stomp-frame-builder.hpp"
#include "network-monitor/stomp-frame.hpp"

namespace network_monitor {

inline boost::system::error_code
    WebSocketClientMock::Config::connect_error_code_{};
inline boost::system::error_code
    WebSocketClientMock::Config::send_error_code_{};
inline boost::system::error_code
    WebSocketClientMock::Config::close_error_code_{};
inline bool WebSocketClientMock::Config::trigger_disconnection_{false};
inline std::function<void(const std::string&)>
    WebSocketClientMock::Config::respond_to_send_{
        [](auto /*message*/) { return; }};
inline std::string WebSocketClientMockForStomp::username{};
inline std::string WebSocketClientMockForStomp::password{};
inline std::string WebSocketClientMockForStomp::endpoint{};

WebSocketClientMock::WebSocketClientMock(
    std::string url,
    std::string /*endpoint*/,
    std::string /*port*/,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& /*tls_context*/)
    : async_context_{boost::asio::make_strand(io_context)},
      server_url_{std::move(url)} {}

std::queue<std::string>& WebSocketClientMock::GetMessageQueue() {
  static std::queue<std::string> message_queue;
  return message_queue;
}

void WebSocketClientMock::Connect(
    WebSocketClientMock::kOnConnectedCallback on_connected_callback,
    WebSocketClientMock::kOnMessageReceivedCallback
        on_message_received_callback,
    kOnDisconnectedCallback on_disconnected_callback) {
  if (Config::connect_error_code_.failed()) {
    connected_ = false;
  } else {
    connected_ = true;
    on_message_received_callback_ = std::move(on_message_received_callback);
    on_disconnected_callback_ = std::move(on_disconnected_callback);
  }

  boost::asio::post(async_context_,
                    [this, callback = std::move(on_connected_callback)]() {
                      if (callback) {
                        callback(Config::connect_error_code_);
                      }
                    });

  if (connected_) {
    boost::asio::post(async_context_, [this]() { MockIncomingMessages(); });
  }
}

void WebSocketClientMock::Send(
    const std::string& message,
    kOnMessageSentCallback on_message_sent_callback) {
  if (!connected_) {
    boost::asio::post(async_context_,
                      [callback = std::move(on_message_sent_callback)]() {
                        if (callback) {
                          callback(boost::asio::error::operation_aborted);
                        }
                      });
    return;
  }

  boost::asio::post(
      async_context_,
      [this, callback = std::move(on_message_sent_callback), message]() {
        if (callback) {
          callback(Config::send_error_code_);
          Config::respond_to_send_(message);
        }
      });
}

void WebSocketClientMock::Close(
    kOnConnectionClosedCallback on_connection_closed_callback) {
  if (connected_) {
    connected_ = false;
    // TODO: closed_ = true;
    Config::trigger_disconnection_ = true;
    boost::asio::post(
        async_context_,
        [this, callback = std::move(on_connection_closed_callback)]() {
          if (callback) {
            callback(Config::close_error_code_);
          }
        });
  } else {
    boost::asio::post(
        async_context_,
        [this, callback = std::move(on_connection_closed_callback)]() {
          if (callback) {
            callback(boost::asio::error::operation_aborted);
          }
        });
  }
}

const std::string& WebSocketClientMock::GetServerUrl() const {
  return server_url_;
}

void WebSocketClientMock::MockIncomingMessages() {
  if (!connected_ || Config::trigger_disconnection_) {
    Config::trigger_disconnection_ = false;
    boost::asio::post(async_context_, [this]() {
      if (on_disconnected_callback_) {
        on_disconnected_callback_(boost::asio::error::operation_aborted);
      }
    });
    return;
  }

  boost::asio::post(async_context_, [this]() {
    if (!GetMessageQueue().empty()) {
      auto message{GetMessageQueue().front()};
      GetMessageQueue().pop();
      if (on_message_received_callback_) {
        on_message_received_callback_({}, std::move((message)));
      }
    }
    MockIncomingMessages();
  });
}

WebSocketClientMockForStomp::WebSocketClientMockForStomp(
    const std::string& url,
    const std::string& endpoint,
    const std::string& port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context)
    : WebSocketClientMock(url, endpoint, port, io_context, tls_context) {
  Config::respond_to_send_ = [this](const auto& message) {
    OnMessage(message);
  };
}

void WebSocketClientMockForStomp::OnMessage(const std::string& message) {
  StompFrame frame{message};
  if (frame.GetStompError() != StompError::Ok) {
    // TODO: log
    Config::trigger_disconnection_ = true;
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
    const network_monitor::StompFrame& frame) const {
  if (FrameIsValidConnect(frame)) {
    GetMessageQueue().push(StompFrameBuilder()
                               .SetCommand(StompCommand::Connected)
                               .AddHeader(StompHeader::Version, stomp_version)
                               .BuildString());
  } else {
    GetMessageQueue().push(StompFrameBuilder()
                               .SetCommand(StompCommand::Error)
                               .SetBody("Authentication failure")
                               .BuildString());
    Config::trigger_disconnection_ = true;
  }
}

void WebSocketClientMockForStomp::HandleSubscribeMessage(
    const network_monitor::StompFrame& frame) {
  if (FrameIsValidSubscribe(frame)) {
    auto receipt_id = frame.GetHeaderValue(StompHeader::Receipt);
    auto subscription_id = frame.GetHeaderValue(StompHeader::Id);
    // TODO: add log MockStompServer: __func__: Sending receipt
    GetMessageQueue().push(StompFrameBuilder()
                               .SetCommand(StompCommand::Receipt)
                               .AddHeader(StompHeader::ReceiptId, receipt_id)
                               .BuildString());
  } else {
    // TODO: add log MockStompServer: __func__: Subscribe
    GetMessageQueue().push(StompFrameBuilder()
                               .SetCommand(StompCommand::Error)
                               .SetBody("Subscribe")
                               .BuildString());
    Config::trigger_disconnection_ = true;
  }
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
