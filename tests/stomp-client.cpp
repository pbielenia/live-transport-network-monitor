#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-client.hpp>
#include <network-monitor/stomp-frame-builder.hpp>
#include <queue>

using NetworkMonitor::StompCommand;
using NetworkMonitor::StompError;
using NetworkMonitor::StompFrame;
using NetworkMonitor::StompHeader;

class WebSocketClientMock {
   public:
    static boost::system::error_code connect_error_code;
    static boost::system::error_code send_error_code;
    static boost::system::error_code close_error_code;
    static std::queue<std::string> message_queue;
    static std::function<void(const std::string&)> respond_to_send;

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
        if (!connected_) {
            boost::asio::post(io_context_, [on_sent_callback]() {
                if (on_sent_callback) {
                    on_sent_callback(boost::asio::error::operation_aborted);
                }
            });
            return;
        }

        boost::asio::post(io_context_, [this, on_sent_callback, message]() {
            if (on_sent_callback) {
                on_sent_callback(send_error_code);
                // TODO: inject server responses here
                respond_to_send(message);
            }
        });
    }

    void Close(std::function<void(boost::system::error_code)> on_close = nullptr)
    {
        // TODO
    }

   private:
    boost::asio::io_context& io_context_;
    bool connected_{false};
    std::function<void(boost::system::error_code, std::string&&)> on_message_callback_;
    std::function<void(boost::system::error_code)> on_disconnected_callback_;
};

class WebSocketClientMockForStomp : public WebSocketClientMock {
public:
    WebSocketClientMockForStomp(
        const std::string& url,
        const std::string& endpoint,
        const std::string& port,
        boost::asio::io_context& io_context,
        boost::asio::ssl::context& tls_context
    ) : WebSocketClientMock(url, endpoint, port, io_context, tls_context) {
        respond_to_send = [this](auto message) {
            OnMessage(message);
        };
    }
    
private:
    void OnMessage(const std::string& message) {
        StompError error;
        StompFrame frame{error, message};
        if (error != StompError::Ok) {
            // TODO: trigger disconnection
            return;
        }

        // TODO: log frame command
        switch (frame.GetCommand()) {
            case StompCommand::Stomp:
            case StompCommand::Connect: {
                // if (CheckConnection(frame)) {
                // TODO: put a log message
                const std::string stomp_version{"1.4"};
                message_queue.push(NetworkMonitor::stomp_frame::MakeConnectedFrame(stomp_version, {}, {}, {}).ToString());
                // TODO: else MakeErrorFrame and trigger disconnection
                break;
            }
            case StompCommand::Subscribe: {
                // TODO
                break;
            }
            default: {
                break;
            }
        }
    }
    
    // TODO: rename it
    // bool CheckConnection(const NetworkMonitor::StompFrame& frame) {
    //     using namespace NetworkMonitor;
    //     if (!frame.HasHeader(StompHeader::Login) || !frame.HasHeader(StompHeader::Passcode)) {
    //         return false;
    //     }
    //     return false;
    //     // TODO: how to get the `username` and the `password`?
    //     // bool check_authentication{
    //     //     frame.GetHeaderValue(StompHeader::Login) == username && frame.GetHeaderValue(StompHeader::Passcode) == password};
    //     // return check_authentication;
    // }
};


inline boost::system::error_code WebSocketClientMock::connect_error_code{};
inline boost::system::error_code WebSocketClientMock::send_error_code{};
inline boost::system::error_code WebSocketClientMock::close_error_code{};
inline std::queue<std::string> message_queue{};

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

// TODO: trigger client/server disconnection (which one? maybe both in separate test cases?)

BOOST_AUTO_TEST_SUITE_END();  // stomp_client

BOOST_AUTO_TEST_SUITE_END();  // network_monitor