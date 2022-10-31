#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <memory>
#include <string>

namespace NetworkMonitor {

/*! \brief Client to connect to a WebSocket server over plain TCP.
 */
class WebSocketClient : public std::enable_shared_from_this<WebSocketClient> {
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

    boost::asio::ip::tcp::resolver resolver_;
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>
        websocket_stream_;

    boost::beast::flat_buffer response_buffer_{};
    bool closed_{true};

    std::function<void(boost::system::error_code)> on_connect_callback_;
    std::function<void(boost::system::error_code, std::string&&)> on_message_callback_;
    std::function<void(boost::system::error_code)> on_disconnect_callback_;
};

} // namespace NetworkMonitor