#pragma once

#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/bimap.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "network-monitor/stomp-frame-builder.hpp"
#include "network-monitor/stomp-frame.hpp"

namespace network_monitor {

/*! \brief Error codes for the STOMP client.
 */
enum class StompClientError {
  Ok = 0,
  CouldNotConnectToWebSocketServer,
  UnexpectedCouldNotCreateValidFrame,
  CouldNotSendConnectFrame,
  CouldNotParseMessageAsStompFrame,
  CouldNotCloseWebSocketConnection,
  WebSocketServerDisconnected,
  CouldNotSendSubscribeFrame,
  UndefinedError
  // TODO
};

// TODO: move it to the common space with stomp-frame.cpp
template <typename L, typename R>
boost::bimap<L, R> MakeBimap(
    std::initializer_list<typename boost::bimap<L, R>::value_type> list) {
  return boost::bimap<L, R>(list.begin(), list.end());
}

// TODO: move it to stomp-client.cpp
static const auto stomp_client_error_strings{
    // clang-format off
    MakeBimap<StompClientError, std::string_view>({
        {StompClientError::Ok,                                    "Ok"                                    },
        {StompClientError::CouldNotConnectToWebSocketServer,      "CouldNotConnectToWebSocketServer"      },
        {StompClientError::UnexpectedCouldNotCreateValidFrame,    "UnexpectedCouldNotCreateValidFrame"    },
        {StompClientError::CouldNotSendConnectFrame,              "CouldNotSendConnectFrame"              },
        {StompClientError::CouldNotParseMessageAsStompFrame,      "CouldNotParseMessageAsStompFrame"      },
        {StompClientError::CouldNotCloseWebSocketConnection,      "CouldNotCloseWebSocketConnection"      },
        {StompClientError::WebSocketServerDisconnected,           "WebSocketServerDisconnected"           },
        {StompClientError::CouldNotSendSubscribeFrame,            "CouldNotSendSubscribeFrame"            },
        {StompClientError::UndefinedError,                        "UndefinedError"                        }
    })
    // clang-format on
};

std::string_view ToStringView(const StompClientError& command) {
  const auto string_representation{
      stomp_client_error_strings.left.find(command)};
  if (string_representation == stomp_client_error_strings.left.end()) {
    return stomp_client_error_strings.left
        .find(StompClientError::UndefinedError)
        ->second;
  } else {
    return string_representation->second;
  }
}

std::ostream& operator<<(std::ostream& os, const StompClientError& error) {
  os << ToStringView(error);
  return os;
}

/*! \brief STOMP client implementing the subset of commands needed by the
 * network-events service.
 *
 *  \param WebSocketClient      WebSocket client class. This type must have the
 * same interface of WebSocketClient.
 */
template <typename WebSocketClient>
class StompClient {
 public:
  /*! \brief Construct a STOMP client connecting to a remote URL/port through a
   * secure WebSocket connection.
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
   * establish a STOMP connection with the user credentials.
   *
   *  \param user_name        User name.
   *  \param user_password    User password.
   *  \param on_connect       This handler is called when the STOMP connection
   * is setup correctly. It will also be called with an error if there is any
   * failure before a successful connection. \param on_disconnect    This
   * handler is called when the STOMP or the WebSocket connection is suddenly
   * closed. In the STOMP protocol, this may happen also in response to bad
   * inputs (authentication, subscription).
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   * one.
   */
  void Connect(
      const std::string& user_name,
      const std::string& user_password,
      std::function<void(StompClientError)> on_connected_callback = nullptr,
      std::function<void(StompClientError)> on_disconnected_callback = nullptr);

  /*! \brief Close the STOMP and WebSocket connection.
   *
   *  \param on_close  Called when the connection has been closed, successfully
   * or not.
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   * one.
   */
  void Close(std::function<void(StompClientError)> on_close = nullptr);

  /*! \brief Subscribe to a STOMP endpoint.
   *
   *  \returns The subscripton ID, if the subscription process was started
   * successfully; otherwise, an empty string.
   *
   *  \param destination              The subscription topic.
   *  \param on_subscribed_callback   Called when the subscription is setup
   * correctly. The handler receives an error code and the subscription ID.
   * Note: On failure, this callback is only called if the failure happened at
   * the WebSocket level, not at the STOMP level. This is due to the fact that
   * the STOMP server automatically closes the WebSocket connection on a STOMP
   * protocol failure. \param on_message_callback      Called on every new
   * message from the subscription destination. It is assumed that the message
   * is received with application/json content type.
   *
   *  All handlers run in a separate I/O execution context from the WebSocket
   * one.
   */
  std::string Subscribe(
      const std::string& destination,
      std::function<void(StompClientError, std::string&&)>
          on_subscribed_callback,
      std::function<void(StompClientError, std::string&&)> on_message_callback);

