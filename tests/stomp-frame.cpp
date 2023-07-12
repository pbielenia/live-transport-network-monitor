#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-frame.hpp>
#include <sstream>
#include <vector>

using NetworkMonitor::StompCommand;
using NetworkMonitor::StompError;
using NetworkMonitor::StompFrame;
using NetworkMonitor::StompHeader;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_frame);

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

static const std::vector<StompError> stomp_errors{
    // clang-format off
    StompError::Ok,
    StompError::UndefinedError,
    StompError::InvalidCommand,
    StompError::InvalidHeader,
    StompError::NoHeaderValue,
    StompError::MissingBodyNewline
    // clang-format on
};

BOOST_AUTO_TEST_SUITE(enum_StompCommand);

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

BOOST_AUTO_TEST_CASE(ostream)
{
    // Get the value of invalid
    std::stringstream stream{};
    stream << StompHeader::Invalid;
    const auto invalid{stream.str()};

    // Check if any valid header doesn't match the invalid
    for (const auto& header : stomp_headers) {
        stream.str("");
        stream.clear();
        stream << header;
        BOOST_CHECK(stream.str() != invalid);
    }
}

BOOST_AUTO_TEST_CASE(ToString)
{
    const auto invalid{NetworkMonitor::ToString(StompHeader::Invalid)};

    for (const auto& header : stomp_headers) {
        BOOST_CHECK(NetworkMonitor::ToString(header) != invalid);
    }
}

BOOST_AUTO_TEST_SUITE_END();  // enum_StompHeader

BOOST_AUTO_TEST_SUITE(enum_StompError);

BOOST_AUTO_TEST_CASE(ostream)
{
    // Get the value of invalid
    std::stringstream stream{};
    stream << StompError::UndefinedError;
    const auto invalid{stream.str()};

    // Check if any valid error doesn't match the invalid
    for (const auto& error : stomp_errors) {
        stream.str("");
        stream.clear();
        stream << error;
        BOOST_CHECK(stream.str() != invalid);
    }
}

BOOST_AUTO_TEST_CASE(ToString)
{
    const auto invalid{NetworkMonitor::ToString(StompError::UndefinedError)};

    for (const auto& error : stomp_errors) {
        BOOST_CHECK(NetworkMonitor::ToString(error) != invalid);
    }
}

BOOST_AUTO_TEST_SUITE_END();  // enum_StompError

BOOST_AUTO_TEST_SUITE(class_StompFrame);

class ExpectedFrame {
   public:
    ExpectedFrame() = default;

    void SetError(StompError error);
    void SetCommand(StompCommand command);
    void AddHeader(StompHeader header, std::string&& value);
    void SetHeadersCheck();
    void SetBody(std::string&& body);

    void Check(StompError parse_error, const StompFrame& parsed_frame) const;

   private:
    using Headers = std::map<StompHeader, std::string>;

    // TODO: Store `parsed_frame` as a private member to simplify invocations of below
    //       methods.
    void CheckHeaders(const StompFrame& parsed_frame) const;
    void CheckHeader(StompHeader header, const StompFrame& parsed_frame) const;

    StompError expected_error{StompError::UndefinedError};
    StompCommand expected_command{StompCommand::Invalid};
    Headers expected_headers{};
    std::string expected_body{""};

    bool check_error{false};
    bool check_command{false};
    bool check_headers{false};
    bool check_body{false};
};

void ExpectedFrame::SetError(StompError error)
{
    check_error = true;
    expected_error = error;
}

void ExpectedFrame::SetCommand(StompCommand command)
{
    check_command = true;
    expected_command = command;
}

void ExpectedFrame::AddHeader(StompHeader header, std::string&& value)
{
    check_headers = true;
    expected_headers.emplace(header, std::move(value));
}

// The purpose of this method is to trigger headers check when no headers are expected.
// `check_headers` is not `true` by default because there are cases when we don't want to
// check headers, for instance when the parsing frame returned with an error.
void ExpectedFrame::SetHeadersCheck()
{
    check_headers = true;
}

void ExpectedFrame::SetBody(std::string&& body)
{
    check_body = true;
    expected_body = std::move(body);
}

