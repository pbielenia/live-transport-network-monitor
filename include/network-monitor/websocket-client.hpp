#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>

namespace network_monitor {

/*! \brief Client to connect to a WebSocket server over plain TCP.
 *
 *  \tparam Resolver          The class to resolve the URL to an IP address. It
 *                            must support the same interface of
 *                            boost::asio::ip::tcp::resolver.
 *  \tparam WebSocketStream   The WebSocket stream class. It must support the
 *                            same interface of boost::beast::websocket::stream.
 */
template <typename Resolver, typename WebSocketStream>
class WebSocketClient {
 public:
  using kOnConnectedCallback = std::function<void(boost::system::error_code)>;
  using kOnMessageReceivedCallback =
      std::function<void(boost::system::error_code, std::string&&)>;
  using kOnMessageSentCallback = std::function<void(boost::system::error_code)>;
  using kOnDisconnectedCallback =
      std::function<void(boost::system::error_code)>;
  using kOnConnectionClosedCallback =
      std::function<void(boost::system::error_code)>;

  /*! \brief Construct a WebSocket client.
   *
   *  \note This constructor does not initiate a connection.
   *
   *  \param url          The URL of the server.
   *  \param endpoint     The endpoint on the server to connect to.
   *                      Example: ltnm.learncppthroughprojects.com/<endpoint>
   *  \param port         The port on the server.
   *  \param io_context   The io_context object. The user takes care of calling
   *                      ioc.run().
   *  \param tls_context  The TLS context to setup a TLS socket stream.
   */
  WebSocketClient(std::string url,
                  std::string endpoint,
                  std::string port,
                  boost::asio::io_context& io_context,
                  boost::asio::ssl::context& tls_context);

  /*! \brief Copy constructor.
   */
  WebSocketClient(const WebSocketClient<Resolver, WebSocketStream>& other) =
      delete;

  /*! \brief Move constructor.
   */
  WebSocketClient(WebSocketClient<Resolver, WebSocketStream>&& other) = delete;

  /*! \brief Destructor.
   */
  ~WebSocketClient();

  /*! \brief Copy assignment operator.
   */
  WebSocketClient<Resolver, WebSocketStream>& operator=(
      const WebSocketClient<Resolver, WebSocketStream>& other) = delete;

  /*! \brief Move assignment operator.
   */
  WebSocketClient<Resolver, WebSocketStream>& operator=(
      WebSocketClient<Resolver, WebSocketStream>&& other) = delete;

  /*! \brief Connect to the server.
   *
   *  \param on_connected_callback          Called when the connection fails or
   *                                        succeeds.
   *  \param on_message_received_callback   Called only when a message is
   *                                        successfully received. The message
   *                                        is an rvalue reference; ownership
   *                                        is passed to the receiver.
   *  \param on_disconnected_callback       Called when the connection is closed
   *                                        by the server or due to a connection
   *                                        error.
   */
  void Connect(
      kOnConnectedCallback on_connected_callback = nullptr,
      kOnMessageReceivedCallback on_message_received_callback = nullptr,
      kOnDisconnectedCallback on_disconnected_callback = nullptr);

  /*! \brief Send a text message to the WebSocket server.
   *
   *  \param message  The message to send. The caller must ensure that this
   *                  string stays in scope until the onSend handler is called.
   *  \param on_send  Called when a message is sent successfully or if it
   *                  failed to send.
   */
  void Send(const std::string& message,
            kOnMessageSentCallback on_message_sent_callback = nullptr);

  /*! \brief Close the WebSocket connection.
   *
   *  \param on_connection_closed_callback  Called when the connection is
   *                                        closed, successfully or not.
   */
  void Close(
      kOnConnectionClosedCallback on_connection_closed_callback = nullptr);

  const std::string& GetServerUrl() const;
  const std::string& GetServerPort() const;

 private:
  static constexpr auto kConnectToServerTimeout = std::chrono::seconds(5);