 private:
  using OnSubscribeCallback =
      std::function<void(StompClientError, std::string&&)>;
  using OnMessageCallback =
      std::function<void(StompClientError, std::string&&)>;

  struct Subscription {
    std::string destination;
    OnSubscribeCallback on_subscribe_callback{nullptr};
    OnMessageCallback on_message_callback{nullptr};
  };

  void OnWebSocketConnected(boost::system::error_code result);
  void OnWebSocketConnectMessageSent(boost::system::error_code result);
  void OnWebSocketMessageReceived(boost::system::error_code result,
                                  std::string&& message);
  void OnWebSocketMessageSent(boost::system::error_code result);
  void OnWebSocketDisconnected(boost::system::error_code result);
  void OnWebSocketClosed(
      boost::system::error_code result,
      std::function<void(StompClientError)> on_close_callback = nullptr);
  void OnWebSocketSentSubscribe(boost::system::error_code result,
                                std::string& subscription_id,
                                Subscription&& subscription);

  void HandleStompConnected(const StompFrame& frame);
  void HandleStompReceipt(const StompFrame& frame);
  void HandleStompMessage(const StompFrame& frame);

  void CallOnConnectedCallbackWithErrorIfValid(StompClientError error);

  static std::string GenerateSubscriptionId();

  std::function<void(StompClientError)> on_connected_callback_;
  std::function<void(StompClientError, StompFrame&&)> on_message_callback_;
  std::function<void(StompClientError)> on_disconnected_callback_;
  std::function<void(StompClientError)> on_closed_callback_;

