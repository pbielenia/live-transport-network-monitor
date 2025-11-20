#pragma once

#include <string>

#include <boost/asio.hpp>
#include <boost/beast.hpp>

namespace network_monitor {

/*! \brief Mock the DNS resolver from Boost.Asio.
 *
 *  We do not mock all available methods — only the ones we are interested in
 *  for testing.
 */
class MockResolver {
 public:
  /*! \brief Mock for the resolver constructor
   */
  template <typename ExecutionContext>
  explicit MockResolver(ExecutionContext&& context);

  template <typename ResolveToken>
  void async_resolve(std::string_view host,
                     std::string_view service,
                     ResolveToken&& token);

  /*! \brief Use this static member in a test to set the error code returned by
   *         async_resolve.
   */
  static boost::system::error_code resolve_error_code;

 private:
  boost::asio::strand<boost::asio::io_context::executor_type> context_;
};

/*! \brief Mock the TCP socket stream from Boost.Beast
 *
 *  Only the methods explicitly used in testing are mocked.
 */
class MockTcpStream : public boost::beast::tcp_stream {
 public:
  using boost::beast::tcp_stream::tcp_stream;

  static boost::system::error_code connect_error_code;

  template <typename ConnectToken>
  void async_connect(endpoint_type type, ConnectToken&& token);
};

template <typename TcpStream>
class MockSslStream : public boost::beast::ssl_stream<TcpStream> {
 public:
  using boost::beast::ssl_stream<TcpStream>::ssl_stream;

  static boost::system::error_code handshake_error_code;

  template <typename HandshakeToken>
  void async_handshake(boost::asio::ssl::stream_base::handshake_type type,
                       HandshakeToken token);
};

template <typename TlsStream>
class MockWebSocketStream : public boost::beast::websocket::stream<TlsStream> {
 public:
  using boost::beast::websocket::stream<TlsStream>::stream;

  static boost::system::error_code handshake_error_code;
  static boost::system::error_code write_error_code;
  static boost::system::error_code read_error_code;
  static boost::system::error_code close_error_code;

  static std::string read_buffer;

  template <typename HandshakeToken>
  void async_handshake(std::string_view host,
                       std::string_view target,
                       HandshakeToken token);

  template <typename ConstBufferSequence, typename WriteHandler>
  void async_write(const ConstBufferSequence& buffers, WriteHandler&& handler);

  template <typename DynamicBuffer, typename ReadHandler>
  void async_read(DynamicBuffer& buffer, ReadHandler&& handler);

  template <typename CloseHandler>
  void async_close(const boost::beast::websocket::close_reason& close_reason,
                   CloseHandler&& handler);

 private:
  template <typename DynamicBuffer, typename ReadHandler>
  void recursive_read_internal(ReadHandler&& handler, DynamicBuffer& buffer);

