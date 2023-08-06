#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <network-monitor/stomp-frame.hpp>
#include <string>

namespace NetworkMonitor {

/*! \brief Error codes for the STOMP client.
 */
enum class StompClientError {
    kOk = 0,
    // TODO
};

/*! \brief STOMP client implementing the subset of commands needed by the network-events
 *         service.
 *
 *  \param WebSocketClient      WebSocket client class. This type must have the same
 *                              interface of WebSocketClient.
 */
template <typename WebSocketClient>
class StompClient {
   public:
    /*! \brief Construct a STOMP client connecting to a remote URL/port through a secure
     *         WebSocket connection.
     *
     *  \note This constructor does not initiate a connection.
     *
     *  \param url          The URL of the server.
     *  \param endpoint     The endpoint on the sever to connect.
     *                      Example: ltnm.learncppthroughprojects.com/<endpoint>.
     *  \param port         The port on the server.
     *  \param io_context   The io_context object. The user takes care of calling
     *                      io_context.run().
     *  \param tls_context  The TLS context to setup a TLS socket stream.
     */
    StompClient(const std::string& url,
                const std::string& endpoint,
                const std::string& port,
                boost::asio::io_context& io_context,
                boost::asio::ssl::context& tls_context);

    /*! \brief Connect to the STOMP server.
     *
     *  \param on_connect       Called when the connection fails or succeeds.
     *  \param on_message       Called when a message arrives.
     *  \param on_disconnect    Called when the connection is closed by the server or due
     *                          to a connection error.
     */
    // TODO: What error objects should be used? In case of boost::system::error_code it's
    //       not possible to include StompFrame parsing errors. Maybe the right choice is
    //       to make error enums for this class. Have to decide on details yet.
    void Connect(
        std::function<void(boost::system::error_code)> on_connected_callback = nullptr,
        std::function<void(boost::system::error_code, StompFrame&&)> on_message_callback =
            nullptr,
        std::function<void(boost::system::error_code)> on_disconnected_callback =
            nullptr);

    /*! \brief Close the STOMP and WebSocket connection.
     *
     *  \param on_close  Called when the connection is closed, successfully or not.
     */
    void Close(std::function<void(boost::system::error_code)> on_close = nullptr);

    /*! \brief Subscribe to a STOMP endpoint.
     *
     *   \returns The subscripton ID.
     */
    std::string Subscribe(
        const std::string& destination,
        std::function<void(boost::system::error_code)> on_subscribed_callback = nullptr);

   private:
    void OnWebSocketConnected(boost::system::error_code result);
    void OnMessageReceived(boost::system::error_code result, std::string&& message);
    void OnMessageSent(boost::system::error_code result);
    void OnWebSocketDisconnected(boost::system::error_code result);

    std::function<void(boost::system::error_code)> on_connected_callback_;
    std::function<void(boost::system::error_code, StompFrame&&)> on_message_callback_;
    std::function<void(boost::system::error_code)> on_disconnected_callback_;
    std::function<void(boost::system::error_code)> on_closed_callback_;

    WebSocketClient websocket_client_;
    boost::asio::io_context& io_context_;

    bool websocket_connected_{false};
};

template <typename WebSocketClient>
StompClient<WebSocketClient>::StompClient(const std::string& url,
                                          const std::string& endpoint,
                                          const std::string& port,
                                          boost::asio::io_context& io_context,
                                          boost::asio::ssl::context& tls_context)
    : websocket_client_{url, endpoint, port, io_context, tls_context},
      io_context_{io_context}
{
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::Connect(
    std::function<void(boost::system::error_code)> on_connected_callback,
    std::function<void(boost::system::error_code, StompFrame&&)> on_message_callback,
    std::function<void(boost::system::error_code)> on_disconnected_callback)
{
    if (on_connected_callback) {
        on_connected_callback_ = std::move(on_connected_callback);
    }
    if (on_message_callback) {
        on_message_callback_ = std::move(on_message_callback);
    }
    if (on_disconnected_callback) {
        on_disconnected_callback_ = std::move(on_disconnected_callback);
    }
    websocket_client_.Connect([this](auto result) { OnWebSocketConnected(result); },
                              [this](auto result, auto&& message) {
                                  OnMessageReceived(result, std::move(message));
                              },
                              [this](auto result) { OnWebSocketDisconnected(result); });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::Close(
    std::function<void(boost::system::error_code)> on_closed)
{
}

template <typename WebSocketClient>
std::string StompClient<WebSocketClient>::Subscribe(
    const std::string& destination,
    std::function<void(boost::system::error_code)> on_subscribed_callback)
{
    return {};
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketConnected(boost::system::error_code result)
{
    if (result.failed()) {
        boost::asio::post(io_context_, [result, this]() {
            if (on_connected_callback_) {
                on_connected_callback_(result);
            }
        });
        return;
    }

    websocket_connected_ = true;

    // TODO: make a CONNECT StompFrame - would be nice to have a possibility to set up
    //                                   a StompFrame
    // TODO: send the StompFrame with websocket_client_.Send(message, ) -
    //       StompFrame will have to be converted to std::string first

    boost::asio::post(io_context_, [result, this]() {
        if (on_connected_callback_) {
            on_connected_callback_(result);
        }
    });
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnMessageReceived(boost::system::error_code result,
                                                     std::string&& message)
{
    if (result.failed()) {
        if (on_message_callback_) {
            on_message_callback_(result, {});
        }
    }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnMessageSent(boost::system::error_code result)
{
    //
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketDisconnected(
    boost::system::error_code result)
{
    //
}

}  // namespace NetworkMonitor
