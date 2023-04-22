#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-frame.hpp>
#include <sstream>
#include <vector>

using namespace NetworkMonitor;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_frame);

BOOST_AUTO_TEST_SUITE(enum_StompCommand);

static const std::vector<StompCommand> stomp_commands{
    // clang-format off
    StompCommand::Abort,
    StompCommand::Ack,
    StompCommand::Begin,
    StompCommand::Commit,
    StompCommand::Connect,
    StompCommand::Connected,
    StompCommand::Disconnect,
    StompCommand::Error,
    StompCommand::Message,
    StompCommand::NAck,
    StompCommand::Receipt,
    StompCommand::Send,
    StompCommand::Stomp,
    StompCommand::Subscribe,
    StompCommand::Unsubscribe
    // clang-format on
};

BOOST_AUTO_TEST_CASE(ostream)
{
    // Get the value of invalid
    std::stringstream stream{};
    stream << StompCommand::Invalid;
    const auto invalid{stream.str()};

    // Check if any valid command doesn't match the invalid
    for (const auto& command : stomp_commands) {
        stream.str("");
        stream.clear();
        stream << command;
        BOOST_CHECK(stream.str() != invalid);
    }
}

BOOST_AUTO_TEST_CASE(ToString)
{
    const auto invalid{NetworkMonitor::ToString(StompCommand::Invalid)};

    for (const auto& command : stomp_commands) {
        BOOST_CHECK(NetworkMonitor::ToString(command) != invalid);
    }
}

BOOST_AUTO_TEST_SUITE_END();  // enum_StompCommand

BOOST_AUTO_TEST_SUITE(enum_StompHeader);

static const std::vector<StompHeader> stomp_headers{
    // clang-format off
    StompHeader::AcceptVersion,
    StompHeader::Ack,
    StompHeader::ContentLength,
    StompHeader::ContentType,
    StompHeader::Destination,
    StompHeader::HeartBeat,
    StompHeader::Host,
    StompHeader::Id,
    StompHeader::Login,
    StompHeader::Message,
    StompHeader::MessageId,
    StompHeader::Passcode,
    StompHeader::Receipt,
    StompHeader::ReceiptId,
    StompHeader::Session,
    StompHeader::Server,
    StompHeader::Subscription,
    StompHeader::Transaction,
    StompHeader::Version
    // clang-format on
};

BOOST_AUTO_TEST_SUITE_END();  // enum_StompHeader

BOOST_AUTO_TEST_SUITE(enum_StompError);

static const std::vector<StompError> stomp_errors{
    // clang-format off
    StompError::Ok
    // clang-format on
};

BOOST_AUTO_TEST_SUITE_END();  // enum_StompError

BOOST_AUTO_TEST_SUITE(class_StompFrame);

BOOST_AUTO_TEST_CASE(parse_well_formed)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"};

    StompError error;
    StompFrame frame{error, std::move(plain)};

    BOOST_CHECK_EQUAL(error, StompError::Ok);
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::Connect);
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::AcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::Host), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
}

// TODO: test each ctor
// TODO: test copy and move operators
// TODO: test enums' ToString and operator<<

BOOST_AUTO_TEST_SUITE_END();  // class_StompFrame

BOOST_AUTO_TEST_SUITE_END();  // stomp_frame

BOOST_AUTO_TEST_SUITE_END();  // network_monitor