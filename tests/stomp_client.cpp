#include "network_monitor/stomp_client.hpp"

#include <string>

#include <boost/asio.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/test/unit_test_suite.hpp>

#include "websocket_client_mock.hpp"

namespace network_monitor {

namespace {

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_client);

using timeout = boost::unit_test::timeout;
using StompClientWithMock = StompClient<WebSocketClientMockForStomp>;

constexpr unsigned kDefaultTestTimeoutInSeconds = 1;

struct StompClientParams {
  std::string url{"some.echo-server.com"};
  std::string endpoint{"/"};
  std::string port{"443"};
};

struct StompClientTestFixture {
  StompClientTestFixture();

  StompClientWithMock CreateStompClientWithMock(
      const StompClientParams& params = {}) {
    return StompClientWithMock{params.url, params.endpoint, params.port,
                               io_context_, tls_context_};
  }

  std::string stomp_username_{"correct_username"};
  std::string stomp_password_{"correct_password"};
  std::string stomp_endpoint_{"correct_endpoint"};

  boost::asio::ssl::context tls_context_{
      boost::asio::ssl::context::tlsv12_client};
  boost::asio::io_context io_context_;
};

StompClientTestFixture::StompClientTestFixture() {
  WebSocketClientMock::Config::send_error_code_ = {};
  WebSocketClientMock::Config::close_error_code_ = {};
  WebSocketClientMock::Config::fail_connecting_websocket_ = false;
  WebSocketClientMock::Config::fail_sending_message_ = false;
  WebSocketClientMock::Results::url = {};
  WebSocketClientMock::Results::endpoint = {};
  WebSocketClientMock::Results::port = {};
  WebSocketClientMock::Results::messages_sent_to_websocket_client = {};
  WebSocketClientMockForStomp::Responses::on_frame_connect = {};
  WebSocketClientMockForStomp::username = stomp_username_;
  WebSocketClientMockForStomp::password = stomp_password_;
  WebSocketClientMockForStomp::endpoint = stomp_endpoint_;

  tls_context_.load_verify_file(TESTS_CACERT_PEM);
}

BOOST_FIXTURE_TEST_SUITE(Constructor, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(SetsWebSocketConnectionData) {
  auto stomp_client = CreateStompClientWithMock({
      .url = "test url",
      .endpoint = "test endpoint",
      .port = "test port",
  });
  BOOST_CHECK(WebSocketClientMock::Results::url == "test url");
  BOOST_CHECK(WebSocketClientMock::Results::endpoint == "test endpoint");
  BOOST_CHECK(WebSocketClientMock::Results::port == "test port");
}

BOOST_AUTO_TEST_SUITE_END();  // Constructor

BOOST_FIXTURE_TEST_SUITE(Connect, StompClientTestFixture);

// If connected to server (WebSocket and STOMP) with success:
// - `on_connecting_done_callback` is called
//    - with ok result
// - expected STOMP frame is passed to WebSocketClient
//    - CONNECT
//    - correct passcode and login
BOOST_AUTO_TEST_CASE(OnConnectedToStompServer,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  bool callback_called{false};
  auto stomp_client = CreateStompClientWithMock();

  auto on_connecting_done_callback = [&callback_called,
                                      &stomp_client](auto result) {
    callback_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);

    BOOST_CHECK(WebSocketClientMock::Results::messages_sent_to_websocket_client
                    .size() == 1);
    BOOST_TEST(WebSocketClientMock::Results::messages_sent_to_websocket_client
                       .front() == std::string{"CONNECT\n"
                                               "passcode:correct_password\n"
                                               "login:correct_username\n"
                                               "host:some.echo-server.com\n"
                                               "accept-version:1.2\n\n"} +
                                       '\0',
               boost::test_tools::per_element());

    stomp_client.Close();
  };
  stomp_client.Connect(stomp_username_, stomp_password_,
                       on_connecting_done_callback);
  io_context_.run();

  BOOST_CHECK(callback_called);
}

// If could not connect to WebSocket server:
// - `on_connecting_done_callback` is called
//   - with error result
BOOST_AUTO_TEST_CASE(OnWebSocketConnectFailure,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  WebSocketClientMock::Config::fail_connecting_websocket_ = true;

  bool on_connected_called{false};
  auto stomp_client = CreateStompClientWithMock();

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

// If could not connect to STOMP frame:
// - `on_connecting_done_callback` is called
//   - with error result
BOOST_AUTO_TEST_CASE(OnWebSocketSendConnectFrameFailed,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  WebSocketClientMockForStomp::Config::fail_sending_message_ = true;

  auto stomp_client = CreateStompClientWithMock();
  bool callback_called{false};

  auto on_connecting_done_callback = [&callback_called,
                                      &stomp_client](auto result) {
    callback_called = true;
    BOOST_CHECK(result == StompClientResult::ErrorConnectingStomp);
    stomp_client.Close();
  };

  stomp_client.Connect(stomp_username_, stomp_password_,
                       on_connecting_done_callback);
  io_context_.run();

  BOOST_CHECK(callback_called);
}

// If could not parse first received STOMP frame (CONNECTED expected)
// - `on_connecting_done_callback` is called
//   - with error result
BOOST_AUTO_TEST_CASE(OnParsingFirstReceivedFrameFailed,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  auto stomp_client = CreateStompClientWithMock();
  bool callback_called{false};

  WebSocketClientMockForStomp::Responses::on_frame_connect = [] {
    return std::string{"invalid frame"};
  };

  auto on_connecting_done_callback = [&callback_called,
                                      &stomp_client](auto result) {
    callback_called = true;
    BOOST_CHECK(result == StompClientResult::ErrorConnectingStomp);
    stomp_client.Close();
  };

  stomp_client.Connect(stomp_username_, stomp_password_,
                       on_connecting_done_callback);
  io_context_.run();

  BOOST_CHECK(callback_called);
}

// If the first received STOMP frame is not CONNECTED
// - `on_connecting_done_callback` is called
//   - with error result

// Server rejects connecting
// send error frame
// disconnects
// call on_connecting_done, not on_disconnect

BOOST_AUTO_TEST_CASE(DoesNotNeedOnConnectingDoneCallbackToMakeConnection,
                     *boost::unit_test::disabled()) {
  // TODO: how does one verify it's actually connected?
}

BOOST_AUTO_TEST_CASE(CallsOnDisconnectedAtStompAuthenticationFailure,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  const auto invalid_password = std::string{"invalid_password"};

  bool on_disconnected_called{false};
  auto stomp_client = CreateStompClientWithMock();

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

BOOST_AUTO_TEST_CASE(SendsCorrectConnectFrame,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  auto stomp_client = CreateStompClientWithMock();

  bool callback_called{false};

  auto on_connecting_done_callback = [&callback_called](auto /*result*/) {
    callback_called = true;
    BOOST_CHECK(WebSocketClientMock::Results::messages_sent_to_websocket_client
                    .size() == 1);
    BOOST_TEST(WebSocketClientMock::Results::messages_sent_to_websocket_client
                       .front() == std::string{"CONNECT\n"
                                               "passcode:correct_password\n"
                                               "login:correct_username\n"
                                               "host:some.echo-server.com\n"
                                               "accept-version:1.2\n\n"} +
                                       '\0',
               boost::test_tools::per_element());
  };

  stomp_client.Connect(stomp_username_, stomp_password_,
                       on_connecting_done_callback);
  io_context_.run();

  BOOST_CHECK(callback_called);
}

BOOST_AUTO_TEST_SUITE_END();  // Connect

BOOST_FIXTURE_TEST_SUITE(Close, StompClientTestFixture);

BOOST_AUTO_TEST_CASE(CallsOnCloseWhenClosed,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  auto stomp_client = CreateStompClientWithMock();
  bool on_closed_called{false};

  auto on_closed_callback{[&on_closed_called](auto result) {
    on_closed_called = true;
    BOOST_CHECK_EQUAL(result, StompClientResult::Ok);
  }};
  auto on_connect_callback{[&stomp_client, &on_closed_callback](auto result) {
    BOOST_REQUIRE_EQUAL(result, StompClientResult::Ok);
    stomp_client.Close(on_closed_callback);
  }};

  stomp_client.Connect(stomp_username_, stomp_password_, on_connect_callback);
  io_context_.run();
  BOOST_CHECK(on_closed_called);
}

BOOST_AUTO_TEST_CASE(DoesNotNeedOnCloseCallbackToCloseConnection,
                     *boost::unit_test::disabled()) {
  // TODO: how to verify it's actually closed?
}

BOOST_AUTO_TEST_CASE(CallsOnCloseWithErrorWhenCloseInvokedWhenNotConnected,
                     *timeout(kDefaultTestTimeoutInSeconds)) {
  auto stomp_client = CreateStompClientWithMock();
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
  auto stomp_client = CreateStompClientWithMock();
  bool on_subscribed_called{false};

  auto on_subscribe_callback{[&stomp_client, &on_subscribed_called](
                                 auto result, auto&& subscription_id) {
    on_subscribed_called = true;
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
  BOOST_CHECK(on_subscribed_called);
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
