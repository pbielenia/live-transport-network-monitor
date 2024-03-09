#include "websocket-client-mock.hpp"

#include "network-monitor/stomp-frame-builder.hpp"

using namespace NetworkMonitor;

inline std::string WebSocketClientMockForStomp::username{};
inline std::string WebSocketClientMockForStomp::password{};
inline boost::system::error_code WebSocketClientMock::connect_error_code{};
inline boost::system::error_code WebSocketClientMock::send_error_code{};
inline boost::system::error_code WebSocketClientMock::close_error_code{};
inline std::queue<std::string> WebSocketClientMock::message_queue{};
inline bool WebSocketClientMock::trigger_disconnection{false};
inline std::function<void(const std::string&)> WebSocketClientMock::respond_to_send{
    nullptr};

WebSocketClientMock::WebSocketClientMock(const std::string& url,
                                         const std::string& endpoint,
                                         const std::string& port,
                                         boost::asio::io_context& io_context,
                                         boost::asio::ssl::context& tls_context)
    : io_context_{io_context},
      server_url_{url}
{
}

void WebSocketClientMock::Connect(
    std::function<void(boost::system::error_code)> on_connected_callback,
    std::function<void(boost::system::error_code, std::string&&)> on_message_callback,
    std::function<void(boost::system::error_code)> on_disconnected_callback)
{
    // TODO: pass error code in `on_connected_callback_` and do not follow with
    // `MockIncomingMessages`
    if (!connect_error_code.failed()) {
        connected_ = true;
        on_message_callback_ = std::move(on_message_callback);
        on_disconnected_callback_ = std::move(on_disconnected_callback);
    }

    boost::asio::post(io_context_, [this, on_connected_callback]() {
        if (on_connected_callback) {
            on_connected_callback(connect_error_code);
        }
    });

    boost::asio::post(io_context_, [this]() { MockIncomingMessages(); });
}

void WebSocketClientMock::Send(
    const std::string& message,
    std::function<void(boost::system::error_code)> on_sent_callback)
{
    if (!connected_) {
        boost::asio::post(io_context_, [on_sent_callback]() {
            if (on_sent_callback) {
                on_sent_callback(boost::asio::error::operation_aborted);
            }
        });
        return;
    }

    boost::asio::post(io_context_, [this, on_sent_callback, message]() {
        if (on_sent_callback) {
            on_sent_callback(send_error_code);
            respond_to_send(message);
        }
    });
}

void WebSocketClientMock::Close(
    std::function<void(boost::system::error_code)> on_close_callback)
{
    if (connected_) {
        boost::asio::post(io_context_, [this, on_close_callback]() {
            connected_ = false;
            // TODO: closed_ = true;
            trigger_disconnection = true;
            if (on_close_callback) {
                on_close_callback(close_error_code);
            }
        });
    } else {
        boost::asio::post(io_context_, [on_close_callback]() {
            if (on_close_callback) {
                on_close_callback(boost::asio::error::operation_aborted);
            }
        });
    }
}

const std::string& WebSocketClientMock::GetServerUrl() const
{
    return server_url_;
}

void WebSocketClientMock::MockIncomingMessages()
{
    if (!connected_ || trigger_disconnection) {
        trigger_disconnection = false;
        boost::asio::post(io_context_, [this]() {
            on_disconnected_callback_(boost::asio::error::operation_aborted);
        });
        return;
    }

    boost::asio::post(io_context_, [this]() {
        if (!message_queue.empty()) {
            auto message{message_queue.front()};
            message_queue.pop();
            if (on_message_callback_) {
                on_message_callback_({}, std::move((message)));
            }
        }
        MockIncomingMessages();
    });
}

WebSocketClientMockForStomp::WebSocketClientMockForStomp(
    const std::string& url,
    const std::string& endpoint,
    const std::string& port,
    boost::asio::io_context& io_context,
    boost::asio::ssl::context& tls_context)
    : WebSocketClientMock(url, endpoint, port, io_context, tls_context)
{
    respond_to_send = [this](auto message) { OnMessage(message); };
}

void WebSocketClientMockForStomp::OnMessage(const std::string& message)
{
    StompError error;
    StompFrame frame{error, message};
    if (error != StompError::Ok) {
        // TODO: trigger disconnection
        return;
    }

    // TODO: log frame command
    switch (frame.GetCommand()) {
        case StompCommand::Stomp:
        case StompCommand::Connect: {
            if (ConnectFrameIsAuthenticated(frame)) {
                const std::string stomp_version{"1.4"};
                message_queue.push(
                    stomp_frame::MakeConnectedFrame(stomp_version, {}, {}, {})
                        .ToString());
                // TODO: else MakeErrorFrame and trigger disconnection
                break;
            }
        }
        case StompCommand::Subscribe: {
            // TODO
            break;
        }
        default: {
            break;
        }
    }
}

bool WebSocketClientMockForStomp::ConnectFrameIsAuthenticated(
    const NetworkMonitor::StompFrame& frame)
{
    using namespace NetworkMonitor;
    if (!frame.HasHeader(StompHeader::Login) || !frame.HasHeader(StompHeader::Passcode)) {
        return false;
    }
    bool authentication_matches{frame.GetHeaderValue(StompHeader::Login) == username &&
                                frame.GetHeaderValue(StompHeader::Passcode) == password};
    return authentication_matches;
}