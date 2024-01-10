#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <network-monitor/stomp-frame.hpp>
#include <network-monitor/stomp-frame-builder.hpp>
#include <string>

namespace NetworkMonitor {

/*! \brief Error codes for the STOMP client.
 */
enum class StompClientError {
    Ok = 0,
    CouldNotConnectToWebSocketServer,
    UnexpectedCouldNotCreateValidFrame,
    CouldNotSendConnectFrame
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
    // TODO: add brief for user_name and user_password
    void Connect(
        const std::string& user_name,
        const std::string& user_password,
        std::function<void(boost::system::error_code)> on_connected_callback = nullptr,
        // std::function<void(boost::system::error_code, StompFrame&&)> on_message_callback =
        //     nullptr,
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
    void OnWebSocketConnectMessageSent(boost::system::error_code result);
    void OnWebSocketMessageReceived(boost::system::error_code result, std::string&& message);
    void OnWebSocketMessageSent(boost::system::error_code result);
    void OnWebSocketDisconnected(boost::system::error_code result);

    void CallOnConnectedCallbackWithErrorIfExist(StompClientError error);

    std::function<void(StompClientError)> on_connected_callback_;
    std::function<void(StompClientError, StompFrame&&)> on_message_callback_;
    std::function<void(StompClientError)> on_disconnected_callback_;
    std::function<void(StompClientError)> on_closed_callback_;

    const std::string& user_name_;
    const std::string& user_password_;

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
    const std::string& user_name,
    const std::string& user_password,
    std::function<void(boost::system::error_code)> on_connected_callback,
    // std::function<void(boost::system::error_code, StompFrame&&)> on_message_callback,
    std::function<void(boost::system::error_code)> on_disconnected_callback)
{
    // add log StompClient: Connecting to STOMP server
    user_name_ = user_name;
    user_password_ = user_password;
    on_connected_callback_ = std::move(on_connected_callback);
    // on_message_callback_ = std::move(on_message_callback);m
    on_disconnected_callback_ = std::move(on_disconnected_callback);

    websocket_client_.Connect([this](auto result) { OnWebSocketConnected(result); },
                              [this](auto result, auto&& message) {
                                  OnWebSocketMessageReceived(result, std::move(message));
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
        // TODO: put a log "StompClient: Could not connect to server: {result.message()}"
        // TODO: The user callbacks for the STOMP client should not run in the WebSocket execution environments.
        //       In other words, you should dispatch the user callbacks from a separate strand. This avoids
        //       unnecessary synchronization barriers between the WebSockets and STOMP code.
        //       - seems to be fine now (10.01.2023)
        CallOnConnectedCallbackWithErrorIfExist(StompClientError::CouldNotConnectToWebSocketServer);
        return;
    }

    websocket_connected_ = true;

    stomp_frame::BuildParameters parameters(StompCommand::Connect);
    parameters.headers.emplace(StompHeader::AcceptVersion, "1.2");
    parameters.headers.emplace(StompHeader::Host, websocket_client_.GetServerUrl());
    parameters.headers.emplace(StompHeader::Login, user_name_);
    parameters.headers.emplace(StompHeader::Passcode, user_password_);

    StompError error;
    StompFrame frame = stomp_frame::Build(error, parameters);
    if (error != StompError::Ok) {
        // add log StompClient: Could not create a valid frame: {error}
        CallOnConnectedCallbackWithErrorIfExist(StompClientError::UnexpectedCouldNotCreateValidFrame);
        return;
    }

    websocket_client_.Send(frame.ToString(), [this](auto result){
        OnWebSocketConnectMessageSent(result);
    });
    
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketConnectMessageSent(boost::system::error_code result)
{
    if (result.failed()) {
        // add log StompClient: Could not send STOMP frame: {result.message()}
        CallOnConnectedCallbackWithErrorIfExist(StompClientError::CouldNotSendConnectFrame)
    }
}


template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketMessageReceived(boost::system::error_code result,
                                                              std::string&& message)
{
    if (result.failed()) {
        if (on_message_callback_) {
            on_message_callback_(result, {});
        }
    }
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketMessageSent(boost::system::error_code result)
{
    //
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::OnWebSocketDisconnected(
    boost::system::error_code result)
{
    //
}

template <typename WebSocketClient>
void StompClient<WebSocketClient>::CallOnConnectedCallbackWithErrorIfExist(StompClientError error)
{
    if (on_connected_callback_) {
        boost::asio::post(io_context_, [&on_connected_callback_, error]() {
            on_connected_callback_(error);
        });
    }
}

}  // namespace NetworkMonitor
