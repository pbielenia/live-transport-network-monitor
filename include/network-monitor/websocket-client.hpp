#pragma once

#include <iostream>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <functional>
#include <string>

namespace network_monitor {

/*! \brief Client to connect to a WebSocket server over TLS
 *
 * \tparam Resolver        The class to resolve the URL to an IP address. It must support
 *                         the same interface of boost::asio::ip::tcp::resolver.
 * \tparam WebSocketStream The WebSocket stream class. It must support the same interface
 *                         of boost::beast:websocket::stream.
 */
template<typename Resolver, typename WebSocketStream> class WebSocketClient {
public:
    using OnConnectCallback = std::function<void(boost::system::error_code)>;
    using OnMessageCallback =
        std::function<void(boost::system::error_code, std::string&&)>;
    using OnDisconnectCallback = std::function<void(boost::system::error_code)>;
    using OnSendCallback = std::function<void(boost::system::error_code)>;
    using OnCloseCallback = std::function<void(boost::system::error_code)>;

    WebSocketClient(std::string host_url,
                    std::string host_endpoint,
                    std::string port,
                    boost::asio::io_context& io_context,
                    boost::asio::ssl::context& ssl_context)
        : host_url{std::move(host_url)},
          host_endpoint{std::move(host_endpoint)},
          port{std::move(port)},
          resolver{boost::asio::make_strand(io_context)},
          websocket{boost::asio::make_strand(io_context), ssl_context}
    {
    }

    ~WebSocketClient() = default;

    void connect(OnConnectCallback on_connect = nullptr,
                 OnMessageCallback on_message = nullptr,
                 OnDisconnectCallback on_disconnect = nullptr)
    {
        on_connect_callback = std::move(on_connect);
        on_message_callback = std::move(on_message);
        on_disconnect_callback = std::move(on_disconnect);

        resolver.async_resolve(host_url, port, [this](auto error_code, auto endpoint) {
            handle_resolve_result(error_code, endpoint);
        });
    }

    void send(const std::string& message, const OnSendCallback& callback = nullptr)
    {
        websocket.async_write(boost::asio::buffer(message),
                              [callback](auto error_code, auto) {
                                  if (callback) {
                                      callback(error_code);
                                  }
                              });
    }

    void close(const OnCloseCallback& callback = nullptr)
    {
        websocket.async_close(boost::beast::websocket::close_code::none,
                              [callback](auto error_code) {
                                  if (callback) {
                                      callback(error_code);
                                  }
                              });
    }

private:
    void handle_resolve_result(const boost::system::error_code& error_code,
                               const boost::asio::ip::tcp::resolver::iterator& endpoint)
    {
        if (error_code) {
            log(__func__, error_code);
            if (on_connect_callback) {
                on_connect_callback(error_code);
            }
            return;
        }

        auto& tcp_layer = websocket.next_layer().next_layer();
        tcp_layer.expires_after(std::chrono::seconds(5));
        tcp_layer.async_connect(
            *endpoint, [this](auto error_code) { handle_connect_result(error_code); });
    }

    void handle_connect_result(const boost::system::error_code& error_code)
    {
        if (error_code) {
            log(__func__, error_code);
            if (on_connect_callback) {
                on_connect_callback(error_code);
            }
            return;
        }

        const auto suggested_timeout =
            boost::beast::websocket::stream_base::timeout::suggested(
                boost::beast::role_type::client);

        auto& tcp_layer = websocket.next_layer().next_layer();
        tcp_layer.expires_never();
        websocket.set_option(suggested_timeout);

        websocket.next_layer().async_handshake(
            boost::asio::ssl::stream_base::client,
            [this](auto error_code) { handle_tls_handshake_result(error_code); });
    }

    void handle_tls_handshake_result(const boost::system::error_code& error_code)
    {
        if (error_code) {
            log(__func__, error_code);
            if (on_connect_callback) {
                on_connect_callback(error_code);
            }
            return;
        }

        websocket.async_handshake(host_url, host_endpoint, [this](auto error_code) {
            handle_websocket_handshake_result(error_code);
        });
    }

    void handle_websocket_handshake_result(const boost::system::error_code& error_code)
    {
        if (error_code) {
            log(__func__, error_code);
            if (on_connect_callback) {
                on_connect_callback(error_code);
            }
            return;
        }

        websocket.text(true);
        listen_to_incoming_message(error_code);

        if (on_connect_callback) {
            on_connect_callback(error_code);
        }
    }

    void listen_to_incoming_message(const boost::system::error_code& error_code)
    {
        if (error_code == boost::asio::error::operation_aborted) {
            if (on_disconnect_callback) {
                on_disconnect_callback(error_code);
            }
            return;
        }

        websocket.async_read(read_buffer, [this](auto error_code, auto bytes_written) {
            handle_read_result(error_code, bytes_written);
            listen_to_incoming_message(error_code);
        });
    }

    void handle_read_result(const boost::system::error_code& error_code,
                            size_t bytes_written)
    {
        if (error_code) {
            return;
        }

        std::string message{boost::beast::buffers_to_string(read_buffer.data())};
        read_buffer.consume(bytes_written);
        if (on_message_callback) {
            on_message_callback(error_code, std::move(message));
        }
    }

    void log(const std::string& where, boost::system::error_code error_code)
    {
        std::cerr << "[" << std::setw(20) << where << "] "
                  << (error_code ? "Error: " : "OK")
                  << (error_code ? error_code.message() : "") << std::endl;
    }

    std::string host_url;
    std::string host_endpoint;
    std::string port;

    Resolver resolver;
    WebSocketStream websocket;

    boost::beast::flat_buffer read_buffer;

    OnConnectCallback on_connect_callback{nullptr};
    OnMessageCallback on_message_callback{nullptr};
    OnDisconnectCallback on_disconnect_callback{nullptr};
};

using BoostWebSocketClient = WebSocketClient<
    boost::asio::ip::tcp::resolver,
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>;

} // namespace network_monitor
