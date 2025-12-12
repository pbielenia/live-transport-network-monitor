#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "network_monitor/logger.hpp"
#include "network_monitor/stomp_frame.hpp"
#include "network_monitor/stomp_frame_builder.hpp"

namespace network_monitor {

/*! \brief Result codes for the STOMP client.
 */
enum class StompClientResult : std::uint8_t {
  Ok = 0,
  ErrorConnectingWebSocket,
  ErrorConnectingStomp,
  WebSocketServerDisconnected,
  CouldNotSendSubscribeFrame,
  ErrorNotConnected,
  UndefinedError
  // TODO
};

std::string_view ToStringView(StompClientResult result);
std::ostream& operator<<(std::ostream& os, StompClientResult result);

/*! \brief STOMP client implementing the subset of commands needed by the
 *         network-events service.
 *
 *  \param WebSocketClient      WebSocket client class. This type must have the
 *                              same interface of WebSocketClient.
 */
template <typename WebSocketClient>
class StompClient {
 public:
  using OnConnectedCallback = std::function<void(StompClientResult)>;
  using OnDisconnectedCallback = std::function<void(StompClientResult)>;
  using OnClosedCallback = std::function<void(StompClientResult)>;
  using OnSubscribedCallback =
      std::function<void(StompClientResult, std::string&&)>;
  using OnMessageCallback =
      std::function<void(StompClientResult, std::string&&)>;

  /*! \brief Construct a STOMP client connecting to a remote URL/port through a
   *         secure WebSocket connection.
   *
   *  \note This constructor does not initiate a connection.
   *
   *  \param url          The URL of the server.
   *  \param endpoint     The endpoint on the sever to connect.
   *                      Example: ltnm.learncppthroughprojects.com/<endpoint>.
   *  \param port         The port on the server.
   *  \param io_context   The io_context object. The user takes care of calling
   *                      io_context.run().
   *  \param tls_context  The TLS context to setup a TLS socket stream.
   */
  StompClient(const std::string& url,
              const std::string& endpoint,
              const std::string& port,
              boost::asio::io_context& io_context,
              boost::asio::ssl::context& tls_context);

  /*! \brief Connect to the STOMP server.
   *
   *  This method first connects to the WebSocket server, then tries to
   *  establish a STOMP connection with the user credentials.
   *
   *  \param user_name        User name.
   *  \param user_password    User password.
   *  \param on_connected_callback  This handler is called when the STOMP
   *                                connection is setup correctly. It will also
   *                                be called with an error if there is any
   *                                failure before a successful connection.
   *  \param on_disconnected_callback  This handler is called when the STOMP
   *                                   or the WebSocket connection is suddenly
   *                                   closed. In the STOMP protocol, this may
   *                                   happen also in response to bad inputs
   *                                   (authentication, subscription).
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   *  one.
   */
  void Connect(const std::string& user_name,
               const std::string& user_password,
               OnConnectedCallback on_connected_callback = nullptr,
               OnDisconnectedCallback on_disconnected_callback = nullptr);

  /*! \brief Close the STOMP and WebSocket connection.
   *
   *  \param callback  Called when the connection has been closed, successfully
   *                   or not.
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   *  one.
   */
  void Close(OnClosedCallback callback = nullptr);

  /*! \brief Subscribe to a STOMP endpoint.
   *
   *  \returns The subscripton ID, if the subscription process was started
   *           successfully; otherwise, an empty string.
   *
   *  \param destination              The subscription topic.
   *  \param on_subscribed_callback   Called when the subscription is setup
   *                                  correctly. The handler receives an error
   *                                  code and the subscription ID. Note: On
   *                                  failure, this callback is only called if
   *                                  the failure happened at the WebSocket
   *                                  level, not at the STOMP level. This is due
   *                                  to the fact that the STOMP server
   *                                  automatically closes the WebSocket
   *                                  connection on a STOMP protocol failure.
   * \param on_message_callback       Called on every new
   *                                  message from the subscription destination.
   *                                  It is assumed that the message is received
   *                                  with application/json content type.
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   *  one.
   */
  std::string Subscribe(const std::string& destination,
                        OnSubscribedCallback on_subscribed_callback,
                        OnMessageCallback on_message_callback);

 private:
  struct Subscription {
    std::string destination;
    OnSubscribedCallback on_subscribed_callback{nullptr};
    OnMessageCallback on_message_callback{nullptr};
  };

  void OnWebSocketConnected(boost::system::error_code result);
  void ConnectToStompServer();
  void OnStompConnectionInitStarted(boost::system::error_code result);
  void HandleStompFrame(StompFrame frame);
  void OnWebSocketMessageReceived(boost::system::error_code result,
                                  std::string&& message);
  void OnWebSocketMessageSent(boost::system::error_code result);
  void OnWebSocketDisconnected(boost::system::error_code result);
  void OnWebSocketClosed(boost::system::error_code result,
                         OnClosedCallback on_closed_callback = nullptr);
  void OnWebSocketSentSubscribe(boost::system::error_code result,
                                std::string& subscription_id,
                                Subscription&& subscription);

