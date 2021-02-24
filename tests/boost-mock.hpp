#pragma once

#include <boost/asio.hpp>
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
    explicit MockResolver(ExecutionContext&& context) :
    {
    }

    /*! \brief Mock for resolver::async_resolve
     */
    template<typename ResolveHandler>
    async_resolve(boost::string_view host,
                  boost::string_view service,
                  ResolveHandler&& handler)
    {
        using resolver = boost::asio::ip::tcp::resolver;
        return boost::asio::async_initiate<ResolveHandler,
                                           void(cont boost::system::error_code&,
                                                resolver::results_type)>(
            [](auto&& handler, auto resolver) {
                if (MockResolver::resolve_error_code) {
                    // Failing branch.
                    boost::asio::post(
                        resolver->context,
                        boost::beast::bind_handler(std::move(handler),
                                                   MockResolver::resolve_error_code,
                                                   resolver::results_type{}));
                } else {
                    // TODO: We do not support the successful branch for now.
                }
            },
            handler, this);
    }

    /*! \brief Use this static member in a test to set the error code returned
     *         by async_resolve.
     */
    static boost::system::error_code resolve_error_code;

private:
    boost::asio::strand<boost::asio::io_context::executor_type> context;
};

inline boost::system::error_code MockResolver::resolve_error_code{};

/*! \brief Type alias for the mocked WebSocketClient.
 *
 *  For now we only mock the DNS resolver.
 */
using MockWebSocketClient = WebSocketClient<
    MockResolver,
    boost::beast::websocket::stream<boost::beast::ssl_stream<boost::beast::tcp_stream>>>;

} // namespace network_monitor
