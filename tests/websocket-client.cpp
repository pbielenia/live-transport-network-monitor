#include "websocket-client.hpp"

#include <boost/asio.hpp>
#include <iostream>
#include <string>

int main()
{
    const std::string url{"echo.websocket.org"};
    const std::string port{"80"};
    const std::string message{"Hello WebSocket"};

    boost::asio::io_context io_context;

    network_monitor::WebSocketClient websocket_client(url, port, io_context);

    bool connected{false};
    bool message_sent{false};
    bool message_received{false};
    bool message_matches{false};
    bool disconnected{false};

    auto on_send = [&message_sent](auto error_code) {
        message_sent = !error_code; };

    auto on_connect = [&websocket_client, &connected, &on_send,
                       &message](auto error_code) {
        connected = !error_code;
        if (!error_code) {
            websocket_client.send(message, on_send);
        }
    };

    auto on_close = [&disconnected](auto error_code) { disconnected = !error_code; };

    auto on_receive = [&websocket_client, &on_close, &message_received, &message_matches,
                       &message](auto error_code, auto received) {
        message_received = !error_code;
        message_matches = message == received;
        websocket_client.close(on_close);
    };

    websocket_client.connect(on_connect, on_receive);
    io_context.run();

    bool ok =
        connected & message_sent & message_received & message_matches & disconnected;

    std::cout << "connected: " << std::boolalpha << connected << "\n"
              << "message_sent: " << std::boolalpha << message_sent << "\n"
              << "message_received: " << std::boolalpha << message_received << "\n"
              << "message_matches: " << std::boolalpha << message_matches << "\n"
              << "disconnected: " << std::boolalpha << disconnected << "\n";

    if (ok) {
        std::cout << "OK\n";
        return 0;
    } else {
        std::cerr << "Test failed\n";
        return 1;
    }

    //    boost::beast::websocket::response_type response;
    //    websocket.async_handshake(response, "echo.websocket.org", "/",
    //                              std::bind(&on_handshake, std::placeholders::_1,
    //                                        std::ref(websocket), std::cref(message)));
    //    if (error_code) {
    //        log(error_code);
    //        return -1;
    //    }
}
