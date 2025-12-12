#include "network_monitor/stomp_client.hpp"

#include <string>

#include <boost/asio.hpp>
#include <boost/test/unit_test.hpp>

#include "websocket_client_mock.hpp"

namespace network_monitor {

namespace {

using timeout = boost::unit_test::timeout;

constexpr unsigned kDefaultTestTimeoutInSeconds = 1;

struct StompClientTestFixture {
  StompClientTestFixture();

  std::string url_{"some.echo-server.com"};
  std::string endpoint_{"/"};
  std::string port_{"443"};
  std::string stomp_username_{"correct_username"};
  std::string stomp_password_{"correct_password"};
  std::string stomp_endpoint_{"correct_endpoint"};

  boost::asio::ssl::context tls_context_{
      boost::asio::ssl::context::tlsv12_client};
  boost::asio::io_context io_context_;
};

StompClientTestFixture::StompClientTestFixture() {
  WebSocketClientMock::Config::connect_error_code_ = {};
  WebSocketClientMock::Config::send_error_code_ = {};
  WebSocketClientMock::Config::close_error_code_ = {};
  WebSocketClientMockForStomp::username = stomp_username_;
  WebSocketClientMockForStomp::password = stomp_password_;
  WebSocketClientMockForStomp::endpoint = stomp_endpoint_;

  tls_context_.load_verify_file(TESTS_CACERT_PEM);
}

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_client);

using StompClientWithMock = StompClient<WebSocketClientMockForStomp>;

BOOST_FIXTURE_TEST_SUITE(Connect, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(CallsOnConnectOnSuccess,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  bool on_connected_called{false};

  StompClientWithMock stomp_client{url_, endpoint_, port_, io_context_,
                                   tls_context_};

  auto on_connect_callback = [&on_connected_called,
                              &stomp_client](auto result) {
    on_connected_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
    stomp_client.Close();
  };
  stomp_client.Connect(stomp_username_, stomp_password_, on_connect_callback);
  io_context_.run();

  BOOST_CHECK(on_connected_called);
}

BOOST_AUTO_TEST_CASE(CallsOnConnectOnWebSocketConnectionFailure,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  WebSocketClientMock::Config::connect_error_code_ =
      boost::asio::ssl::error::stream_truncated;

  bool on_connected_called{false};

  StompClientWithMock stomp_client{url_, endpoint_, port_, io_context_,
                                   tls_context_};

  auto on_connect_callback = [&on_connected_called,
                              &stomp_client](auto result) {
    on_connected_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::ErrorConnectingWebSocket);
    stomp_client.Close();
  };
  stomp_client.Connect(stomp_username_, stomp_password_, on_connect_callback);
  io_context_.run();

  BOOST_CHECK(on_connected_called);
}

BOOST_AUTO_TEST_CASE(DoesNotNeedOnConnectedCallbackToMakeConnection,
                     *boost::unit_test::disabled()) {
  // TODO: how does one verify it's actually connected?
}

BOOST_AUTO_TEST_CASE(CallsOnDisconnectedAtStompAuthenticationFailure,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  const auto invalid_password = std::string{"invalid_password"};

  bool on_disconnected_called{false};

  StompClientWithMock stomp_client{url_, endpoint_, port_, io_context_,
                                   tls_context_};

  auto on_connected_callback = [](auto /*result*/) { BOOST_CHECK(false); };
  auto on_disconnected_callback = [&on_disconnected_called,
                                   &stomp_client](auto result) {
    on_disconnected_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::WebSocketServerDisconnected);
  };

  stomp_client.Connect(stomp_username_, invalid_password, on_connected_callback,
                       on_disconnected_callback);
  io_context_.run();

  BOOST_CHECK(on_disconnected_called);
}

BOOST_AUTO_TEST_SUITE_END();  // Connect

BOOST_FIXTURE_TEST_SUITE(Close, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(CallsOnCloseWhenClosed,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  StompClientWithMock stomp_client{url_, endpoint_, port_, io_context_,
                                   tls_context_};

  bool closed{false};

  auto on_close_callback{[&closed](auto result) {
    closed = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
  }};
  auto on_connect_callback{[&stomp_client, &on_close_callback](auto result) {
    BOOST_REQUIRE_EQUAL(result, StompClientResult::Ok);
    stomp_client.Close(on_close_callback);
  }};

  stomp_client.Connect(stomp_username_, stomp_password_, on_connect_callback);
  io_context_.run();
  BOOST_CHECK(closed);
}

BOOST_AUTO_TEST_CASE(DoesNotNeedOnCloseCallbackToCloseConnection,
                     *boost::unit_test::disabled()) {
  // TODO: how to verify it's actually closed?
}

BOOST_AUTO_TEST_CASE(CallsOnCloseWithErrorWhenCloseInvokedWhenNotConnected,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  StompClientWithMock stomp_client{url_, endpoint_, port_, io_context_,
                                   tls_context_};

  bool on_closed_called{false};

  auto on_closed_callback{[&on_closed_called](auto result) {
    on_closed_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::ErrorNotConnected);
  }};

  stomp_client.Close(on_closed_callback);
  io_context_.run();
  BOOST_CHECK(on_closed_called);
}

BOOST_AUTO_TEST_SUITE_END();  // Close

BOOST_FIXTURE_TEST_SUITE(Subscribe, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(ReturnsSubscriptionIdOnSuccess,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  StompClientWithMock stomp_client{url_, endpoint_, port_, io_context_,
                                   tls_context_};

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
    auto subscription_id = stomp_client.Subscribe(
        stomp_endpoint_, on_subscribe_callback, on_message_callback);
    BOOST_REQUIRE(!subscription_id.empty());
  }};

  stomp_client.Connect(stomp_username_, stomp_password_, on_connect_callback);
  io_context_.run();
  BOOST_CHECK(on_subscribe_called);
}

BOOST_AUTO_TEST_SUITE_END();  // Subscribe

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
 * - verify subscription ids are different
 *
 * - on_disconnect callback called at STOMP bad authentication
 * - on_disconnect callback called at server connection lost
 *
 * - connect, subscribe & verify, close, connect, subscribe & verify
 */

BOOST_AUTO_TEST_SUITE_END();  // stomp_client

BOOST_AUTO_TEST_SUITE_END();  // network_monitor

}  // namespace

}  // namespace network_monitor
