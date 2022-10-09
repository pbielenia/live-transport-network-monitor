#include "network-monitor/websocket-client.h"

#include <openssl/ssl.h>

using namespace NetworkMonitor;

WebSocketClient::WebSocketClient(const std::string& url,
                                 const std::string& endpoint,
                                 const std::string& port,
                                 boost::asio::io_context& io_context)
    : server_url_{url},
      server_endpoint_{endpoint},
      server_port_{port},
      resolver_{boost::asio::make_strand(io_context)},
      websocket_stream_{boost::asio::make_strand(io_context)}
{
}

WebSocketClient::~WebSocketClient() {}

void WebSocketClient::Connect(
    std::function<void(boost::system::error_code)> on_connect,
    std::function<void(boost::system::error_code, std::string&&)> on_message,
    std::function<void(boost::system::error_code)> on_disconnect)
{
    SaveProvidedCallbacks(std::move(on_connect), std::move(on_message),
                          std::move(on_disconnect));
    ResolveServerUrl();
}

void WebSocketClient::SaveProvidedCallbacks(
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

void WebSocketClient::ResolveServerUrl()
{
    resolver_.async_resolve(server_url_, server_port_,
                            [this](const boost::system::error_code& error,
                                   boost::asio::ip::tcp::resolver::results_type results) {
                                if (error) {
                                    return;
                                }
                                OnServerUrlResolved(results);
                            });
}

void WebSocketClient::OnServerUrlResolved(
    boost::asio::ip::tcp::resolver::results_type results)
{
    ConnectToServer(results);
}

void WebSocketClient::ConnectToServer(
    boost::asio::ip::tcp::resolver::results_type results)
{
    websocket_stream_.next_layer().async_connect(
        *results, [this](auto error) { OnConnectedToServer(error); });
}

void WebSocketClient::OnConnectedToServer(const boost::system::error_code& error)
{
    Handshake();
}

void WebSocketClient::Handshake()
{
    const std::string connection_host{server_url_ + ':' + server_port_};
    websocket_stream_.async_handshake(
        connection_host, server_endpoint_,
        [this](const boost::system::error_code& error) { OnHandshakeCompleted(error); });
}

void WebSocketClient::OnHandshakeCompleted(const boost::system::error_code& error)
{
    CallOnConnectCallbackIfExists(error);
    ListenToIncomingMessage(error);
    closed_ = false;
}

void WebSocketClient::CallOnConnectCallbackIfExists(
    const boost::system::error_code& error)
{
    if (on_connect_callback_) {
        on_connect_callback_(error);
    }
}

void WebSocketClient::ListenToIncomingMessage(const boost::system::error_code& error)
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

void WebSocketClient::OnMessageReceived(const boost::system::error_code& error,
                                        const size_t received_bytes_count)
{
    if (error) {
        return;
    }
    CallOnMessageCallbackIfExists(error, ReadMessage(received_bytes_count));
    ListenToIncomingMessage(error);
}

std::string WebSocketClient::ReadMessage(const size_t received_bytes_count)
{
    std::string message{boost::beast::buffers_to_string(response_buffer_.data())};
    response_buffer_.consume(received_bytes_count);
    return message;
}

void WebSocketClient::CallOnMessageCallbackIfExists(
    const boost::system::error_code& error, std::string&& message)
{
    if (on_message_callback_) {
        on_message_callback_(error, std::move(message));
    }
}

void WebSocketClient::Send(
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

void WebSocketClient::Close(
    std::function<void(boost::system::error_code)> on_close_callback)
{
    websocket_stream_.async_close(boost::beast::websocket::close_code::none,
                                  on_close_callback);
    closed_ = true;
}
