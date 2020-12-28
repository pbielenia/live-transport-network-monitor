#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <functional>
#include <string>

namespace network_monitor {

class WebSocketClient {
public:
    using OnConnectCallback = std::function<void(boost::system::error_code)>;
    using OnMessageCallback =
        std::function<void(boost::system::error_code, std::string&&)>;
    using OnDisconnectCallback = std::function<void(boost::system::error_code)>;
    using OnSendCallback = std::function<void(boost::system::error_code)>;
    using OnCloseCallback = std::function<void(boost::system::error_code)>;

    WebSocketClient(std::string host_url,
                    std::string port,
                    boost::asio::io_context& io_context,
                    boost::asio::ssl::context& ssl_context);

    ~WebSocketClient() = default;

    void connect(OnConnectCallback on_connect = nullptr,
                 OnMessageCallback on_message = nullptr,
                 OnDisconnectCallback on_disconnect = nullptr);

    void send(const std::string& message, const OnSendCallback& callback = nullptr);
    void close(const OnCloseCallback& callback = nullptr);

private:
    void handle_resolve_result(const boost::system::error_code& error_code,
                               const boost::asio::ip::tcp::resolver::iterator& endpoint);
    void handle_connect_result(const boost::system::error_code& error_code);
    void handle_tls_handshake_result(const boost::system::error_code& error_code);
    void handle_websocket_handshake_result(const boost::system::error_code& error_code);
    void listen_to_incoming_message(const boost::system::error_code& error_code);
    void handle_read_result(const boost::system::error_code& error_code,
                            size_t bytes_written);

    std::string host_url;
    std::string port;

    boost::asio::ip::tcp::resolver resolver;
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>
        websocket;

    boost::beast::flat_buffer read_buffer;

    OnConnectCallback on_connect_callback{nullptr};
    OnMessageCallback on_message_callback{nullptr};
    OnDisconnectCallback on_disconnect_callback{nullptr};
};

} // namespace network_monitor
