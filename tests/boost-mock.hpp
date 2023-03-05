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
        [](auto&& handler, auto resolver) {
            // Failing branch.
            if (MockResolver::resolve_error_code) {
                boost::asio::post(
                    resolver->context_,
                    boost::beast::bind_handler(std::move(handler),
                                               MockResolver::resolve_error_code,
                                               resolver::results_type{}));
            } else {
                // TODO: We do not support the successful branch for now.
            }
        },
        token, this);
}

// Out-of-line static member initialization
inline boost::system::error_code MockResolver::resolve_error_code{};

/*! \brief Type alias for the mocked WebSocketClient.
 *
 *  For now we only mock the DNS resolver.
 */
using TestWebSocketClient = WebSocketClient<
    MockResolver,
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>;

} // namespace NetworkMonitor
