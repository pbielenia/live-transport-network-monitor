#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-client.hpp>

class WebSocketClientMock {
   public:
    static boost::system::error_code connect_error_code;
    static boost::system::error_code send_error_code;

    WebSocketClientMock(const std::string& url,
                        const std::string& endpoint,
                        const std::string& port,
                        boost::asio::io_context& io_context,
                        boost::asio::ssl::context& tls_context)
        : io_context_{io_context}
    {
    }

    void Connect(
        std::function<void(boost::system::error_code)> on_connected_callback,
        std::function<void(boost::system::error_code, std::string&&)> on_message_callback,
        std::function<void(boost::system::error_code)> on_disconnected_callback)
    {
        if (!connect_error_code.failed()) {
            connected_ = true;
            on_message_callback_ = std::move(on_message_callback);
            on_disconnected_callback_ = std::move(on_disconnected_callback);
        }

        boost::asio::post(io_context_, [this, on_connected_callback]() {
            if (on_connected_callback) {
                on_connected_callback(connect_error_code);
            }
        });
    }

    void Send(const std::string& message,
              std::function<void(boost::system::error_code)> on_sent_callback = nullptr)
    {
        if (connected_) {
            boost::asio::post(io_context_, [this, on_sent_callback, message]() {
                if (on_sent_callback) {
                    on_sent_callback(send_error_code);
                    // TODO: inject server responses here
                }
            });
        } else {
            boost::asio::post(io_context_, [on_sent_callback]() {
                if (on_sent_callback) {
                    on_sent_callback(boost::asio::error::operation_aborted);
                }
            });
        }
    }

   private:
    boost::asio::io_context& io_context_;
    bool connected_{false};
    std::function<void(boost::system::error_code, std::string&&)> on_message_callback_;
    std::function<void(boost::system::error_code)> on_disconnected_callback_;
};

inline boost::system::error_code WebSocketClientMock::connect_error_code{};
inline boost::system::error_code WebSocketClientMock::send_error_code{};

struct StompClientTestFixture {
    StompClientTestFixture()
    {
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

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    bool on_connected_called{false};

    auto on_connect_callback = [&on_connected_called](auto result) {
        on_connected_called = true;
        BOOST_CHECK(!result.failed());
    };

    NetworkMonitor::StompClient<WebSocketClientMock> stomp_client{
        url, endpoint, port, io_context, tls_context};
    stomp_client.Connect(on_connect_callback);
    io_context.run();

    BOOST_CHECK(on_connected_called);
}

BOOST_AUTO_TEST_CASE(CallsOnConnectOnFailure)
{
    const std::string url{"some.echo-server.com"};
    const std::string endpoint{"/"};
    const std::string port{"443"};

    boost::asio::ssl::context tls_context{boost::asio::ssl::context::tlsv12_client};
    tls_context.load_verify_file(TESTS_CACERT_PEM);
    boost::asio::io_context io_context{};

    WebSocketClientMock::connect_error_code = boost::asio::error::host_not_found;

    bool on_connected_called{false};

    auto on_connect_callback = [&on_connected_called](auto result) {
        on_connected_called = true;
        BOOST_CHECK_EQUAL(result, WebSocketClientMock::connect_error_code);
    };

    NetworkMonitor::StompClient<WebSocketClientMock> stomp_client{
        url, endpoint, port, io_context, tls_context};
    stomp_client.Connect(on_connect_callback);
    io_context.run();

    BOOST_CHECK(on_connected_called);
}

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

BOOST_AUTO_TEST_SUITE_END();  // stomp_client

BOOST_AUTO_TEST_SUITE_END();  // network_monitor