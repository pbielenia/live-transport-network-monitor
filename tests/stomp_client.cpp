#include "network_monitor/stomp_client.hpp"

#include <string>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include "websocket_client_mock.hpp"

namespace network_monitor {

namespace {

using timeout = boost::unit_test::timeout;

struct StompClientTestFixture {
  StompClientTestFixture();

  static void SetStompCredentials(const std::string& username,
                                  const std::string& password);
  static const std::string& GetStompUsername();
  static const std::string& GetStompPassword();

  std::string url{"some.echo-server.com"};
  std::string endpoint{"/"};
  std::string port{"443"};
  std::string stomp_username;
  std::string stomp_password;
  std::string stomp_endpoint;

  boost::asio::ssl::context tls_context;
  boost::asio::io_context io_context;
};

StompClientTestFixture::StompClientTestFixture()
    : stomp_username{"correct_username"},
      stomp_password{"correct_password"},
      stomp_endpoint{"correct_endpoint"},
      tls_context{boost::asio::ssl::context::tlsv12_client} {
  WebSocketClientMock::Config::connect_error_code_ = {};
  WebSocketClientMock::Config::send_error_code_ = {};
  WebSocketClientMock::Config::close_error_code_ = {};
  WebSocketClientMockForStomp::username = stomp_username;
  WebSocketClientMockForStomp::password = stomp_password;
  WebSocketClientMockForStomp::endpoint = stomp_endpoint;

  tls_context.load_verify_file(TESTS_CACERT_PEM);
}

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_FIXTURE_TEST_SUITE(stomp_client, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(CallsOnConnectOnSuccess, *timeout(1)) {
  bool on_connected_called{false};

  StompClient<WebSocketClientMockForStomp> stomp_client{
      url, endpoint, port, io_context, tls_context};

  auto on_connect_callback = [&on_connected_called,
                              &stomp_client](auto result) {
    on_connected_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
    stomp_client.Close();
  };
  stomp_client.Connect(stomp_username, stomp_password, on_connect_callback);
  io_context.run();

  BOOST_CHECK(on_connected_called);
}

BOOST_AUTO_TEST_CASE(CallsOnConnectOnWebSocketConnectionFailure, *timeout(1)) {
  WebSocketClientMock::Config::connect_error_code_ =
      boost::asio::ssl::error::stream_truncated;

  bool on_connected_called{false};

  StompClient<WebSocketClientMockForStomp> stomp_client{
      url, endpoint, port, io_context, tls_context};

  auto on_connect_callback = [&on_connected_called,
                              &stomp_client](auto result) {
    on_connected_called = true;
    BOOST_CHECK_EQUAL(result,
                      StompClientResult::CouldNotConnectToWebSocketServer);
    stomp_client.Close();
  };
  stomp_client.Connect(stomp_username, stomp_password, on_connect_callback);
  io_context.run();

  BOOST_CHECK(on_connected_called);
}

BOOST_AUTO_TEST_CASE(DoesNotNeedOnConnectedCallbackToMakeConnection,
                     *boost::unit_test::disabled()) {
  // TODO
}

BOOST_AUTO_TEST_CASE(CallsOnDisconnectedAtStompAuthenticationFailure,
                     *timeout(1)) {
  const auto invalid_password = std::string{"invalid_password"};

  bool on_connected_called{false};
  bool on_disconnected_called{false};

  StompClient<WebSocketClientMockForStomp> stomp_client{
      url, endpoint, port, io_context, tls_context};

  auto on_connected_callback = [&on_connected_called,
                                &stomp_client](auto /*result*/) {
    on_connected_called = true;
  };
  auto on_disconnected_callback = [&on_disconnected_called,
                                   &stomp_client](auto result) {
    on_disconnected_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::WebSocketServerDisconnected);
  };

  stomp_client.Connect(stomp_username, invalid_password, on_connected_callback,
                       on_disconnected_callback);
  io_context.run();

  BOOST_CHECK(!on_connected_called);
  BOOST_CHECK(on_disconnected_called);
}

BOOST_AUTO_TEST_CASE(CallsOnCloseWhenClosed, *timeout(1)) {
  StompClient<WebSocketClientMockForStomp> stomp_client{
      url, endpoint, port, io_context, tls_context};

  bool closed{false};

  auto on_close_callback{[&closed](auto result) {
    closed = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
  }};
  auto on_connect_callback{[&stomp_client, &on_close_callback](auto result) {
    BOOST_REQUIRE_EQUAL(result, StompClientResult::Ok);
    stomp_client.Close(on_close_callback);
  }};

  stomp_client.Connect(stomp_username, stomp_password, on_connect_callback);
  io_context.run();
  BOOST_CHECK(closed);
}

BOOST_AUTO_TEST_CASE(DoesNotNeedOnCloseCallbackToCloseConnection,
                     *boost::unit_test::disabled()) {
  // TODO
}

BOOST_AUTO_TEST_CASE(CallsOnCloseWithErrorWhenCloseInvokedWhenNotConnected,
                     *timeout(1)) {
  StompClient<WebSocketClientMockForStomp> stomp_client{
      url, endpoint, port, io_context, tls_context};

  bool closed{false};

  auto on_close_callback{[&closed](auto result) {
    closed = true;
    BOOST_CHECK_EQUAL(result,
                      StompClientResult::CouldNotCloseWebSocketConnection);
  }};

  stomp_client.Close(on_close_callback);
  io_context.run();
  BOOST_CHECK(closed);
}

BOOST_AUTO_TEST_CASE(ReturnsSubscriptionIdAtSuccess, *timeout(1)) {
  StompClient<WebSocketClientMockForStomp> stomp_client{
      url, endpoint, port, io_context, tls_context};

  bool on_subscribe_called{false};

  auto on_subscribe_callback{[&stomp_client, &on_subscribe_called](
                                 auto result, auto&& subscription_id) {
    on_subscribe_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
    BOOST_CHECK(!subscription_id.empty());
    stomp_client.Close();
  }};

  auto on_message_callback{[](auto, auto&&) {}};
  auto on_connect_callback{[this, &stomp_client, &on_subscribe_callback,
                            &on_message_callback](auto result) {
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
    auto subscription_id{stomp_client.Subscribe(
        stomp_endpoint, on_subscribe_callback, on_message_callback)};
    BOOST_REQUIRE(!subscription_id.empty());
  }};

  stomp_client.Connect(stomp_username, stomp_password, on_connect_callback);
  io_context.run();
  BOOST_CHECK(on_subscribe_called);
}

/* StompClient::Connect()
 * - on_connected invoked with success
 * - on_connected invoked with failure
 * - on_disconnect invoked
 *
 * StompClient::Close()
 * - on_close invoked with success
 * - on_close invoked with failure
 *
 * StompClient::Subscribe()
 * - on_subscribe called success
 * - on_subscribe called failure
 * - on_message invoked
 * - subcribe to one destination and receive message
 * - subscribe to two destination and receive messages
 *
 * - on_disconnect callback called at STOMP bad authentication
 * - on_disconnect callback called at server connection lost
 */

BOOST_AUTO_TEST_SUITE_END();  // stomp_client

BOOST_AUTO_TEST_SUITE_END();  // network_monitor

}  // namespace

}  // namespace network_monitor
