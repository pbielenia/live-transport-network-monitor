#include "websocket-client.hpp"

#include <openssl/ssl.h>
#include <chrono>
#include <iomanip>
#include <iostream>

using namespace network_monitor;

static void log(const std::string& where, boost::system::error_code error_code)
{
    std::cerr << "[" << std::setw(20) << where << "] " << (error_code ? "Error: " : "OK")
              << (error_code ? error_code.message() : "") << std::endl;
}

WebSocketClient::WebSocketClient(std::string host_url,
                                 std::string port,
                                 boost::asio::io_context& io_context)
    : host_url{std::move(host_url)},
      port{std::move(port)},
      resolver{boost::asio::make_strand(io_context)},
      websocket{boost::asio::make_strand(io_context)}
{
}

void WebSocketClient::connect(OnConnectCallback on_connect,
                              OnMessageCallback on_message,
                              OnDisconnectCallback on_disconnect)
{
    on_connect_callback = std::move(on_connect);
    on_message_callback = std::move(on_message);
    on_disconnect_callback = std::move(on_disconnect);

    resolver.async_resolve(host_url, port, [this](auto error_code, auto endpoint) {
        handle_resolve_result(error_code, endpoint);
    });
}

void WebSocketClient::handle_resolve_result(
    const boost::system::error_code& error_code,
    const boost::asio::ip::tcp::resolver::iterator& endpoint)
{
    if (error_code) {
        log(__func__, error_code);
        if (on_connect_callback) {
            on_connect_callback(error_code);
        }
        return;
    }
    websocket.next_layer().expires_after(std::chrono::seconds(5));
    websocket.next_layer().async_connect(
        *endpoint, [this](auto error_code) { handle_connect_result(error_code); });
}

void WebSocketClient::handle_connect_result(const boost::system::error_code& error_code)
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

    websocket.next_layer().expires_never();
    websocket.set_option(suggested_timeout);
    websocket.async_handshake(
        host_url, "/", [this](auto error_code) { handle_handshake_result(error_code); });
}

void WebSocketClient::handle_handshake_result(const boost::system::error_code& error_code)
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

void WebSocketClient::listen_to_incoming_message(
    const boost::system::error_code& error_code)
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

void WebSocketClient::handle_read_result(const boost::system::error_code& error_code,
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

void WebSocketClient::send(const std::string& message, const OnSendCallback& callback)
{
    websocket.async_write(boost::asio::buffer(message),
                          [callback](auto error_code, auto) {
                              if (callback) {
                                  callback(error_code);
                              }
                          });
}

void WebSocketClient::close(const OnCloseCallback& callback)
{
    websocket.async_close(boost::beast::websocket::close_code::none,
                          [callback](auto error_code) {
                              if (callback) {
                                  callback(error_code);
                              }
                          });
}
