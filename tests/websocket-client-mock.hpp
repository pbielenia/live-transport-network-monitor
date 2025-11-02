#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <functional>
#include <queue>
#include <string>

#include "network-monitor/stomp-frame.hpp"

namespace NetworkMonitor {

/*! \brief Mock the NetworkMonitor::WebSocketClient.
 *
 */
class WebSocketClientMock {
 public:
  static boost::system::error_code connect_error_code;
  static boost::system::error_code send_error_code;
  static boost::system::error_code close_error_code;
  static std::queue<std::string> message_queue;
  static bool trigger_disconnection;
  static std::function<void(const std::string&)> respond_to_send;

  WebSocketClientMock(const std::string& url,
                      const std::string& endpoint,
                      const std::string& port,
                      boost::asio::io_context& io_context,
                      boost::asio::ssl::context& tls_context);

  void Connect(
      std::function<void(boost::system::error_code)> on_connected_callback,
      std::function<void(boost::system::error_code, std::string&&)> on_message_callback,
      std::function<void(boost::system::error_code)> on_disconnected_callback);
  void Send(const std::string& message,
            std::function<void(boost::system::error_code)> on_sent_callback = nullptr);
  void Close(std::function<void(boost::system::error_code)> on_close_callback = nullptr);
  const std::string& GetServerUrl() const;

 private:
  void MockIncomingMessages();

  boost::asio::strand<boost::asio::io_context::executor_type> async_context_;
  const std::string server_url_;

  bool connected_{false};
  std::function<void(boost::system::error_code, std::string&&)> on_message_callback_;
  std::function<void(boost::system::error_code)> on_disconnected_callback_;
};

/*! \brief Mocks the NetworkMonitor::WebSocketClient talking to a STOMP server. Mocks
 *         STOMP communication specifically.
 */
class WebSocketClientMockForStomp : public WebSocketClientMock {
 public:
  /*! \brief Use `username` and `password` static members in a test to set STOMP user
   *         credentials. Needed to test if StompClient passes credentials correctly.
   */
  static std::string username;
  static std::string password;

  /*! \brief `endpoint` static member is used to test if StompClient passes endpoint
   *         correctly.
   */
  static std::string endpoint;

  WebSocketClientMockForStomp(const std::string& url,
                              const std::string& endpoint,
                              const std::string& port,
                              boost::asio::io_context& io_context,
                              boost::asio::ssl::context& tls_context);

  const std::string stomp_version{"1.4"};

 private:
  void OnMessage(const std::string& message);
  void HandleConnectMessage(const NetworkMonitor::StompFrame& frame);
  void HandleSubscribeMessage(const NetworkMonitor::StompFrame& frame);

  bool FrameIsValidConnect(const NetworkMonitor::StompFrame& frame);
  bool FrameIsValidSubscribe(const NetworkMonitor::StompFrame& frame);
};

}  // namespace NetworkMonitor
