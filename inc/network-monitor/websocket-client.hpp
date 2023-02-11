// TODO: Add header guards

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>

namespace network_monitor {

/*! \brief Client to connect to a WebSocket server over plain TCP.
 *
 *  \tparam Resolver        The class to resolve the URL to an IP address. It must support
 *                          the same interface of boost::asio::ip::tcp::resolver.
 *  \tparam WebSocketStream The WebSocket stream class. It must support the same interface
 *                          of boost::beast::websocket::stream.
 */
template<typename Resolver, typename WebSocketStream> class WebSocketClient {
public:
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
    WebSocketClient(const std::string& url,
                    const std::string& endpoint,
                    const std::string& port,
                    boost::asio::io_context& io_context,
                    boost::asio::ssl::context& tls_context);

    /*! \brief Destructor.
     */
    ~WebSocketClient();

    /*! \brief Connect to the server.
     *
     *  \param on_connect       Called when the connection fails or succeeds.
     *  \param on_message       Called only when a message is successfully
     *                          received. The message is an rvalue reference;
     *                          ownership is passed to the receiver.
     *  \param on_disconnect    Called when the connection is closed by the server
     *                          or due to a connection error.
     */
    void Connect(std::function<void(boost::system::error_code)> on_connect = nullptr,
                 std::function<void(boost::system::error_code, std::string&&)>
                     on_message = nullptr,
                 std::function<void(boost::system::error_code)> on_disconnect = nullptr);

    /*! \brief Send a text message to the WebSocket server.
     *
     *  \param message  The message to send. The caller must ensure that this
     *                  string stays in scope until the onSend handler is called.
     *  \param on_send  Called when a message is sent successfully or if it
     *                  failed to send.
     */
    void Send(const std::string& message,
              std::function<void(boost::system::error_code)> on_send_callback = nullptr);

    /*! \brief Close the WebSocket connection.
     *
     *  \param on_close Called when the connection is closed, successfully or not.
     */
    void Close(std::function<void(boost::system::error_code)> on_close = nullptr);

private:
    void SaveProvidedCallbacks(
        std::function<void(boost::system::error_code)> on_connect = nullptr,
        std::function<void(boost::system::error_code, std::string&&)> on_message =
            nullptr,
        std::function<void(boost::system::error_code)> on_disconnect = nullptr);
    void ResolveServerUrl();
    void ConnectToServer(boost::asio::ip::tcp::resolver::results_type results);
    void SetTcpStreamTimeoutToSuggested();
    void HandshakeTls();
    void HandshakeWebSocket();
    void ListenToIncomingMessage(const boost::system::error_code& error);
    std::string ReadMessage(const size_t received_bytes_count);

    void CallOnConnectCallbackIfExists(const boost::system::error_code& error);
    void CallOnMessageCallbackIfExists(const boost::system::error_code& error,
                                       std::string&& message);

    void OnServerUrlResolved(boost::asio::ip::tcp::resolver::results_type results);
    void OnConnectedToServer(const boost::system::error_code& error);
    void OnTlsHandshakeCompleted(const boost::system::error_code& error);
    void OnWebSocketHandshakeCompleted(const boost::system::error_code& error);
    void OnMessageReceived(const boost::system::error_code& error,
                           const size_t received_bytes_count);

    const std::string server_url_{};
    const std::string server_endpoint_{};
    const std::string server_port_{};

    Resolver resolver_;
    WebSocketStream websocket_stream_;

    boost::beast::flat_buffer response_buffer_{};
    bool closed_{true};

    std::function<void(boost::system::error_code)> on_connect_callback_;
    std::function<void(boost::system::error_code, std::string&&)> on_message_callback_;
    std::function<void(boost::system::error_code)> on_disconnect_callback_;
};

template<typename Resolver, typename WebSocketStream>
WebSocketClient<Resolver, WebSocketStream>::WebSocketClient(
    const std::string& url,
    const std::string& endpoint,
    const std::string& port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context)
    : server_url_{url},
      server_endpoint_{endpoint},
      server_port_{port},
      resolver_{boost::asio::make_strand(io_context)},
      websocket_stream_{boost::asio::make_strand(io_context), tls_context}
{
}

template<typename Resolver, typename WebSocketStream>
WebSocketClient<Resolver, WebSocketStream>::~WebSocketClient() = default;

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::Connect(
    std::function<void(boost::system::error_code)> on_connect,
    std::function<void(boost::system::error_code, std::string&&)> on_message,
    std::function<void(boost::system::error_code)> on_disconnect)
{
    SaveProvidedCallbacks(std::move(on_connect), std::move(on_message),
                          std::move(on_disconnect));
    ResolveServerUrl();
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::SaveProvidedCallbacks(
    std::function<void(boost::system::error_code)> on_connect,
    std::function<void(boost::system::error_code, std::string&&)> on_message,
    std::function<void(boost::system::error_code)> on_disconnect)
{
    if (on_connect) {
        on_connect_callback_ = std::move(on_connect);
    }
    if (on_message) {
        on_message_callback_ = std::move(on_message);
    }
    if (on_disconnect) {
        on_disconnect_callback_ = std::move(on_disconnect);
    }
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::ResolveServerUrl()
{
    // TODO: The async_resolve method is deprecated. Use overload with separate host
    //       and service parameters.
    //       https://www.boost.org/doc/libs/1_81_0/doc/html/boost_asio/reference/ip__basic_resolver/async_resolve.html
    resolver_.async_resolve(server_url_, server_port_, [this](auto error, auto results) {
        if (error) {
            return;
        }
        OnServerUrlResolved(results);
    });
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnServerUrlResolved(
    boost::asio::ip::tcp::resolver::results_type results)
{
    ConnectToServer(results);
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::ConnectToServer(
    boost::asio::ip::tcp::resolver::results_type results)
{
    auto& tcp_stream = boost::beast::get_lowest_layer(websocket_stream_);
    tcp_stream.expires_after(std::chrono::seconds(5));
    tcp_stream.async_connect(*results,
                             [this](auto error) { OnConnectedToServer(error); });
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnConnectedToServer(
    const boost::system::error_code& error)
{
    SetTcpStreamTimeoutToSuggested();

    // Set the host name before the TLS handshake or the connection will fail
    SSL_set_tlsext_host_name(websocket_stream_.next_layer().native_handle(),
                             server_url_.c_str());

    HandshakeTls();
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::SetTcpStreamTimeoutToSuggested()
{
    auto& tcp_stream = boost::beast::get_lowest_layer(websocket_stream_);
    tcp_stream.expires_never();
    websocket_stream_.set_option(boost::beast::websocket::stream_base::timeout::suggested(
        boost::beast::role_type::client));
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::HandshakeTls()
{
    websocket_stream_.next_layer().async_handshake(
        boost::asio::ssl::stream_base::handshake_type::client,
        [this](auto error) { OnTlsHandshakeCompleted(error); });
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnTlsHandshakeCompleted(
    const boost::system::error_code& error)
{
    HandshakeWebSocket();
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::HandshakeWebSocket()
{
    const std::string connection_host{server_url_ + ':' + server_port_};
    websocket_stream_.async_handshake(connection_host, server_endpoint_,
                                      [this](const boost::system::error_code& error) {
                                          OnWebSocketHandshakeCompleted(error);
                                      });
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnWebSocketHandshakeCompleted(
    const boost::system::error_code& error)
{
    CallOnConnectCallbackIfExists(error);
    ListenToIncomingMessage(error);
    closed_ = false;
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::CallOnConnectCallbackIfExists(
    const boost::system::error_code& error)
{
    if (on_connect_callback_) {
        on_connect_callback_(error);
    }
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::ListenToIncomingMessage(
    const boost::system::error_code& error)
{
    if (error == boost::asio::error::operation_aborted) {
        if (on_disconnect_callback_ && !closed_) {
            on_disconnect_callback_(error);
        }
        return;
    }
    websocket_stream_.async_read(response_buffer_,
                                 [this](auto error, auto received_bytes_count) {
                                     OnMessageReceived(error, received_bytes_count);
                                 });
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::OnMessageReceived(
    const boost::system::error_code& error, const size_t received_bytes_count)
{
    if (error) {
        return;
    }
    CallOnMessageCallbackIfExists(error, ReadMessage(received_bytes_count));
    ListenToIncomingMessage(error);
}

template<typename Resolver, typename WebSocketStream>
std::string
WebSocketClient<Resolver, WebSocketStream>::ReadMessage(const size_t received_bytes_count)
{
    std::string message{boost::beast::buffers_to_string(response_buffer_.data())};
    response_buffer_.consume(received_bytes_count);
    return message;
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::CallOnMessageCallbackIfExists(
    const boost::system::error_code& error, std::string&& message)
{
    if (on_message_callback_) {
        on_message_callback_(error, std::move(message));
    }
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::Send(
    const std::string& message,
    std::function<void(boost::system::error_code)> on_send_callback)
{
    websocket_stream_.async_write(boost::asio::buffer(message),
                                  [this, on_send_callback](auto error, auto) {
                                      if (on_send_callback) {
                                          on_send_callback(error);
                                      }
                                  });
}

template<typename Resolver, typename WebSocketStream>
void WebSocketClient<Resolver, WebSocketStream>::Close(
    std::function<void(boost::system::error_code)> on_close_callback)
{
    websocket_stream_.async_close(boost::beast::websocket::close_code::none,
                                  on_close_callback);
    closed_ = true;
}

using BoostWebSocketClient = WebSocketClient<
    boost::asio::ip::tcp::resolver,
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>;

} // namespace network_monitor