  std::string user_name_;
  std::string user_password_;

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
    std::function<void(StompClientError)> on_connected_callback,
    std::function<void(StompClientError)> on_disconnected_callback) {
  // TODO: add log StompClient: Connecting to STOMP server
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
void StompClient<WebSocketClient>::Close(
    std::function<void(StompClientError)> on_closed) {
  // TODO: log StompClient: Closing connection to STOMP server
  // TODO: clear subscriptions
  websocket_client_.Close(
      [this, on_closed](auto result) { OnWebSocketClosed(result, on_closed); });
}

template <typename WebSocketClient>
std::string StompClient<WebSocketClient>::Subscribe(
    const std::string& destination,
    std::function<void(StompClientError, std::string&&)> on_subscribed_callback,
    std::function<void(StompClientError, std::string&&)> on_message_callback) {
  // TODO: add log StompClient: Subscribing to {destination}

  auto subscription_id{GenerateSubscriptionId()};

  // TODO: handle error ocurred when creating a frame
  auto stomp_frame{stomp_frame::MakeSubscribeFrame(destination, subscription_id,
                                                   "auto", subscription_id)};

  Subscription subscription{destination, on_subscribed_callback,
                            on_message_callback};

  // TODO: why mutable? why assign operator instead of just std::move?
  auto on_websocket_sent_callback = [this, subscription_id,
                                     subscription = std::move(subscription)](
                                        auto result) mutable {
    OnWebSocketSentSubscribe(result, subscription_id, std::move(subscription));
  };

  websocket_client_.Send(stomp_frame.ToString(), on_websocket_sent_callback);

  return subscription_id;
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketConnected(
    boost::system::error_code result) {
  if (result.failed()) {
    // TODO: put a log "StompClient: Could not connect to server:
    // {result.message()}"
    CallOnConnectedCallbackWithErrorIfValid(
        StompClientError::CouldNotConnectToWebSocketServer);
    return;
  }

  websocket_connected_ = true;

  // TODO: use stomp_frame::MakeConnectFrame
  stomp_frame::BuildParameters parameters(StompCommand::Connect);
  parameters.headers.emplace(StompHeader::AcceptVersion, "1.2");
  parameters.headers.emplace(StompHeader::Host,
                             websocket_client_.GetServerUrl());
  parameters.headers.emplace(StompHeader::Login, user_name_);
  parameters.headers.emplace(StompHeader::Passcode, user_password_);

  const auto frame = stomp_frame::Build(parameters);
  if (frame.GetStompError() != StompError::Ok) {
    // add log StompClient: Could not create a valid frame: {error}
    CallOnConnectedCallbackWithErrorIfValid(
        StompClientError::UnexpectedCouldNotCreateValidFrame);
    return;
  }

  websocket_client_.Send(frame.ToString(), [this](auto result) {
    OnWebSocketConnectMessageSent(result);
  });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketConnectMessageSent(
    boost::system::error_code result) {
  if (result.failed()) {
    // add log StompClient: Could not send STOMP frame: {result.message()}
    CallOnConnectedCallbackWithErrorIfValid(
        StompClientError::CouldNotSendConnectFrame);
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketMessageReceived(
    boost::system::error_code result, std::string&& message) {
  if (result.failed()) {
    // TODO: handle
    return;
  }

  StompFrame frame{std::move(message)};
  if (frame.GetStompError() != StompError::Ok) {
    // TODO: log StompClient: Could not parse message as STOMP frame:
    // {stomp_error}
    CallOnConnectedCallbackWithErrorIfValid(
        StompClientError::CouldNotParseMessageAsStompFrame);
    return;
  }

  // TODO: log StompClient: Received {frame.GetCommand()}
  switch (frame.GetCommand()) {
    case StompCommand::Connected:
      HandleStompConnected(frame);
      break;
    case StompCommand::Receipt:
      HandleStompReceipt(frame);
      break;
    case StompCommand::Message:
      HandleStompMessage(frame);
    // TODO
    default: {
      // TODO: log StompClient: Unexpected STOMP command: {frame.GetCommand()}
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
  // TODO: log: StompClient: WebSocket connection disconnected:
  // {result.message()}
  websocket_connected_ = false;
  if (on_disconnected_callback_) {
    auto error{result ? StompClientError::WebSocketServerDisconnected
                      : StompClientError::Ok};
    boost::asio::post(async_context_,
                      [this, error]() { on_disconnected_callback_(error); });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketClosed(
    boost::system::error_code result,
    std::function<void(StompClientError)> on_close_callback) {
  if (on_close_callback) {
    auto error{result.failed()
                   ? StompClientError::CouldNotCloseWebSocketConnection
                   : StompClientError::Ok};
    boost::asio::post(async_context_, [on_close_callback, error]() {
      on_close_callback(error);
    });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketSentSubscribe(
    boost::system::error_code result,
    std::string& subscription_id,
    Subscription&& subscription) {
  if (result.failed()) {
    // TODO: log StompClient: Could not subscribe to {subscribtion.destination}:
    // {result.message}
    if (subscription.on_subscribe_callback) {
      boost::asio::post(async_context_,
                        [on_subscribe = subscription.on_subscribe_callback]() {
                          on_subscribe(
                              StompClientError::CouldNotSendSubscribeFrame, {});
                        });
    }
    return;
  }

  subscriptions_.emplace(subscription_id, std::move(subscription));
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompConnected(
    const StompFrame& frame) {
  // TODO: log StompClient: Successfully connected to STOMP server
  CallOnConnectedCallbackWithErrorIfValid(StompClientError::Ok);
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::HandleStompReceipt(const StompFrame& frame) {
  // Supports only SUBSCRIBE frame.
  auto subscription_id{frame.GetHeaderValue(StompHeader::ReceiptId)};
  auto subscription_iterator{subscriptions_.find(std::string(subscription_id))};
  if (subscription_iterator == subscriptions_.end()) {
    // TODO: log error StompClient:: Cannot find subscription {subscription_id}
    return;
  }
  const auto& subscription{subscription_iterator->second};

  // TODO: add log StompClient: Successfully subscribed to {subscription_id}
  if (subscription.on_subscribe_callback) {
    boost::asio::post(
        async_context_,
        [on_subscribe = subscription.on_subscribe_callback,
         subscription_id = std::string(subscription_id)]() mutable {
          on_subscribe(StompClientError::Ok, std::move(subscription_id));
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
    // TODO: add error log
    return;
  }

  if (!subscriptions_.count(subscription_id)) {
    // TODO: add error log
    return;
  }
  auto& subscription = subscriptions_.at(subscription_id);

  if (subscription.destination != destination) {
    // TODO: add error log
    return;
  }

  if (subscription.on_message_callback) {
    boost::asio::post(
        async_context_, [on_message = subscription.on_message_callback,
                         message_body = std::string(frame.GetBody())]() {
          on_message(StompClientError::Ok, std::string(message_body));
        });
  }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::CallOnConnectedCallbackWithErrorIfValid(
    StompClientError error) {
  if (on_connected_callback_) {
    boost::asio::post(async_context_,
                      [on_connected_callback = on_connected_callback_,
                       error]() { on_connected_callback(error); });
  }
}

template <typename WebSocketClient>
std::string StompClient<WebSocketClient>::GenerateSubscriptionId() {
  std::stringstream output{};
  output << boost::uuids::random_generator()();
  return output.str();
}

}  // namespace network_monitor
