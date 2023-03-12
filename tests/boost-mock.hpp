#pragma once

#include <boost/asio.hpp>
#include <string>

namespace NetworkMonitor {

/*! \brief Mock the DNS resolver from Boost.Asio.
 *
 *  We do not mock all available methods — only the ones we are interested in
 *  for testing.
 */
class MockResolver {
public:
    /*! \brief Mock for the resolver constructor
     */
    template<typename ExecutionContext> explicit MockResolver(ExecutionContext&& context);

    template<typename ResolveToken>
    void
    async_resolve(std::string_view host, std::string_view service, ResolveToken&& token);

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

    template <typename ConnectHandler>
    void async_connect(endpoint_type type, ConnectHandler&& handler);
};

template<typename ExecutionContext>
MockResolver::MockResolver(ExecutionContext&& context) : context_{context}
{
}

template<typename ResolveToken>
void MockResolver::async_resolve(std::string_view host,
                                 std::string_view service,
                                 ResolveToken&& token)
{
    using resolver = boost::asio::ip::tcp::resolver;
    return boost::asio::async_initiate<
        ResolveToken, void(const boost::system::error_code&, resolver::results_type)>(
        [](auto&& handler, auto resolver, auto host, auto service) {
            if (MockResolver::resolve_error_code) {
                // Failing branch.
                boost::asio::post(
                    resolver->context_,
                    boost::beast::bind_handler(
                        std::move(handler),
                        MockResolver::resolve_error_code,
                        resolver::results_type{}
                    )
                );
            } else {
                // Successful branch.
                boost::asio::post(
                    resolver->context_,
                    boost::beast::bind_handler(
                        std::move(handler),
                        MockResolver::resolve_error_code,
                        resolver::results_type::create(
                            boost::asio::ip::tcp::endpoint{
                                boost::asio::ip::make_address(
                                    "127.0.0.1"
                                ),
                                443
                            },
                            host,
                            service
                        )
                    )
                );
            }
        },
        token,
        this,
        std::string(host),
        std::string(service)
    );
}

template <typename ConnectHandler>
void MockTcpStream::async_connect(endpoint_type type, ConnectHandler&& handler) {
    return boost::asio::async_initiate<
        ConnectHandler, void(boost::system::error_code)>(
        [](auto&& handler, auto stream) {
            boost::asio::post(stream->get_executor(),
                                boost::beast::bind_handler(std::move(handler),
                                                            MockTcpStream::connect_error_code));
        }, handler, this
    );
}

// Out-of-line static members initialization
inline boost::system::error_code MockResolver::resolve_error_code{};
inline boost::system::error_code MockTcpStream::connect_error_code{};

template <typename TeardownHandler>
void async_teardown(boost::beast::role_type role,
                    MockTcpStream& socket,
                    TeardownHandler&& handler) {
    return;
}

/*! \brief Type alias for the mocked WebSocketClient.
 *
 *  Only the DNS resolver and the TCP stream are mocked.
 */
using TestWebSocketClient = WebSocketClient<
    MockResolver,
    boost::beast::websocket::stream<boost::beast::ssl_stream<MockTcpStream>>>;

} // namespace NetworkMonitor
