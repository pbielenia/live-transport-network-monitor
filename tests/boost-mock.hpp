#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/utility/string_view.hpp>
#include <network-monitor/websocket-client.hpp>

namespace network_monitor {

/*! \brief Mock the DNS resolver from Boost.Asio.
 *
 *  We do not mock all available methods - only the ones we are interested in for testing.
 */
class MockResolver {
public:
    /*! \brief Mock for the resolver contructor
     */
    template<typename ExecutionContext>
    explicit MockResolver(ExecutionContext&& context) : context{context}
    {
    }

    /*! \brief Mock for resolver::async_resolve
     */
    template<typename ResolveHandler>
    void async_resolve(boost::string_view host,
                  boost::string_view service,
                  ResolveHandler&& handler)
    {
        using resolver = boost::asio::ip::tcp::resolver;
        return boost::asio::async_initiate<ResolveHandler,
                                           void(const boost::system::error_code&,
                                                resolver::results_type)>(
            [](auto&& handler, auto resolver, auto host, auto service) {
                if (MockResolver::resolve_error_code) {
                    // Failing branch.
                    boost::asio::post(
                        resolver->context,
                        boost::beast::bind_handler(std::move(handler),
                                                   MockResolver::resolve_error_code,
                                                   resolver::results_type{}));
                } else {
                    // Successful branch.
                    boost::asio::post(
                        resolver->context,
                        boost::beast::bind_handler(
                            std::move(handler), MockResolver::resolve_error_code,
                            resolver::results_type::create(
                                boost::asio::ip::tcp::endpoint{
                                    boost::asio::ip::make_address("127.0.0.1"), 443},
                                host, service)));
                }
            },
            handler, this, host.to_string(), service.to_string());
    }

    /*! \brief Use this static member in a test to set the error code returned
     *         by async_resolve.
     */
    static boost::system::error_code resolve_error_code;

private:
    boost::asio::strand<boost::asio::io_context::executor_type> context;
};

inline boost::system::error_code MockResolver::resolve_error_code{};

/*! \brief Mock the TCP socket stream for Boost.Beast.
 *
 *  We do not mock all available methods - only the ones we are interested in for testing.
 */
class MockTcpStream : public boost::beast::tcp_stream {
public:
    /*! \brief Inherit all constructors from the parent class.
     */
    using boost::beast::tcp_stream::tcp_stream;

    /*! \brief Use this static member in a test to set the error code returned
     *         by async_connect.
     */
    static boost::system::error_code connect_error_code;

    /*! \brief Mock for tcp_stream::async_connect
     */
    template<typename ConnectHandler>
    void async_connect(endpoint_type type, ConnectHandler&& handler)
    {
        return boost::asio::async_initiate<ConnectHandler,
                                           void(boost::system::error_code)>(
            [](auto&& handler, auto stream) {
                // Call the user callback.
                boost::asio::post(
                    stream->get_executor(),
                    boost::beast::bind_handler(std::move(handler),
                                               MockTcpStream::connect_error_code));
            },
            handler, this);
    }
};

inline boost::system::error_code MockTcpStream::connect_error_code{};

template<typename TeardownHandler>
void async_teardown(boost::beast::role_type role,
                    MockTcpStream& socket,
                    TeardownHandler&& handler)
{
    return;
}

/*! \brief Type alias for the mocked WebSocketClient.
 *
 *  For now we only mock the DNS resolver.
 */
using MockWebSocketClient = WebSocketClient<
    MockResolver,
    boost::beast::websocket::stream<boost::beast::ssl_stream<MockTcpStream>>>;

} // namespace network_monitor