void ExpectedFrame::Check(StompError parse_error, const StompFrame& parsed_frame) const
{
    if (check_error) {
        if (expected_error != StompError::Ok) {
            BOOST_CHECK(parse_error != StompError::Ok);
        }
        BOOST_CHECK_EQUAL(parse_error, expected_error);
    }
    if (check_command) {
        BOOST_CHECK_EQUAL(parsed_frame.GetCommand(), expected_command);
    }
    if (check_headers) {
        CheckHeaders(parsed_frame);
    }
    if (check_body) {
        BOOST_CHECK_EQUAL(parsed_frame.GetBody(), expected_body);
    }
}

void ExpectedFrame::CheckHeaders(const StompFrame& parsed_frame) const
{
    for (const auto& header : stomp_headers) {
        CheckHeader(header, parsed_frame);
    }
}

void ExpectedFrame::CheckHeader(StompHeader header, const StompFrame& parsed_frame) const
{
    if (expected_headers.count(header)) {
        BOOST_CHECK_EQUAL(parsed_frame.HasHeader(header), true);
        BOOST_CHECK_EQUAL(parsed_frame.GetHeaderValue(header),
                          expected_headers.at(header));
    } else {
        BOOST_CHECK_EQUAL(parsed_frame.HasHeader(header), false);
        BOOST_CHECK_EQUAL(parsed_frame.GetHeaderValue(header), "");
    }
}

BOOST_AUTO_TEST_CASE(parse_well_formed)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::Ok);
    expected.SetCommand(StompCommand::Connect);
    expected.AddHeader(StompHeader::AcceptVersion, "42");
    expected.AddHeader(StompHeader::Host, "host.com");
    expected.SetBody("Frame body");

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_well_formed_content_length)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:10\n"
        "\n"
        "Frame body\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::Ok);
    expected.SetCommand(StompCommand::Connect);
    expected.AddHeader(StompHeader::AcceptVersion, "42");
    expected.AddHeader(StompHeader::Host, "host.com");
    expected.AddHeader(StompHeader::ContentLength, "10");
    expected.SetBody("Frame body");

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_empty_body)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::Ok);
    expected.SetCommand(StompCommand::Connect);
    expected.AddHeader(StompHeader::AcceptVersion, "42");
    expected.AddHeader(StompHeader::Host, "host.com");
    expected.SetBody("");

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_empty_body_content_length)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "content-length:0\n"
        "\n"
        "\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::Ok);
    expected.SetCommand(StompCommand::Connect);
    expected.AddHeader(StompHeader::AcceptVersion, "42");
    expected.AddHeader(StompHeader::Host, "host.com");
    expected.AddHeader(StompHeader::ContentLength, "0");
    expected.SetBody("");

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_empty_headers)
{
    std::string plain{
        "DISCONNECT\n"
        "\n"
        "Frame body\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::Ok);
    expected.SetCommand(StompCommand::Disconnect);
    expected.SetHeadersCheck();
    expected.SetBody("Frame body");

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_only_command)
{
    std::string plain{
        "DISCONNECT\n"
        "\n"
        "\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::Ok);
    expected.SetCommand(StompCommand::Disconnect);
    expected.SetHeadersCheck();
    expected.SetBody("");

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_invalid_command)
{
    std::string plain{
        "CONNECT_INVALID\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"};


    ExpectedFrame expected;
    expected.SetError(StompError::InvalidCommand);

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_invalid_header)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "header_invalid:value\n"
        "\n"
        "Frame body\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::InvalidHeader);

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_header_no_value)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "login\n"
        "\n"
        "Frame body\0"};

    ExpectedFrame expected;
    expected.SetError(StompError::NoHeaderValue);

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

BOOST_AUTO_TEST_CASE(parse_missing_body_newline)
{
    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"};

    ExpectedFrame expected;
    expected.SetError(StompError::MissingBodyNewline);

    StompError error;
    StompFrame frame{error, std::move(plain)};

    expected.Check(error, frame);
}

// TODO: test each ctor
// TODO: test copy and move operators
// TODO: test what GetHeaderValue returns when HasHeader returns false
// TODO: test enums' ToString and operator<<

BOOST_AUTO_TEST_SUITE_END();  // class_StompFrame

BOOST_AUTO_TEST_SUITE_END();  // stomp_frame

BOOST_AUTO_TEST_SUITE_END();  // network_monitor