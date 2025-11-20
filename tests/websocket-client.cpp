#include "network-monitor/websocket-client.hpp"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/test/unit_test.hpp>

#include "boost-mock.hpp"

using network_monitor::MockResolver;
using network_monitor::MockSslStream;
using network_monitor::MockTcpStream;
using network_monitor::MockWebSocketStream;
using network_monitor::TestWebSocketClient;
using network_monitor::WebSocketClient;

// This fixture is used to re-initialize all mock properties before a test.
struct WebSocketClientTestFixture {
  WebSocketClientTestFixture() {
    MockResolver::resolve_error_code = {};
    MockTcpStream::connect_error_code = {};
    MockSslStream<MockTcpStream>::handshake_error_code = {};
    MockWebSocketStream<MockSslStream<MockTcpStream>>::handshake_error_code =
        {};
    MockWebSocketStream<MockSslStream<MockTcpStream>>::close_error_code = {};
    MockWebSocketStream<MockSslStream<MockTcpStream>>::write_error_code = {};
    MockWebSocketStream<MockSslStream<MockTcpStream>>::read_error_code = {};
    MockWebSocketStream<MockSslStream<MockTcpStream>>::read_buffer = {};
  }
};

// Used to set a timeout on tests that may hang or suffer from a slow
// connection.
using timeout = boost::unit_test::timeout;

constexpr auto kExpectedTimeout = std::chrono::milliseconds(250);

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_WebSocketClient);

BOOST_AUTO_TEST_CASE(cacert_pem) {
  BOOST_CHECK(std::filesystem::exists(TESTS_CACERT_PEM));
}

