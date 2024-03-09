#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-client.hpp>
#include <network-monitor/stomp-frame-builder.hpp>
#include <queue>

#include "websocket-client-mock.hpp"

using NetworkMonitor::StompCommand;
using NetworkMonitor::StompError;
using NetworkMonitor::StompFrame;
using NetworkMonitor::StompHeader;
using NetworkMonitor::WebSocketClientMock;
using NetworkMonitor::WebSocketClientMockForStomp;

struct StompClientTestFixture {
    StompClientTestFixture()
    {
        WebSocketClientMockForStomp::username = "John";
        WebSocketClientMockForStomp::password = "1234";
        WebSocketClientMock::connect_error_code = {};
        WebSocketClientMock::send_error_code = {};
    }
};

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_FIXTURE_TEST_SUITE(stomp_client, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(CallsOnConnectOnSuccess)
{
    const std::string url{"some.echo-server.com"};
    const std::string endpoint{"/"};
    const std::string port{"443"};
    const std::string username{"John"};
    const std::string password{"1234"};

    WebSocketClientMockForStomp::username = username;
    WebSocketClientMockForStomp::password = password;

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    bool on_connected_called{false};

    NetworkMonitor::StompClient<WebSocketClientMockForStomp> stomp_client{
        url, endpoint, port, io_context, tls_context};

    auto on_connect_callback = [&on_connected_called, &stomp_client](auto result) {
        on_connected_called = true;
        BOOST_CHECK_EQUAL(result, NetworkMonitor::StompClientError::Ok);
        stomp_client.Close();
    };
    stomp_client.Connect(username, password, on_connect_callback);
    io_context.run();

    BOOST_CHECK(on_connected_called);
}

// BOOST_AUTO_TEST_CASE(CallsOnConnectOnFailure)
// {
//     const std::string url{"some.echo-server.com"};
//     const std::string endpoint{"/"};
//     const std::string port{"443"};

//     boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
//     tls_context.load_verify_file(TESTS_CACERT_PEM);
//     boost::asio::io_context io_context{};

//     WebSocketClientMock::connect_error_code = boost::asio::error::host_not_found;

//     bool on_connected_called{false};

//     auto on_connect_callback = [&on_connected_called](auto result) {
//         on_connected_called = true;
//         BOOST_CHECK_EQUAL(result, WebSocketClientMock::connect_error_code);
//     };

//     NetworkMonitor::StompClient<WebSocketClientMock> stomp_client{
//         url, endpoint, port, io_context, tls_context};
//     stomp_client.Connect(on_connect_callback);
//     io_context.run();

//     BOOST_CHECK(on_connected_called);
// }

/* StompClient::Connect()
 * - on_connected invoked with success
 * - on_connected invoked with failure
 * - on_message invoked - it needs the Subscribe() first, isn't it?
 * - on_disconnect invoked
 *
 * StompClient::Close()
 * - on_close invoked
 *
 * StompClient::Subscribe()
 * - on_subscribe called success
 * - on_subscribe called failure
 *
 *
 * - on_subscribe + on_message
 */

// TODO: trigger client/server disconnection (which one? maybe both in separate test
// cases?)

BOOST_AUTO_TEST_SUITE_END();  // stomp_client

BOOST_AUTO_TEST_SUITE_END();  // network_monitor