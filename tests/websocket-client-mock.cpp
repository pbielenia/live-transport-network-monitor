#include "websocket-client-mock.hpp"

#include <functional>
#include <queue>
#include <string>
#include <utility>

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include "network-monitor/stomp-frame-builder.hpp"
#include "network-monitor/stomp-frame.hpp"

using namespace network_monitor;

inline boost::system::error_code WebSocketClientMock::connect_error_code{};
inline boost::system::error_code WebSocketClientMock::send_error_code{};
inline boost::system::error_code WebSocketClientMock::close_error_code{};
inline std::queue<std::string> WebSocketClientMock::message_queue{};
inline bool WebSocketClientMock::trigger_disconnection{false};
inline std::function<void(const std::string&)>
    WebSocketClientMock::respond_to_send{[](auto message) { return; }};
inline std::string WebSocketClientMockForStomp::username{};
inline std::string WebSocketClientMockForStomp::password{};
inline std::string WebSocketClientMockForStomp::endpoint{};

WebSocketClientMock::WebSocketClientMock(std::string url,
                                         const std::string& endpoint,
                                         const std::string& port,
                                         boost::asio::io_context& io_context,
                                         boost::asio::ssl::context& tls_context)
    : async_context_{boost::asio::make_strand(io_context)},
      server_url_{std::move(url)} {}

void WebSocketClientMock::Connect(
    std::function<void(boost::system::error_code)> on_connected_callback,
    std::function<void(boost::system::error_code, std::string&&)>
        on_message_callback,
    std::function<void(boost::system::error_code)> on_disconnected_callback) {
  if (connect_error_code.failed()) {
    connected_ = false;
  } else {
    connected_ = true;
    on_message_callback_ = std::move(on_message_callback);
    on_disconnected_callback_ = std::move(on_disconnected_callback);
  }

  boost::asio::post(async_context_, [this, on_connected_callback]() {
    if (on_connected_callback) {
      on_connected_callback(connect_error_code);
    }
  });

  if (connected_) {
    boost::asio::post(async_context_, [this]() { MockIncomingMessages(); });
  }
}

void WebSocketClientMock::Send(
    const std::string& message,
    std::function<void(boost::system::error_code)> on_sent_callback) {
  if (!connected_) {
    boost::asio::post(async_context_, [on_sent_callback]() {
      if (on_sent_callback) {
        on_sent_callback(boost::asio::error::operation_aborted);
      }
    });
    return;
  }

  boost::asio::post(async_context_, [this, on_sent_callback, message]() {
    if (on_sent_callback) {
      on_sent_callback(send_error_code);
      respond_to_send(message);
    }
  });
}

void WebSocketClientMock::Close(
    std::function<void(boost::system::error_code)> on_close_callback) {
  if (connected_) {
    connected_ = false;
    // TODO: closed_ = true;
    trigger_disconnection = true;
    boost::asio::post(async_context_, [this, on_close_callback]() {
      if (on_close_callback) {
        on_close_callback(close_error_code);
      }
    });
  } else {
    boost::asio::post(async_context_, [this, on_close_callback]() {
      if (on_close_callback) {
        on_close_callback(boost::asio::error::operation_aborted);
      }
    });
  }
}

const std::string& WebSocketClientMock::GetServerUrl() const {
  return server_url_;
}

void WebSocketClientMock::MockIncomingMessages() {
  if (!connected_ || trigger_disconnection) {
    trigger_disconnection = false;
    boost::asio::post(async_context_, [this]() {
      if (on_disconnected_callback_) {
        on_disconnected_callback_(boost::asio::error::operation_aborted);
      }
    });
    return;
  }

  boost::asio::post(async_context_, [this]() {
    if (!message_queue.empty()) {
      auto message{message_queue.front()};
      message_queue.pop();
      if (on_message_callback_) {
        on_message_callback_({}, std::move((message)));
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
  respond_to_send = [this](auto message) { OnMessage(message); };
}

void WebSocketClientMockForStomp::OnMessage(const std::string& message) {
  StompFrame frame{message};
  if (frame.GetStompError() != StompError::Ok) {
    // TODO: log
    trigger_disconnection = true;
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
  if (FrameIsValidConnect(frame)) {
    message_queue.push(
        stomp_frame::MakeConnectedFrame(stomp_version, {}, {}, {}).ToString());
  } else {
    message_queue.push(
        stomp_frame::MakeErrorFrame("Authentication failure", {}).ToString());
    trigger_disconnection = true;
  }
}

void WebSocketClientMockForStomp::HandleSubscribeMessage(
    const network_monitor::StompFrame& frame) {
  if (FrameIsValidSubscribe(frame)) {
    auto receipt_id = frame.GetHeaderValue(StompHeader::Receipt);
    auto subscription_id = frame.GetHeaderValue(StompHeader::Id);
    // TODO: add log MockStompServer: __func__: Sending receipt
    message_queue.push(
        stomp_frame::MakeReceiptFrame(std::string{receipt_id}).ToString());
  } else {
    // TODO: add log MockStompServer: __func__: Subscribe
    message_queue.push(stomp_frame::MakeErrorFrame("Subscribe").ToString());
    trigger_disconnection = true;
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