  bool connection_is_open_{false};
};

template <typename ExecutionContext>
MockResolver::MockResolver(ExecutionContext&& context) : context_{context} {}

template <typename ResolveToken>
void MockResolver::async_resolve(std::string_view host,
                                 std::string_view service,
                                 ResolveToken&& token) {
  using resolver = boost::asio::ip::tcp::resolver;

  constexpr auto kSuccessfulTargetPort = 443;

  return boost::asio::async_initiate<ResolveToken,
                                     void(const boost::system::error_code&,
                                          resolver::results_type)>(
      [](auto&& handler, auto resolver, auto host, auto service) {
        if (MockResolver::resolve_error_code) {
          // Failing branch.
          boost::asio::post(
              resolver->context_,
              boost::beast::bind_handler(std::move(handler),
                                         MockResolver::resolve_error_code,
                                         resolver::results_type{}));
        } else {
          // Successful branch.
          boost::asio::post(
              resolver->context_,
              boost::beast::bind_handler(
                  std::move(handler), MockResolver::resolve_error_code,
                  resolver::results_type::create(
                      boost::asio::ip::tcp::endpoint{
                          boost::asio::ip::make_address("127.0.0.1"),
                          kSuccessfulTargetPort},
                      host, service)));
        }
      },
      token, this, std::string(host), std::string(service));
}

template <typename ConnectToken>
void MockTcpStream::async_connect(endpoint_type type, ConnectToken&& token) {
  return boost::asio::async_initiate<ConnectToken,
                                     void(boost::system::error_code)>(
      [](auto&& handler, auto stream) {
        boost::asio::post(
            stream->get_executor(),
            boost::beast::bind_handler(std::move(handler),
                                       MockTcpStream::connect_error_code));
      },
      token, this);
}

template <typename TcpStream>
template <typename HandshakeToken>
void MockSslStream<TcpStream>::async_handshake(
    boost::asio::ssl::stream_base::handshake_type type, HandshakeToken token) {
  return boost::asio::async_initiate<HandshakeToken,
                                     void(boost::system::error_code)>(
      [](auto&& handler, auto ssl_stream) {
        boost::asio::post(ssl_stream->get_executor(),
                          boost::beast::bind_handler(
                              std::move(handler),
                              MockSslStream<TcpStream>::handshake_error_code));
      },
      token, this);
}

template <typename TlsStream>
template <typename HandshakeToken>
void MockWebSocketStream<TlsStream>::async_handshake(std::string_view host,
                                                     std::string_view target,
                                                     HandshakeToken token) {
  return boost::asio::async_initiate<HandshakeToken,
                                     void(boost::system::error_code)>(
      [](auto&& handler, auto websocket_stream) {
        if (!MockWebSocketStream<TlsStream>::handshake_error_code.failed()) {
          websocket_stream->connection_is_open_ = true;
        }
        boost::asio::post(
            websocket_stream->get_executor(),
            boost::beast::bind_handler(
                std::move(handler),
                MockWebSocketStream<TlsStream>::handshake_error_code));
      },
      token, this);
}

template <typename TlsStream>
template <typename ConstBufferSequence, typename WriteHandler>
void MockWebSocketStream<TlsStream>::async_write(
    const ConstBufferSequence& buffers, WriteHandler&& handler) {
  return boost::asio::async_initiate<
      WriteHandler, void(boost::system::error_code, std::size_t)>(
      [this](auto&& handler, auto websocket_stream, const auto& buffers) {
        boost::system::error_code error_code;
        std::size_t bytes_transferred{0};

        if (!connection_is_open_) {
          error_code = boost::asio::error::operation_aborted;
        } else {
          error_code = MockWebSocketStream<TlsStream>::write_error_code;
        }

        if (!error_code) {
          bytes_transferred = buffers.size();
        }

        boost::asio::post(
            websocket_stream->get_executor(),
            boost::beast::bind_handler(std::move(handler), error_code,
                                       bytes_transferred));
      },
      handler, this, buffers);
}

template <typename TlsStream>
template <typename DynamicBuffer, typename ReadHandler>
void MockWebSocketStream<TlsStream>::async_read(DynamicBuffer& buffer,
                                                ReadHandler&& handler) {
  return boost::asio::async_initiate<
      ReadHandler, void(boost::system::error_code, std::size_t)>(
      [this](auto&& handler, auto& buffer) {
        recursive_read_internal(handler, buffer);
      },
      handler, buffer);
}

template <typename TlsStream>
template <typename DynamicBuffer, typename ReadHandler>
void MockWebSocketStream<TlsStream>::recursive_read_internal(
    ReadHandler&& handler, DynamicBuffer& buffer) {
  boost::system::error_code error_code;
  std::size_t bytes_read{0};
  auto& source_buffer = MockWebSocketStream<TlsStream>::read_buffer;

  if (!connection_is_open_) {
    error_code = boost::asio::error::operation_aborted;
  } else if (MockWebSocketStream<TlsStream>::read_error_code) {
    error_code = MockWebSocketStream<TlsStream>::read_error_code;
  } else if (!source_buffer.empty()) {
    bytes_read = boost::asio::buffer_copy(buffer.prepare(source_buffer.size()),
                                          boost::asio::buffer(source_buffer));
    buffer.commit(bytes_read);
    source_buffer.clear();
  }

  if (bytes_read == 0 && !error_code) {
    boost::asio::post(this->get_executor(),
                      [this, handler = std::move(handler), &buffer]() {
                        recursive_read_internal(handler, buffer);
                      });
  } else {
    boost::asio::post(
        this->get_executor(),
        boost::beast::bind_handler(std::move(handler), error_code, bytes_read));
  }
}

template <typename TlsStream>
template <typename CloseHandler>
void MockWebSocketStream<TlsStream>::async_close(
    const boost::beast::websocket::close_reason& close_reason,
    CloseHandler&& handler) {
  return boost::asio::async_initiate<CloseHandler,
                                     void(boost::system::error_code)>(
      [this](auto&& handler, auto websocket_stream) {
        boost::system::error_code error_code{};
        if (connection_is_open_) {
          if (MockWebSocketStream<TlsStream>::close_error_code) {
            error_code = MockWebSocketStream<TlsStream>::close_error_code;
          } else {
            connection_is_open_ = false;
          }
        } else {
          error_code = boost::asio::error::operation_aborted;
        }

        boost::asio::post(
            websocket_stream->get_executor(),
            boost::beast::bind_handler(std::move(handler), error_code));
      },
      handler, this);
}

// Out-of-line static members initialization
inline boost::system::error_code MockResolver::resolve_error_code{};
inline boost::system::error_code MockTcpStream::connect_error_code{};

template <typename TcpStream>
inline boost::system::error_code
    MockSslStream<TcpStream>::handshake_error_code{};

template <typename TlsStream>
inline boost::system::error_code
    MockWebSocketStream<TlsStream>::handshake_error_code{};

template <typename TlsStream>
inline boost::system::error_code
    MockWebSocketStream<TlsStream>::write_error_code{};

template <typename TlsStream>
inline boost::system::error_code
    MockWebSocketStream<TlsStream>::read_error_code{};

template <typename TlsStream>
inline boost::system::error_code
    MockWebSocketStream<TlsStream>::close_error_code{};

template <typename TlsStream>
inline std::string MockWebSocketStream<TlsStream>::read_buffer{};

template <typename TeardownHandler>
void async_teardown(boost::beast::role_type role,
                    MockTcpStream& socket,
                    TeardownHandler&& handler) {}

template <typename TeardownHandler, typename TcpStream>
void async_teardown(boost::beast::role_type role,
                    MockSslStream<TcpStream>& socket,
                    TeardownHandler&& handler) {}

template <typename TeardownHandler, typename TlsStream>
void async_teardown(boost::beast::role_type role,
                    MockWebSocketStream<TlsStream>& socket,
                    TeardownHandler&& handler) {}

/*! \brief Type alias for the mocked WebSocketClient.
 *
 *  Only the DNS resolver and the TCP stream are mocked.
 */
using TestWebSocketClient =
    WebSocketClient<MockResolver,
                    MockWebSocketStream<MockSslStream<MockTcpStream>>>;

}  // namespace network_monitor
