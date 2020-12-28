#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <network-monitor/websocket-client.hpp>
#include <string>

using network_monitor::WebSocketClient;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_CASE(cacert_pem)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
}

BOOST_AUTO_TEST_CASE(class_WebSocketClient)
{
    const std::string url{"echo.websocket.org"};
    const std::string port{"80"};
    const std::string message{"Hello WebSocket"};

    boost::asio::io_context io_context;

    network_monitor::WebSocketClient websocket_client(url, port, io_context);

    bool connected{false};
    bool message_sent{false};
    bool message_received{false};
    bool disconnected{false};
    std::string echo{};

    auto on_send = [&message_sent](auto error_code) { message_sent = !error_code; };

    auto on_connect = [&websocket_client, &connected, &on_send,
                       &message](auto error_code) {
        connected = !error_code;
        if (!error_code) {
            websocket_client.send(message, on_send);
        }
    };

    auto on_close = [&disconnected](auto error_code) { disconnected = !error_code; };

    auto on_receive = [&websocket_client, &on_close, &message_received,
                       &echo](auto error_code, auto received) {
        message_received = !error_code;
        echo = std::move(received);
        websocket_client.close(on_close);
    };

    websocket_client.connect(on_connect, on_receive);
    io_context.run();

    BOOST_CHECK(connected);
    BOOST_CHECK(message_sent);
    BOOST_CHECK(message_received);
    BOOST_CHECK(disconnected);
    BOOST_CHECK_EQUAL(message, echo);
}

BOOST_AUTO_TEST_SUITE_END();
