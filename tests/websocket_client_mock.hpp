#pragma once

#include <functional>
#include <string>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include "network_monitor/stomp_frame.hpp"

namespace network_monitor {

/*! \brief Mock the network_monitor::WebSocketClient.
 *
 */
class WebSocketClientMock {
 public:
  using OnConnectingDoneCallback =
      std::function<void(boost::system::error_code)>;
  using OnMessageReceivedCallback =
      std::function<void(boost::system::error_code, std::string&&)>;
  using OnMessageSentCallback = std::function<void(boost::system::error_code)>;
  using OnDisconnectedCallback = std::function<void(boost::system::error_code)>;
  using OnConnectionClosedCallback =
      std::function<void(boost::system::error_code)>;

  struct Config {
    static boost::system::error_code connect_error_code_;
    static boost::system::error_code send_error_code_;
    static boost::system::error_code close_error_code_;
    static bool trigger_disconnection_;
    static std::function<void(const std::string&)> on_message_sent_;
  };

  WebSocketClientMock(std::string url,
                      std::string endpoint,
                      std::string port,
                      boost::asio::io_context& io_context,
                      boost::asio::ssl::context& tls_context);

  void Connect(OnConnectingDoneCallback on_connecting_done_callback,
               OnMessageReceivedCallback on_message_received_callback,
               OnDisconnectedCallback on_disconnected_callback);
  void Send(const std::string& message,
            OnMessageSentCallback on_message_sent_callback = nullptr);
  void Close(
      OnConnectionClosedCallback on_connection_closed_callback = nullptr);
  const std::string& GetServerUrl() const;

 protected:
  void Disconnect();
  void SendToWebSocketClient(std::string message);

 private:
  boost::asio::strand<boost::asio::io_context::executor_type> async_context_;
  std::string server_url_;

  bool connected_{false};
  OnMessageReceivedCallback on_message_received_callback_;
  OnDisconnectedCallback on_disconnected_callback_;
};

/*! \brief Mocks the network_monitor::WebSocketClient talking to a STOMP server.
 * Mocks STOMP communication specifically.
 */
class WebSocketClientMockForStomp : public WebSocketClientMock {
 public:
  /*! \brief Use `username` and `password` static members in a test to set STOMP
   * user credentials. Needed to test if StompClient passes credentials
   * correctly.
   */
  static std::string username;
  static std::string password;

  /*! \brief `endpoint` static member is used to test if StompClient passes
   * endpoint correctly.
   */
  static std::string endpoint;

  WebSocketClientMockForStomp(const std::string& url,
                              const std::string& endpoint,
                              const std::string& port,
                              boost::asio::io_context& io_context,
                              boost::asio::ssl::context& tls_context);

  std::string stomp_version{"1.4"};

 private:
  void OnMessageSentToStompServer(const std::string& message);

  void HandleConnectMessage(const network_monitor::StompFrame& frame);
  void HandleSubscribeMessage(const network_monitor::StompFrame& frame);

  static bool FrameIsValidConnect(const network_monitor::StompFrame& frame);
  static bool FrameIsValidSubscribe(const network_monitor::StompFrame& frame);
};

}  // namespace network_monitor