  void ResolveServerUrl();
  void ConnectToServer(
      const boost::asio::ip::tcp::resolver::results_type& endpoint);
  void SetSuggestedTcpStreamTimeout();
  void HandshakeTls();
  void HandshakeWebSocket();
  void ListenToIncomingMessage(boost::system::error_code error);
  std::string ReadMessage(size_t received_bytes_count);

  void OnServerUrlResolved(
      boost::system::error_code error,
      boost::asio::ip::tcp::resolver::results_type results);
  void OnConnectedToServer(boost::system::error_code error);
  void OnConnectingToServerCompleted(boost::system::error_code error);
  void OnTlsHandshakeCompleted(boost::system::error_code error);
  void OnWebSocketHandshakeCompleted(boost::system::error_code error);
  void OnMessageReceived(boost::system::error_code error,
                         size_t received_bytes_count);

  const std::string server_url_;
  const std::string server_endpoint_;
  const std::string server_port_;

  Resolver resolver_;
  WebSocketStream websocket_stream_;

  boost::beast::flat_buffer response_buffer_;
  bool connection_is_open_{false};

  kOnConnectedCallback on_connected_callback_;
  kOnMessageReceivedCallback on_message_received_callback_;
  kOnDisconnectedCallback on_disconnected_callback_;
};

template <typename Resolver, typename WebSocketStream>
WebSocketClient<Resolver, WebSocketStream>::WebSocketClient(
    std::string url,
    std::string endpoint,
    std::string port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context)
    : server_url_{std::move(url)},
      server_endpoint_{std::move(endpoint)},
      server_port_{std::move(port)},
      resolver_{boost::asio::make_strand(io_context)},
      websocket_stream_{boost::asio::make_strand(io_context), tls_context} {}

template <typename Resolver, typename WebSocketStream>
WebSocketClient<Resolver, WebSocketStream>::~WebSocketClient() = default;

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::Connect(
    WebSocketClient::kOnConnectedCallback on_connected_callback,
    WebSocketClient::kOnMessageReceivedCallback on_message_received_callback,
    WebSocketClient::kOnDisconnectedCallback on_disconnected_callback) {
  on_connected_callback_ = std::move(on_connected_callback);
  on_message_received_callback_ = std::move(on_message_received_callback);
  on_disconnected_callback_ = std::move(on_disconnected_callback);

  ResolveServerUrl();
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::ResolveServerUrl() {
  // FIXME: The async_resolve method is deprecated. Use overload with separate
  // host
  //        and service parameters.
  //        https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/reference/ip__basic_resolver/async_resolve.html
  resolver_.async_resolve(server_url_, server_port_,
                          [this](auto error, auto results) {
                            OnServerUrlResolved(error, results);
                          });
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnServerUrlResolved(
    boost::system::error_code error,
    boost::asio::ip::tcp::resolver::results_type results) {
  if (error.failed()) {
    OnConnectingToServerCompleted(error);
    return;
  }
  ConnectToServer(results);
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::ConnectToServer(
    const boost::asio::ip::tcp::resolver::results_type& endpoint) {
  auto& tcp_stream = boost::beast::get_lowest_layer(websocket_stream_);
  tcp_stream.expires_after(kConnectToServerTimeout);
  tcp_stream.async_connect(*endpoint,
                           [this](auto error) { OnConnectedToServer(error); });
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnConnectedToServer(
    boost::system::error_code error) {
  if (error.failed()) {
    OnConnectingToServerCompleted(error);
    return;
  }

  SetSuggestedTcpStreamTimeout();

  // Set the host name before the TLS handshake or the connection will fail
  SSL_set_tlsext_host_name(websocket_stream_.next_layer().native_handle(),
                           server_url_.c_str());

  HandshakeTls();
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnConnectingToServerCompleted(
    boost::system::error_code error) {
  if (on_connected_callback_) {
    on_connected_callback_(error);
  }
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver,
                     WebSocketStream>::SetSuggestedTcpStreamTimeout() {
  auto& tcp_stream = boost::beast::get_lowest_layer(websocket_stream_);
  tcp_stream.expires_never();
  websocket_stream_.set_option(
      boost::beast::websocket::stream_base::timeout::suggested(
          boost::beast::role_type::client));
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::HandshakeTls() {
  websocket_stream_.next_layer().async_handshake(
      boost::asio::ssl::stream_base::handshake_type::client,
      [this](auto error) { OnTlsHandshakeCompleted(error); });
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnTlsHandshakeCompleted(
    boost::system::error_code error) {
  if (error.failed()) {
    OnConnectingToServerCompleted(error);
    return;
  }
  HandshakeWebSocket();
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::HandshakeWebSocket() {
  const std::string connection_host{server_url_ + ':' + server_port_};
  websocket_stream_.async_handshake(connection_host, server_endpoint_,
                                    [this](boost::system::error_code error) {
                                      OnWebSocketHandshakeCompleted(error);
                                    });
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnWebSocketHandshakeCompleted(
    boost::system::error_code error) {
  if (error.failed()) {
    OnConnectingToServerCompleted(error);
    return;
  }

  connection_is_open_ = true;
  OnConnectingToServerCompleted(error);
  ListenToIncomingMessage(error);
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::ListenToIncomingMessage(
    boost::system::error_code error) {
  if (error == boost::asio::error::operation_aborted) {
    if (on_disconnected_callback_ && connection_is_open_) {
      connection_is_open_ = false;
      on_disconnected_callback_(error);
    }
    return;
  }
  websocket_stream_.async_read(response_buffer_,
                               [this](auto error, auto received_bytes_count) {
                                 OnMessageReceived(error, received_bytes_count);
                               });
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnMessageReceived(
    boost::system::error_code error, const size_t received_bytes_count) {
  // FIXME: this return on error prevents calling the remaining stuff if there's
  //        an error and even so wants to pass the error to them
  if (error) {
    return;
  }

  if (on_message_received_callback_) {
    on_message_received_callback_(error, ReadMessage(received_bytes_count));
  }

  ListenToIncomingMessage(error);
}

template <typename Resolver, typename WebSocketStream>
std::string WebSocketClient<Resolver, WebSocketStream>::ReadMessage(
    const size_t received_bytes_count) {
  std::string message{boost::beast::buffers_to_string(response_buffer_.data())};
  response_buffer_.consume(received_bytes_count);
  return message;
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::Send(
    const std::string& message,
    WebSocketClient::kOnMessageSentCallback on_message_sent_callback) {
  websocket_stream_.async_write(
      boost::asio::buffer(message),
      [this, on_message_sent_callback = std::move(on_message_sent_callback)](
          auto error, auto) {
        if (on_message_sent_callback) {
          on_message_sent_callback(error);
        }
      });
}

template <typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::Close(
    WebSocketClient::kOnConnectionClosedCallback
        on_connection_closed_callback) {
  websocket_stream_.async_close(
      boost::beast::websocket::close_code::none,
      [on_connection_closed_callback =
           std::move(on_connection_closed_callback)](auto error_code) {
        if (on_connection_closed_callback) {
          on_connection_closed_callback(error_code);
        }
      });
  connection_is_open_ = false;
}

template <typename Resolver, typename WebSocketStream>
const std::string& WebSocketClient<Resolver, WebSocketStream>::GetServerUrl()
    const {
  return server_url_;
}

template <typename Resolver, typename WebSocketStream>
const std::string& WebSocketClient<Resolver, WebSocketStream>::GetServerPort()
    const {
  return server_port_;
}

using BoostWebSocketClient =
    WebSocketClient<boost::asio::ip::tcp::resolver,
                    boost::beast::websocket::stream<
                        boost::beast::ssl_stream<boost::beast::tcp_stream>>>;

}  // namespace network_monitor
