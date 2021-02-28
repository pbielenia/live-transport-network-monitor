#include "boost-mock.hpp"

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/test/unit_test.hpp>
#include <chrono>
#include <filesystem>
#include <network-monitor/websocket-client.hpp>
#include <string>

using network_monitor::BoostWebSocketClient;
using network_monitor::MockResolver;
using network_monitor::MockTcpStream;
using network_monitor::MockWebSocketClient;

struct WebSocketClientTestFixture {
    WebSocketClientTestFixture()
    {
        MockResolver::resolve_error_code = {};
        MockTcpStream::connect_error_code = {};
    }
};

static boost::unit_test::timeout timeout{3};

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_WebSocketClient);

BOOST_AUTO_TEST_CASE(cacert_pem)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
}

BOOST_FIXTURE_TEST_SUITE(Connect, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(fail_resolve, *timeout)
{
    const std::string url{"echo.websocket.org"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context ssl_context{boost::asio::ssl::context::tlsv12_client};
    ssl_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    MockResolver::resolve_error_code = boost::asio::error::host_not_found;

    MockWebSocketClient client{url, endpoint, port, io_context, ssl_context};
    bool called_on_connect{false};
    auto on_connect{[&called_on_connect](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK_EQUAL(error_code, boost::asio::error::host_not_found);
    }};
    client.connect(on_connect);
    io_context.run();

    BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(fail_socket_connect, *timeout)
{
    // We use the mock client so we don't really connect to the target.
    const std::string url{"echo.websocket.org"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context ssl_context{boost::asio::ssl::context::tlsv12_client};
    ssl_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    // Set the expected error codes.
    MockTcpStream::connect_error_code = boost::asio::error::connection_refused;

    MockWebSocketClient client{url, endpoint, port, io_context, ssl_context};
    bool called_on_connect{false};
    auto on_connect{[&called_on_connect](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK_EQUAL(error_code, boost::asio::error::connection_refused);
    }};
    client.connect(on_connect);
    io_context.run();

    // When we get here, the io_context::run function has run out of work to do.
    BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_SUITE_END(); // Connect

BOOST_AUTO_TEST_SUITE(live);

BOOST_AUTO_TEST_CASE(echo_websocket_org, *timeout)
{
    const std::string url{"echo.websocket.org"};
    const std::string endpoint{"/"};
    const std::string port{"443"};
    const std::string message{"Hello WebSocket"};

    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context{boost::asio::ssl::context::tlsv12_client};
    ssl_context.load_verify_file(TESTS_CACERT_PEM);

    network_monitor::BoostWebSocketClient websocket_client(url, endpoint, port,
                                                           io_context, ssl_context);

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

bool check_response(const std::string& response)
{
    bool is_ok{true};
    is_ok &= response.find("ERROR") != std::string::npos;
    is_ok &= response.find("ValidationInvalidAuth") != std::string::npos;
    return is_ok;
}

BOOST_AUTO_TEST_CASE(network_events, *timeout)
{
    const std::string url{"ltnm.learncppthroughprojects.com"};
    const std::string endpoint{"/network-events"};
    const std::string port{"443"};

    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context{boost::asio::ssl::context::tlsv12_client};
    ssl_context.load_verify_file(TESTS_CACERT_PEM);

    network_monitor::BoostWebSocketClient websocket_client(url, endpoint, port,
                                                           io_context, ssl_context);

    bool connected{false};
    bool message_sent{false};
    bool message_received{false};
    bool disconnected{false};
    std::string response{};

    const std::string username{"fake_username"};
    const std::string password{"fake_password"};

    std::stringstream ss{};
    ss << "STOMP"
       << "\n"
       << "accept-version:1.2"
       << "\n"
       << "host:transportforlondon.com"
       << "\n"
       << "login:" << username << "\n"
       << "passcode:" << password << "\n"
       << "\n"
       << '\0';
    const std::string message{ss.str()};

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
                       &response](auto error_code, auto received) {
        message_received = !error_code;
        response = std::move(received);
        websocket_client.close(on_close);
    };

    websocket_client.connect(on_connect, on_receive);
    io_context.run();

    BOOST_CHECK(connected);
    BOOST_CHECK(message_sent);
    BOOST_CHECK(message_received);
    BOOST_CHECK(disconnected);
    BOOST_CHECK(check_response(response));
}

BOOST_AUTO_TEST_SUITE_END(); // live

BOOST_AUTO_TEST_SUITE_END(); // class_WebSocketClient

BOOST_AUTO_TEST_SUITE_END(); // network_monitor
