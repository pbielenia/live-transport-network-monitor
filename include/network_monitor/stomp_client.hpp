#pragma once

#include <algorithm>
#include <cstdint>
#include <functional>
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
  using OnConnectingDoneCallback = std::function<void(StompClientResult)>;
  using OnDisconnectedCallback = std::function<void(StompClientResult)>;
  using OnClosedCallback = std::function<void(StompClientResult)>;
  using OnSubscribedCallback =
      std::function<void(StompClientResult, std::string)>;
  using OnReceivedCallback =
      std::function<void(StompClientResult, std::string)>;

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
   *  \param on_connecting_done_callback  This handler is called when the STOMP
   *                                      connection is setup correctly. It will
   *                                      also be called with an error if there
   *                                      is any failure before a successful
   *                                      connection.
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
               OnConnectingDoneCallback on_connecting_done_callback = nullptr,
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
   *                                  code and the subscription ID.
   *                                  Note: On failure, this callback is only
   *                                        called if the failure happened at
   *                                        the WebSocket level, not at the
   *                                        STOMP level. This is due to the fact
   *                                        that the STOMP server automatically
   *                                        closes the WebSocket connection on a
   *                                        STOMP protocol failure.
   * \param on_received_callback      Called on every new message from the
   *                                  subscription destination. It is assumed
   *                                  that the message is received with
   *                                  application/json content type.
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   *  one.
   */
  std::string Subscribe(std::string_view destination,
                        OnSubscribedCallback on_subscribed_callback,
                        OnReceivedCallback on_received_callback);

 private:
  struct Subscription {
    std::string id;
    std::string destination;
    OnSubscribedCallback on_subscribed_callback;
    OnReceivedCallback on_received_callback;
  };

  static std::string GenerateSubscriptionId();

  void OnWebSocketConnected(boost::system::error_code result);
  void ConnectToStompServer();
  void OnStompConnectSent(boost::system::error_code result);
  void HandleStompFrame(StompFrame frame);
  void OnWebSocketReceived(std::string message);
  void OnWebSocketSent(boost::system::error_code result);
  void OnWebSocketDisconnected(boost::system::error_code result);
  void OnWebSocketClosed(boost::system::error_code result,
                         OnClosedCallback on_closed_callback = nullptr);
  void OnWebSocketSentSubscribe(boost::system::error_code result,
                                Subscription subscription);

  void HandleStompConnected(const StompFrame& frame);
  void HandleStompReceipt(const StompFrame& frame);
  void HandleStompMessage(const StompFrame& frame);
  void HandleStompError(const StompFrame& frame);

  void OnConnectingDone(StompClientResult result);

  std::optional<std::reference_wrapper<Subscription>> FindSubscription(
      std::string_view subscription_id);

  OnConnectingDoneCallback on_connecting_done_callback_;
  OnDisconnectedCallback on_disconnected_callback_;
  OnClosedCallback on_closed_callback_;
  OnReceivedCallback on_received_callback_;

  std::string user_name_;
  std::string user_password_;

  std::vector<Subscription> subscriptions_;

  WebSocketClient websocket_client_;
  boost::asio::strand<boost::asio::io_context::executor_type> async_context_;

  bool websocket_connected_{false};
  bool stomp_connected_{false};
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
    OnConnectingDoneCallback on_connecting_done_callback,
    OnDisconnectedCallback on_disconnected_callback) {
  LOG_INFO("Connecting to STOMP server");

  user_name_ = user_name;
  user_password_ = user_password;
  on_connecting_done_callback_ = std::move(on_connecting_done_callback);
  on_disconnected_callback_ = std::move(on_disconnected_callback);

  websocket_client_.Connect(
      [this](auto result) { OnWebSocketConnected(result); },
      [this](auto message) { OnWebSocketReceived(std::move(message)); },
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
    std::string_view destination,
    OnSubscribedCallback on_subscribed_callback,
    OnReceivedCallback on_received_callback) {
  LOG_INFO("Starting subscription to '{}'", destination);

  auto subscription =
      Subscription{.id = GenerateSubscriptionId(),
                   .destination = std::string{destination},
                   .on_subscribed_callback = on_subscribed_callback,
                   .on_received_callback = on_received_callback};

  LOG_DEBUG("subscription_id: '{}'", subscription.id);

  std::string stomp_frame{StompFrameBuilder()
                              .SetCommand(StompCommand::Subscribe)
                              .AddHeader(StompHeader::Destination, destination)
                              .AddHeader(StompHeader::Id, subscription.id)
                              .AddHeader(StompHeader::Ack, "auto")
                              .AddHeader(StompHeader::Receipt, subscription.id)
                              .BuildString()};

  std::string subscription_id{subscription.id};

  websocket_client_.Send(
      stomp_frame, [this, subscription = std::move(subscription)](auto result) {
        OnWebSocketSentSubscribe(result, std::move(subscription));
      });

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

  websocket_client_.Send(connect_frame,
                         [this](auto result) { OnStompConnectSent(result); });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnStompConnectSent(
    boost::system::error_code result) {
  if (result.failed()) {
    LOG_ERROR("Could not send STOMP frame: {}", result.message());
    OnConnectingDone(StompClientResult::ErrorConnectingStomp);
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketReceived(std::string message) {
  const auto frame = StompFrame(std::move(message));
  if (frame.GetParseResultCode() != ParseResultCode::Ok) {
    LOG_WARN("Could not parse the message to STOMP frame: {}",
             ToString(frame.GetParseResultCode()));
    if (!stomp_connected_) {
      OnConnectingDone(StompClientResult::ErrorConnectingStomp);
    }

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
      break;
    case StompCommand::Error:
      HandleStompError(frame);
      break;
    default: {
      // TODO: handle the rest of commands
      LOG_ERROR("Unexpected STOMP command: '{}'", ToString(frame.GetCommand()));
      break;
    }
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketSent(
    boost::system::error_code result) {
  //
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketDisconnected(
    boost::system::error_code result) {
  LOG_INFO("WebSocket disconnected: {}", result.message());

  websocket_connected_ = false;
  stomp_connected_ = false;

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
    boost::system::error_code result, Subscription subscription) {
  if (result.failed()) {
    LOG_WARN("Could not subscribe to '{}': ", subscription.destination,
             result.message());
    if (subscription.on_subscribed_callback) {
      boost::asio::post(
          async_context_,
          [on_subscribed = subscription.on_subscribed_callback]() {
            on_subscribed(StompClientResult::CouldNotSendSubscribeFrame, {});
          });
    }
    return;
  }

  subscriptions_.push_back(std::move(subscription));
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompConnected(
    const StompFrame& /*frame*/) {
  LOG_INFO("Connected to STOMP server");
  stomp_connected_ = true;
  OnConnectingDone(StompClientResult::Ok);
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompReceipt(const StompFrame& frame) {
  if (!stomp_connected_) {
    OnConnectingDone(StompClientResult::ErrorConnectingStomp);
    return;
  }

  // Supports only SUBSCRIBE frame.
  const auto subscription_id = frame.GetHeaderValue(StompHeader::ReceiptId);

  auto subscription = FindSubscription(subscription_id);
  if (!subscription.has_value()) {
    LOG_WARN("Unknown subscription id: '{}'", subscription_id);
    return;
  }

  LOG_INFO("Subscribed to '{}'", subscription.value().get().id);

  if (subscription.value().get().on_subscribed_callback) {
    boost::asio::post(async_context_, [subscription]() mutable {
      subscription.value().get().on_subscribed_callback(
          StompClientResult::Ok, subscription.value().get().id);
    });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompMessage(const StompFrame& frame) {
  if (!stomp_connected_) {
    OnConnectingDone(StompClientResult::ErrorConnectingStomp);
    return;
  }

  // Find the subscription
  auto destination = frame.GetHeaderValue(StompHeader::Destination);
  auto message_id = frame.GetHeaderValue(StompHeader::MessageId);
  auto subscription_id =
      std::string(frame.GetHeaderValue(StompHeader::Subscription));

  if (destination.empty() || message_id.empty() || subscription_id.empty()) {
    LOG_WARN("Required fields are missing");
    return;
  }

  auto subscription = FindSubscription(subscription_id);
  if (!subscription.has_value()) {
    LOG_WARN("Unknown subscription id: '{}'", subscription_id);
    return;
  }

  if (subscription.value().get().destination != destination) {
    LOG_WARN(
        "message destination does no match subscription destination: '{}' and "
        "'{}'",
        subscription.value().get().destination, destination);
    return;
  }

  if (subscription.value().get().on_received_callback) {
    boost::asio::post(async_context_,
                      [&subscription, message_body = frame.GetBody()]() {
                        subscription.value().get().on_received_callback(
                            StompClientResult::Ok, std::string(message_body));
                      });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompError(const StompFrame& frame) {
  if (!stomp_connected_) {
    OnConnectingDone(StompClientResult::ErrorConnectingStomp);
    return;
  }

  websocket_client_.Close(
      [this](auto result) { OnWebSocketDisconnected(result); });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnConnectingDone(StompClientResult result) {
  LOG_DEBUG("result: {}", ToStringView(result));
  if (on_connecting_done_callback_) {
    boost::asio::post(async_context_, [callback = on_connecting_done_callback_,
                                       result]() { callback(result); });
  }
}

template <typename WebSocketClient>
std::optional<
    std::reference_wrapper<typename StompClient<WebSocketClient>::Subscription>>
StompClient<WebSocketClient>::FindSubscription(
    std::string_view subscription_id) {
  auto find_result = std::ranges::find_if(
      subscriptions_, [&subscription_id](const auto& subscription) {
        return subscription_id == subscription.id;
      });

  return find_result != subscriptions_.end()
             ? std::make_optional(std::reference_wrapper(*find_result))
             : std::nullopt;
}

template <typename WebSocketClient>
std::string StompClient<WebSocketClient>::GenerateSubscriptionId() {
  std::stringstream output{};
  output << boost::uuids::random_generator()();
  return output.str();
}

}  // namespace network_monitor
