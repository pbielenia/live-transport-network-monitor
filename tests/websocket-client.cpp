#include "network-monitor/websocket-client.hpp"

#include "boost-mock.hpp"

#include <boost/asio.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <sstream>
#include <string>

using NetworkMonitor::MockResolver;
using NetworkMonitor::MockTcpStream;
using NetworkMonitor::MockSslStream;
using NetworkMonitor::MockWebSocketStream;
using NetworkMonitor::TestWebSocketClient;
using NetworkMonitor::WebSocketClient;

// This fixture is used to re-initialize all mock properties before a test.
struct WebSocketClientTestFixture {
    WebSocketClientTestFixture() {
        MockResolver::resolve_error_code = {};
        MockTcpStream::connect_error_code = {};
        MockSslStream<MockTcpStream>::handshake_error_code = {};
        MockWebSocketStream<MockSslStream<MockTcpStream>>::handshake_error_code = {};
    }
};

// Used to set a timeout on tests that may hang or suffer from a slow connection.
using timeout = boost::unit_test::timeout;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_WebSocketClient);

BOOST_AUTO_TEST_CASE(cacert_pem)
{
    BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
}

BOOST_FIXTURE_TEST_SUITE(Connect, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(fail_resolve, *timeout{1})
{
    const std::string url{"some.echo-server.com"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    MockResolver::resolve_error_code = boost::asio::error::host_not_found;

    TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

    bool called_on_connect{false};
    auto on_connect{[&called_on_connect](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK_EQUAL(error_code, MockResolver::resolve_error_code);
    }};

    client.Connect(on_connect);
    io_context.run();

    BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(fail_socket_connection, *timeout{1})
{
    const std::string url{"some.echo-server.com"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    MockTcpStream::connect_error_code = boost::asio::error::connection_refused;

    TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

    bool called_on_connect{false};
    auto on_connect{[&called_on_connect](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK_EQUAL(error_code, MockTcpStream::connect_error_code);
    }};

    client.Connect(on_connect);
    io_context.run();

    BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(fail_socket_handshake, *timeout{1})
{
    const std::string url{"some.echo-server.com"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    MockSslStream<MockTcpStream>::handshake_error_code = boost::asio::ssl::error::stream_truncated;

    TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

    bool called_on_connect{false};
    auto on_connect{[&called_on_connect](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK_EQUAL(error_code, MockSslStream<MockTcpStream>::handshake_error_code);
    }};

    client.Connect(on_connect);
    io_context.run();

    BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(fail_websocket_handshake, *timeout{1})
{
    // TODO: this could include MockWebSocketStream as well
    using MockTlsStream = MockSslStream<MockTcpStream>;

    const std::string url{"some.echo-server.com"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    MockWebSocketStream<MockTlsStream>::handshake_error_code = boost::beast::websocket::error::no_host;

    TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

    bool called_on_connect{false};
    auto on_connect{[&called_on_connect](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK_EQUAL(error_code, MockWebSocketStream<MockTlsStream>::handshake_error_code);
    }};

    client.Connect(on_connect);
    io_context.run();

    BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_SUITE_END(); // Connect

BOOST_AUTO_TEST_SUITE(live);

BOOST_AUTO_TEST_CASE(echo, *timeout{20})
{
    const std::string url{"ltnm.learncppthroughprojects.com"};
    const std::string endpoint{"/echo"};
    const std::string port{"443"};
    const std::string message{"Hello WebSocket"};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);

    boost::asio::io_context io_context{};
    NetworkMonitor::BoostWebSocketClient client{url, endpoint, port, io_context,
                                                 tls_context};

    bool connected{false};
    bool message_sent{false};
    bool message_received{false};
    bool message_matches{false};
    bool disconnected{false};
    std::string echo{};

    auto on_send{[&message_sent](auto error_code) { message_sent = !error_code; }};
    auto on_connect{[&client, &connected, &on_send, &message](auto error_code) {
        connected = !error_code.failed();
        if (connected) {
            client.Send(message, on_send);
        }
    }};
    auto on_close{[&disconnected](auto error_code) { disconnected = !error_code.failed(); }};
    auto on_receive{[&client, &on_close, &message_received, &message_matches, &message,
                    &echo](auto error_code, auto received) {
        message_received = !error_code.failed();
        echo = message;
        client.Close(on_close);
    }};

    client.Connect(on_connect, on_receive);
    io_context.run();

    BOOST_CHECK(connected);
    BOOST_CHECK(message_sent);
    BOOST_CHECK(message_received);
    BOOST_CHECK(disconnected);
    BOOST_CHECK_EQUAL(message, echo);
}

bool CheckResponse(const std::string& response)
{
    bool ok{true};
    ok &= response.find("ERROR") != std::string::npos;
    ok &= response.find("ValidationInvalidAuth") != std::string::npos;
    return ok;
}

BOOST_AUTO_TEST_CASE(send_stomp_frame)
{
    const std::string url{"ltnm.learncppthroughprojects.com"};
    const std::string endpoint{"/network-events"};
    const std::string port{"443"};
    const std::string username{"test"};
    const std::string password{"test"};

    std::stringstream stream;
    stream << "STOMP\n"
           << "accept-version:1.2\n"
           << "host:ltnm.learncppthroughprojects.com\n"
           << "login:" << username << "\n"
           << "passcode:" << password << "\n"
           << "\n"
           << '\0';

    const std::string message{stream.str()};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);

    boost::asio::io_context io_context{};
    NetworkMonitor::BoostWebSocketClient client{url, endpoint, port, io_context,
                                                 tls_context};

    bool connected{false};
    bool message_sent{false};
    bool message_received{false};
    bool disconnected{false};
    std::string response{};

    auto on_send_callback{[&message_sent](auto error_code) {
        message_sent = !error_code;
    }};

    auto on_connect_callback{[&client, &connected, &on_send_callback, &message](auto error_code) {
        connected = !error_code;
        if (connected) {
            client.Send(message, on_send_callback);
        }
    }};

    auto on_close_callback{[&disconnected](auto error_code) {
        disconnected = !error_code;
    }};

    auto on_receive_callback{[&client, &on_close_callback, &message_received, &response](auto error_code, auto received) {
        message_received = !error_code;
        response = std::move(received);
        client.Close(on_close_callback);
    }};

    client.Connect(on_connect_callback, on_receive_callback);
    io_context.run();

    BOOST_CHECK(connected);
    BOOST_CHECK(message_sent);
    BOOST_CHECK(message_received);
    BOOST_CHECK(disconnected);
    BOOST_CHECK(CheckResponse(response));
}

BOOST_AUTO_TEST_SUITE_END(); // live

BOOST_AUTO_TEST_SUITE_END(); // class_WebSocketClient

BOOST_AUTO_TEST_SUITE_END(); // network_monitor