BOOST_FIXTURE_TEST_SUITE(Connect, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(fail_resolve, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
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

BOOST_AUTO_TEST_CASE(fail_socket_connection, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
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

BOOST_AUTO_TEST_CASE(fail_socket_handshake, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  MockSslStream<MockTcpStream>::handshake_error_code =
      boost::asio::ssl::error::stream_truncated;

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool called_on_connect{false};
  auto on_connect{[&called_on_connect](auto error_code) {
    called_on_connect = true;
    BOOST_CHECK_EQUAL(error_code,
                      MockSslStream<MockTcpStream>::handshake_error_code);
  }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(fail_websocket_handshake, *timeout{1}) {
  using MockTlsStream = MockSslStream<MockTcpStream>;

  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  MockWebSocketStream<MockTlsStream>::handshake_error_code =
      boost::beast::websocket::error::no_host;

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool called_on_connect{false};
  auto on_connect{[&called_on_connect](auto error_code) {
    called_on_connect = true;
    BOOST_CHECK_EQUAL(error_code,
                      MockWebSocketStream<MockTlsStream>::handshake_error_code);
  }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(successful_nothing_to_read, *timeout(1)) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool called_on_connect{false};
  auto on_connect{[&called_on_connect, &client](auto error_code) {
    called_on_connect = true;
    BOOST_CHECK(!error_code);
    client.Close();
  }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_CASE(successful_no_connecthandler, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool timeout_occured{false};

  boost::asio::high_resolution_timer timer(io_context, kExpectedTimeout);
  timer.async_wait([&timeout_occured, &client](auto error_code) {
    timeout_occured = true;
    BOOST_CHECK(!error_code);
    client.Close();
  });

  client.Connect();
  io_context.run();

  BOOST_CHECK(timeout_occured);
}

BOOST_AUTO_TEST_SUITE_END();  // Connect

BOOST_FIXTURE_TEST_SUITE(onMessage, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(one_message, *timeout{1}) {
  using WebsocketSocketStream =
      MockWebSocketStream<MockSslStream<MockTcpStream>>;

  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};
  const std::string expected_message{"Test message"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  WebsocketSocketStream::read_buffer = expected_message;

  bool called_on_message{false};

  auto on_message{[&called_on_message, &expected_message, &client](
                      auto error_code, auto received_message) {
    called_on_message = true;
    BOOST_CHECK(!error_code);
    BOOST_CHECK_EQUAL(expected_message, received_message);

    client.Close();
  }};

  client.Connect(nullptr, on_message);
  io_context.run();

  BOOST_CHECK(called_on_message);
}

BOOST_AUTO_TEST_CASE(two_messages, *timeout{1}) {
  using WebsocketSocketStream =
      MockWebSocketStream<MockSslStream<MockTcpStream>>;

  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};
  const std::string expected_message_1{"The first test message"};
  const std::string expected_message_2{"The second test message"};

  auto tls_context =
      boost::asio::ssl::context{boost::asio::ssl::context::tlsv12_client};
  auto io_context = boost::asio::io_context{};

  tls_context.load_verify_file(TESTS_CACERT_PEM);

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  unsigned called_on_message_count{};

  auto on_connect{[](auto error_code) { BOOST_CHECK(!error_code); }};
  auto on_message{[&called_on_message_count, &expected_message_1,
                   &expected_message_2,
                   &client](auto error_code, auto received_message) {
    called_on_message_count++;
    BOOST_CHECK(!error_code);

    if (called_on_message_count == 1) {
      BOOST_CHECK_EQUAL(expected_message_1, received_message);
      WebsocketSocketStream::read_buffer = expected_message_2;
    } else if (called_on_message_count == 2) {
      BOOST_CHECK_EQUAL(expected_message_2, received_message);
      client.Close();
    }
  }};

  WebsocketSocketStream::read_buffer = expected_message_1;
  client.Connect(on_connect, on_message);
  io_context.run();

  BOOST_CHECK_EQUAL(called_on_message_count, 2);
}

BOOST_AUTO_TEST_CASE(fail, *timeout{1}) {
  using WebsocketSocketStream =
      MockWebSocketStream<MockSslStream<MockTcpStream>>;

  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};
  const std::string expected_message{"Test message"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  WebsocketSocketStream::read_error_code =
      boost::beast::websocket::error::bad_data_frame;
  WebsocketSocketStream::read_buffer = expected_message;

  bool called_on_connect{false};
  bool called_on_message{false};
  bool timeout_occured{false};

  boost::asio::high_resolution_timer timer(io_context, kExpectedTimeout);
  timer.async_wait([&timeout_occured, &client](auto error_code) {
    timeout_occured = true;
    BOOST_CHECK(!error_code);
    client.Close();
  });

  auto on_connect{[&called_on_connect](auto error_code) {
    called_on_connect = true;
    BOOST_CHECK(!error_code);
  }};
  auto on_message{
      [&called_on_message](auto /*error_code*/, auto /*received_message*/) {
        called_on_message = true;
        BOOST_CHECK(false);
      }};

  client.Connect(on_connect, on_message);
  io_context.run();

  BOOST_CHECK(called_on_connect);
  BOOST_CHECK(!called_on_message);
  BOOST_CHECK(timeout_occured);
}

BOOST_AUTO_TEST_CASE(no_handler, *timeout{1}) {
  using WebsocketSocketStream =
      MockWebSocketStream<MockSslStream<MockTcpStream>>;

  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool called_on_connect{false};
  bool timeout_occured{false};

  boost::asio::high_resolution_timer timer(io_context, kExpectedTimeout);
  timer.async_wait([&timeout_occured, &client](auto error_code) {
    timeout_occured = true;
    BOOST_CHECK(!error_code);
    client.Close();
  });

  auto on_connect{[&called_on_connect](auto error_code) {
    called_on_connect = true;
    BOOST_CHECK(!error_code);
  }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(called_on_connect);
  BOOST_CHECK(timeout_occured);
}

BOOST_AUTO_TEST_SUITE_END();  // onMessage

BOOST_FIXTURE_TEST_SUITE(Send, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(one_message, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};
  const std::string message_to_send{"Test message"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool called_on_connect{false};
  bool called_on_send{false};

  auto on_sent_callback{[&called_on_send, &client](auto error_code) {
    called_on_send = true;
    BOOST_CHECK(!error_code.failed());
    client.Close();
  }};
  auto on_connect{
      [&called_on_connect, &client, &message_to_send,
       on_sent_callback = std::move(on_sent_callback)](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK(!error_code.failed());

        client.Send(message_to_send, std::move(on_sent_callback));
      }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(called_on_connect);
  BOOST_CHECK(called_on_send);
}

BOOST_AUTO_TEST_CASE(send_before_connect, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};
  const std::string message_to_send{"Test message"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool called_on_send{false};

  auto on_sent_callback{[&called_on_send, &client](auto error_code) {
    called_on_send = true;
    BOOST_CHECK_EQUAL(error_code, boost::asio::error::operation_aborted);
  }};

  client.Send(message_to_send, std::move(on_sent_callback));
  io_context.run();

  BOOST_CHECK(called_on_send);
}

BOOST_AUTO_TEST_CASE(fail, *timeout{1}) {
  using WebsocketSocketStream =
      MockWebSocketStream<MockSslStream<MockTcpStream>>;

  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};
  const std::string message_to_send{"Test message"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  WebsocketSocketStream::write_error_code =
      boost::beast::websocket::error::bad_data_frame;

  bool called_on_connect{false};
  bool called_on_send{false};

  auto on_sent_callback{[&called_on_send, &client](auto error_code) {
    called_on_send = true;
    BOOST_CHECK(error_code == boost::beast::websocket::error::bad_data_frame);
    client.Close();
  }};
  auto on_connect{
      [&called_on_connect, &client, &message_to_send,
       on_sent_callback = std::move(on_sent_callback)](auto error_code) {
        called_on_connect = true;
        BOOST_CHECK(!error_code.failed());
        client.Send(message_to_send, std::move(on_sent_callback));
      }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(called_on_send);
  BOOST_CHECK(called_on_connect);
}

BOOST_AUTO_TEST_SUITE_END();  // Send

BOOST_FIXTURE_TEST_SUITE(Close, WebSocketClientTestFixture);

BOOST_AUTO_TEST_CASE(close, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool on_close_called{false};
  auto on_close{[&on_close_called](auto error_code) {
    BOOST_CHECK(!error_code);
    on_close_called = true;
  }};
  auto on_connect{[&client, &on_close](auto error_code) {
    BOOST_CHECK(!error_code);
    client.Close(on_close);
  }};

  client.Connect(on_connect);
  io_context.run();

  BOOST_CHECK(on_close_called);
}

BOOST_AUTO_TEST_CASE(close_before_connect, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool on_close_called{false};

  auto on_close{[&on_close_called](auto error_code) {
    BOOST_CHECK_EQUAL(error_code, boost::asio::error::operation_aborted);
    on_close_called = true;
  }};

  client.Close(on_close);
  io_context.run();

  BOOST_CHECK(on_close_called);
}

BOOST_AUTO_TEST_CASE(close_no_disconnect, *timeout{1}) {
  const std::string url{"some.echo-server.com"};
  const std::string endpoint{"/"};
  const std::string port{"443"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);
  boost::asio::io_context io_context{};

  TestWebSocketClient client{url, endpoint, port, io_context, tls_context};

  bool on_close_called{false};
  bool on_disconnect_called{false};

  auto on_close{[&on_close_called](auto error_code) {
    on_close_called = true;
    BOOST_CHECK(!error_code.failed());
  }};
  auto on_connect{[&on_close, &client](auto error_code) {
    BOOST_REQUIRE(!error_code.failed());
    client.Close(on_close);
  }};
  auto on_disconnect{[&on_disconnect_called](auto /*error_code*/) {
    on_disconnect_called = true;
  }};

  client.Connect(on_connect, nullptr, on_disconnect);
  io_context.run();

  BOOST_CHECK(on_close_called);
  BOOST_CHECK(!on_disconnect_called);
}

BOOST_AUTO_TEST_SUITE_END();  // Close

BOOST_AUTO_TEST_SUITE(live);

BOOST_AUTO_TEST_CASE(echo, *timeout{20}) {
  const std::string url{"ltnm.learncppthroughprojects.com"};
  const std::string endpoint{"/echo"};
  const std::string port{"443"};
  const std::string message{"Hello WebSocket"};

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);

  boost::asio::io_context io_context{};
  network_monitor::BoostWebSocketClient client{url, endpoint, port, io_context,
                                               tls_context};

  bool connected{false};
  bool message_sent{false};
  bool message_received{false};
  bool disconnected{false};
  std::string echo{};

  auto on_sent_callback{[&message_sent](auto error_code) {
    message_sent = !error_code.failed();
  }};
  auto on_connect{[&client, &connected,
                   on_sent_callback = std::move(on_sent_callback),
                   &message](auto error_code) {
    connected = !error_code.failed();
    if (connected) {
      client.Send(message, std::move(on_sent_callback));
    }
  }};
  auto on_close{[&disconnected](auto error_code) {
    disconnected = !error_code.failed();
  }};
  auto on_receive{[&client, &on_close, &message_received, &message, &echo](
                      auto error_code, auto /*received*/) {
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

bool CheckResponse(const std::string& response) {
  bool ok{true};
  ok &= response.find("ERROR") != std::string::npos;
  ok &= response.find("ValidationInvalidAuth") != std::string::npos;
  return ok;
}

BOOST_AUTO_TEST_CASE(send_stomp_frame) {
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

  boost::asio::ssl::context tls_context{
      boost::asio::ssl::context::tlsv12_client};
  tls_context.load_verify_file(TESTS_CACERT_PEM);

  boost::asio::io_context io_context{};
  network_monitor::BoostWebSocketClient client{url, endpoint, port, io_context,
                                               tls_context};

  bool connected{false};
  bool message_sent{false};
  bool message_received{false};
  bool disconnected{false};
  std::string response{};

  auto on_sent_callback{
      [&message_sent](auto error_code) { message_sent = !error_code; }};

  auto on_connect_callback{[&client, &connected,
                            on_sent_callback = std::move(on_sent_callback),
                            &message](auto error_code) {
    connected = !error_code;
    if (connected) {
      client.Send(message, std::move(on_sent_callback));
    }
  }};

  auto on_close_callback{
      [&disconnected](auto error_code) { disconnected = !error_code; }};

  auto on_receive_callback{[&client, &on_close_callback, &message_received,
                            &response](auto error_code, auto received) {
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

BOOST_AUTO_TEST_SUITE_END();  // live

BOOST_AUTO_TEST_SUITE_END();  // class_WebSocketClient

BOOST_AUTO_TEST_SUITE_END();  // network_monitor