  void HandleStompConnected(const StompFrame& frame);
  void HandleStompReceipt(const StompFrame& frame);
  void HandleStompMessage(const StompFrame& frame);

  void OnConnectingDone(StompClientResult result);

  static std::string GenerateSubscriptionId();

  OnConnectedCallback on_connected_callback_;
  OnDisconnectedCallback on_disconnected_callback_;
  OnClosedCallback on_closed_callback_;
  OnMessageCallback on_message_callback_;

  std::string user_name_;
  std::string user_password_;

  // TODO: rework to not use std::string as a key
  std::unordered_map<std::string, Subscription> subscriptions_{};

  WebSocketClient websocket_client_;
  boost::asio::strand<boost::asio::io_context::executor_type> async_context_;

  bool websocket_connected_{false};
};

template <typename WebSocketClient>
StompClient<WebSocketClient>::StompClient(
    const std::string& url,
    const std::string& endpoint,
    const std::string& port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context)
    : websocket_client_{url, endpoint, port, io_context, tls_context},
      async_context_{boost::asio::make_strand(io_context)} {}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::Connect(
    const std::string& user_name,
    const std::string& user_password,
    OnConnectedCallback on_connected_callback,
    OnDisconnectedCallback on_disconnected_callback) {
  LOG_INFO("Connecting to STOMP server");

  user_name_ = user_name;
  user_password_ = user_password;
  on_connected_callback_ = std::move(on_connected_callback);
  on_disconnected_callback_ = std::move(on_disconnected_callback);

  websocket_client_.Connect(
      [this](auto result) { OnWebSocketConnected(result); },
      [this](auto result, auto&& message) {
        OnWebSocketMessageReceived(result, std::move(message));
      },
      [this](auto result) { OnWebSocketDisconnected(result); });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::Close(OnClosedCallback callback) {
  LOG_INFO("Closing connection");

  if (!websocket_connected_) {
    if (callback) {
      callback(StompClientResult::ErrorNotConnected);
    }
    return;
  }

  // TODO: clear subscriptions

  websocket_client_.Close(
      [this, callback](auto result) { OnWebSocketClosed(result, callback); });
}

template <typename WebSocketClient>
std::string StompClient<WebSocketClient>::Subscribe(
    const std::string& destination,
    OnSubscribedCallback on_subscribed_callback,
    OnMessageCallback on_message_callback) {
  LOG_INFO("Starting subscription to '{}'", destination);

  auto subscription_id{GenerateSubscriptionId()};
  LOG_DEBUG("subscription_id: '{}'", subscription_id);

  // TODO: handle error ocurred when creating a frame
  // TODO: setting subscription_id to Id and Receipt is misleading
  auto stomp_frame{StompFrameBuilder()
                       .SetCommand(StompCommand::Subscribe)
                       .AddHeader(StompHeader::Destination, destination)
                       .AddHeader(StompHeader::Id, subscription_id)
                       .AddHeader(StompHeader::Ack, "auto")
                       .AddHeader(StompHeader::Receipt, subscription_id)
                       .BuildString()};

  Subscription subscription{destination, on_subscribed_callback,
                            on_message_callback};

  // TODO: why mutable? why assign operator instead of just std::move?
  auto on_websocket_sent_callback = [this, subscription_id,
                                     subscription = std::move(subscription)](
                                        auto result) mutable {
    OnWebSocketSentSubscribe(result, subscription_id, std::move(subscription));
  };

  websocket_client_.Send(stomp_frame, on_websocket_sent_callback);

  return subscription_id;
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketConnected(
    boost::system::error_code result) {
  if (result.failed()) {
    LOG_ERROR("Could not connect to STOMP server: {}", result.message());
    OnConnectingDone(StompClientResult::ErrorConnectingWebSocket);
    return;
  }

  websocket_connected_ = true;
  ConnectToStompServer();
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::ConnectToStompServer() {
  const auto connect_frame =
      StompFrameBuilder()
          .SetCommand(StompCommand::Connect)
          .AddHeader(StompHeader::AcceptVersion, "1.2")
          .AddHeader(StompHeader::Host, websocket_client_.GetServerUrl())
          .AddHeader(StompHeader::Login, user_name_)
          .AddHeader(StompHeader::Passcode, user_password_)
          .BuildString();

  websocket_client_.Send(connect_frame, [this](auto result) {
    OnStompConnectionInitStarted(result);
  });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnStompConnectionInitStarted(
    boost::system::error_code result) {
  if (result.failed()) {
    LOG_ERROR("Could not send STOMP frame: {}", result.message());
    OnConnectingDone(StompClientResult::ErrorConnectingStomp);
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketMessageReceived(
    boost::system::error_code result, std::string&& message) {
  if (result.failed()) {
    LOG_WARN("Receiving message failed");
    // TODO: handle
    return;
  }

  const auto frame = StompFrame(std::move(message));
  if (frame.GetParseResultCode() != ParseResultCode::Ok) {
    LOG_WARN("Could not parse the message to STOMP frame: {}",
             ToString(frame.GetParseResultCode()));
    return;
  }

  HandleStompFrame(std::move(frame));
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompFrame(StompFrame frame) {
  LOG_DEBUG("Received STOMP frame: '{}'", ToString(frame.GetCommand()));

  switch (frame.GetCommand()) {
    case StompCommand::Connected:
      HandleStompConnected(frame);
      break;
    case StompCommand::Receipt:
      HandleStompReceipt(frame);
      break;
    case StompCommand::Message:
      HandleStompMessage(frame);
    default: {
      // TODO: handle the rest of commands
      LOG_ERROR("Unexpected STOMP command: '{}'", ToString(frame.GetCommand()));
      break;
    }
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketMessageSent(
    boost::system::error_code result) {
  //
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketDisconnected(
    boost::system::error_code result) {
  LOG_INFO("WebSocket disconnected: {}", result.message());

  websocket_connected_ = false;

  if (on_disconnected_callback_) {
    auto error{result ? StompClientResult::WebSocketServerDisconnected
                      : StompClientResult::Ok};
    boost::asio::post(async_context_,
                      [this, error]() { on_disconnected_callback_(error); });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketClosed(
    boost::system::error_code result, OnClosedCallback on_closed_callback) {
  LOG_INFO("Connection closed");

  if (on_closed_callback) {
    boost::asio::post(
        async_context_, [callback = std::move(on_closed_callback), &result]() {
          callback(result.failed() ? StompClientResult::UndefinedError
                                   : StompClientResult::Ok);
        });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketSentSubscribe(
    boost::system::error_code result,
    std::string& subscription_id,
    Subscription&& subscription) {
  if (result.failed()) {
    LOG_WARN("Could not subscribe to '{}': ", subscription.destination,
             result.message());
    if (subscription.on_subscribed_callback) {
      boost::asio::post(
          async_context_,
          [on_subscribe = subscription.on_subscribed_callback]() {
            on_subscribe(StompClientResult::CouldNotSendSubscribeFrame, {});
          });
    }
    return;
  }

  subscriptions_.emplace(subscription_id, std::move(subscription));
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompConnected(
    const StompFrame& /*frame*/) {
  LOG_INFO("Connected to STOMP server");
  OnConnectingDone(StompClientResult::Ok);
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompReceipt(const StompFrame& frame) {
  // Supports only SUBSCRIBE frame.
  auto subscription_id =
      std::string(frame.GetHeaderValue(StompHeader::ReceiptId));
  if (!subscriptions_.contains(subscription_id)) {
    LOG_WARN("Unknown subscription id: '{}'", subscription_id);
    return;
  }

  const auto& subscription = subscriptions_.at(subscription_id);
  LOG_INFO("Subscribed to '{}'", subscription_id);

  if (subscription.on_subscribed_callback) {
    boost::asio::post(async_context_,
                      [&subscription,
                       subscription_id = std::move(subscription_id)]() mutable {
                        subscription.on_subscribed_callback(
                            StompClientResult::Ok, std::move(subscription_id));
                      });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompMessage(const StompFrame& frame) {
  // Find the subscription
  auto destination = frame.GetHeaderValue(StompHeader::Destination);
  auto message_id = frame.GetHeaderValue(StompHeader::MessageId);
  auto subscription_id =
      std::string(frame.GetHeaderValue(StompHeader::Subscription));

  if (destination.empty() || message_id.empty() || subscription_id.empty()) {
    LOG_WARN("Required fields are missing");
    return;
  }

  if (!subscriptions_.contains(subscription_id)) {
    LOG_WARN("Unknown subscription id: '{}'", subscription_id);
    return;
  }
  auto& subscription = subscriptions_.at(subscription_id);

  if (subscription.destination != destination) {
    LOG_WARN(
        "message.destination does no match subscription.destination: '{}' and "
        "'{}'",
        subscription.destination, destination);
    return;
  }

  // TODO: extract body from StompFrame

  if (subscription.on_message_callback) {
    boost::asio::post(
        async_context_,
        [&subscription, message_body = std::string(frame.GetBody())]() {
          subscription.on_message_callback(StompClientResult::Ok,
                                           std::string(message_body));
        });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnConnectingDone(StompClientResult result) {
  if (on_connected_callback_) {
    boost::asio::post(async_context_, [callback = on_connected_callback_,
                                       result]() { callback(result); });
  }
}

template <typename WebSocketClient>
std::string StompClient<WebSocketClient>::GenerateSubscriptionId() {
  std::stringstream output{};
  output << boost::uuids::random_generator()();
  return output.str();
}

}  // namespace network_monitor
