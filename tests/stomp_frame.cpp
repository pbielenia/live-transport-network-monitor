#include "network_monitor/stomp_frame.hpp"

#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>

#include "utils/formatters.hpp"

namespace {

using network_monitor::StompCommand;
using network_monitor::StompError;
using network_monitor::StompFrame;
using network_monitor::StompHeader;

using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_frame);

constexpr std::array<StompCommand, 16> kStompCommands{
    // clang-format off
    StompCommand::Abort,
    StompCommand::Ack,
    StompCommand::Begin,
    StompCommand::Commit,
    StompCommand::Connect,
    StompCommand::Connected,
    StompCommand::Disconnect,
    StompCommand::Error,
    StompCommand::Invalid,
    StompCommand::Message,
    StompCommand::NAck,
    StompCommand::Receipt,
    StompCommand::Send,
    StompCommand::Stomp,
    StompCommand::Subscribe,
    StompCommand::Unsubscribe
    // clang-format on
};

constexpr std::array<StompHeader, 20> kStompHeaders{
    // clang-format off
    StompHeader::AcceptVersion,
    StompHeader::Ack,
    StompHeader::ContentLength,
    StompHeader::ContentType,
    StompHeader::Destination,
    StompHeader::HeartBeat,
    StompHeader::Host,
    StompHeader::Id,
    StompHeader::Invalid,
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

constexpr std::array<StompError, 17> kStompErrors{
    // clang-format off
    StompError::Ok,
    StompError::UndefinedError,
    StompError::InvalidCommand,
    StompError::InvalidHeader,
    StompError::InvalidHeaderValue,
    StompError::NoHeaderValue,
    StompError::EmptyHeaderValue,
    StompError::NoNewlineCharacters,
    StompError::MissingLastHeaderNewline,
    StompError::MissingBodyNewline,
    StompError::MissingClosingNullCharacter,
    StompError::JunkAfterBody,
    StompError::ContentLengthsDontMatch,
    StompError::MissingRequiredHeader,
    StompError::NoData,
    StompError::MissingCommand,
    StompError::NoHeaderName
    // clang-format on
};

BOOST_AUTO_TEST_SUITE(enums);

// Calls `operator<<()` on each value in `enums` and verifies if invalid string
// is returned only for `invalid_value`.
template <typename Enum, std::size_t Size>
void VerifyOstreamDoesNotReturnInvalidForValidValues(
    Enum invalid_value, const std::array<Enum, Size>& enums) {
  // Get the string of the invalid enum value
  std::stringstream stream{};
  stream << invalid_value;
  const auto invalid_string{stream.str()};

  for (auto value : enums) {
    stream.str("");
    stream.clear();
    stream << value;
    if (stream.str() == invalid_string && value != invalid_value) {
      BOOST_FAIL("invalid string returned for non-invalid value");
    }
  }
}

// Calls `network_monitor::ToString()` on each value in `enums` and verifies
// if invalid string is returned only for `invalid_value`.
template <typename Enum, std::size_t Size>
void VerifyToStringDoesNotReturnInvalidForValidValues(
    Enum invalid_value, const std::array<Enum, Size>& enums) {
  const auto invalid_string{::network_monitor::ToString(invalid_value)};

  for (auto value : enums) {
    if (::network_monitor::ToString(value) == invalid_string &&
        value != invalid_value) {
      BOOST_FAIL("invalid string returned for non-invalid value");
    }
  }
}

BOOST_AUTO_TEST_CASE(OstreamDoesNotReturnInvalidForValidCommands) {
  VerifyOstreamDoesNotReturnInvalidForValidValues(StompCommand::Invalid,
                                                  kStompCommands);
}

BOOST_AUTO_TEST_CASE(ToStringDoesNotReturnInvalidForValidCommands) {
  VerifyToStringDoesNotReturnInvalidForValidValues(StompCommand::Invalid,
                                                   kStompCommands);
}

BOOST_AUTO_TEST_CASE(OstreamDoesNotReturnInvalidForValidHeaders) {
  VerifyOstreamDoesNotReturnInvalidForValidValues(StompHeader::Invalid,
                                                  kStompHeaders);
}

BOOST_AUTO_TEST_CASE(ToStringDoesNotReturnInvalidForValidHeaders) {
  VerifyToStringDoesNotReturnInvalidForValidValues(StompHeader::Invalid,
                                                   kStompHeaders);
}

BOOST_AUTO_TEST_CASE(OstreamDoesNotReturnUndefinedForDefinedError) {
  VerifyOstreamDoesNotReturnInvalidForValidValues(StompError::UndefinedError,
                                                  kStompErrors);
}

BOOST_AUTO_TEST_CASE(ToStringDoesNotReturnUndefinedForDefinedErrors) {
  VerifyToStringDoesNotReturnInvalidForValidValues(StompError::UndefinedError,
                                                   kStompErrors);
}

BOOST_AUTO_TEST_SUITE_END();  // enums

BOOST_AUTO_TEST_SUITE(class_StompFrame);

struct ExpectedFrame {
  std::optional<StompError> error_;
  std::optional<StompCommand> command_;
  StompFrame::Headers headers_;
  std::string body_;
};

void VerifyHeader(StompHeader header,
                  const StompFrame& actual,
                  const ExpectedFrame& expected) {
  if (expected.headers_.contains(header)) {
    BOOST_CHECK_EQUAL(actual.HasHeader(header), true);
    BOOST_CHECK_EQUAL(actual.GetHeaderValue(header),
                      expected.headers_.at(header));
  } else {
    BOOST_CHECK_EQUAL(actual.HasHeader(header), false);
    BOOST_CHECK_EQUAL(actual.GetHeaderValue(header), "");
  }
}

void VerifyHeaders(const StompFrame& actual, const ExpectedFrame& expected) {
  BOOST_REQUIRE_EQUAL(expected.headers_.size(), actual.GetAllHeaders().size());

  if (expected.headers_.empty()) {
    return;
  }

  for (const auto& header : kStompHeaders) {
    VerifyHeader(header, actual, expected);
  }
}

void VerifyFrame(const StompFrame& actual, const ExpectedFrame& expected) {
  if (expected.error_.has_value()) {
    BOOST_REQUIRE_EQUAL(actual.GetStompError(), expected.error_.value());
  }

  // Do not check other fields if `actual_frame` is in error state, as
  // accessing them may be undefined.
  if (actual.GetStompError() != StompError::Ok) {
    return;
  }

  if (expected.command_.has_value()) {
    BOOST_CHECK_EQUAL(actual.GetCommand(), expected.command_.value());
  }

  VerifyHeaders(actual, expected);
  BOOST_CHECK_EQUAL(actual.GetBody(), expected.body_);
}

std::ostream& operator<<(std::ostream& os, const ExpectedFrame& frame) {
  os << R"({ "error": ")"
     << (frame.error_.has_value() ? ToString(frame.error_.value()) : "none")
     << R"(", "command": ")"
     << (frame.command_.has_value() ? ToString(frame.command_.value()) : "none")
     << R"(", "headers": )" << frame.headers_ << R"(, "body": ")" << frame.body_
     << R"(" })";
  return os;
}

BOOST_AUTO_TEST_SUITE(parse);

struct TestData {
  std::string context_name_;
  std::string stomp_message_;
  ExpectedFrame expected_;
};

std::ostream& operator<<(std::ostream& os, const TestData& data) {
  os << R"({ "context_name_": ")" << data.context_name_ << R"(", "expected_": )"
     << data.expected_ << R"(, "stomp_message_": ")" << data.stomp_message_
     << R"("})";

  return os;
}

void VerifyFrame(const TestData& test_data) {
  BOOST_TEST_CONTEXT("Test name: " << test_data.context_name_) {
    auto actual_frame = StompFrame{test_data.stomp_message_};
    VerifyFrame(actual_frame, test_data.expected_);
  }
}

BOOST_DATA_TEST_CASE(
    ValidFormat,
    boost::unit_test::data::make(
        std::vector<TestData>{
            TestData{.context_name_ = "Full",
                     .stomp_message_ = "CONNECT\n"
                                       "accept-version:42\n"
                                       "host:host.com\n"
                                       "\n"
                                       "Frame body\0"s,
                     .expected_ = {.error_ = StompError::Ok,
                                   .command_ = StompCommand::Connect,
                                   .headers_ =
                                       {
                                           {StompHeader::AcceptVersion, "42"},
                                           {StompHeader::Host, "host.com"},
                                       },
                                   .body_ = "Frame body"}},
            TestData{
                .context_name_ =
                    "content-length header matching the actual body length",
                .stomp_message_ = "CONNECT\n"
                                  "accept-version:42\n"
                                  "host:host.com\n"
                                  "content-length:10\n"
                                  "\n"
                                  "Frame body\0"s,
                .expected_ = {.error_ = StompError::Ok,
                              .command_ = StompCommand::Connect,
                              .headers_ =
                                  {
                                      {StompHeader::AcceptVersion, "42"},
                                      {StompHeader::Host, "host.com"},
                                      {StompHeader::ContentLength, "10"},
                                  },
                              .body_ = "Frame body"}},
            TestData{.context_name_ = "Empty body allowed",
                     .stomp_message_ = "CONNECT\n"
                                       "accept-version:42\n"
                                       "host:host.com\n"
                                       "\n"
                                       "\0"s,
                     .expected_ = {.error_ = StompError::Ok,
                                   .command_ = StompCommand::Connect,
                                   .headers_ =
                                       {
                                           {StompHeader::AcceptVersion, "42"},
                                           {StompHeader::Host, "host.com"},
                                       },
                                   .body_ = ""}},
            TestData{.context_name_ =
                         "Empty body with matching content-length header",
                     .stomp_message_ = "CONNECT\n"
                                       "accept-version:42\n"
                                       "host:host.com\n"
                                       "content-length:0\n"
                                       "\n"
                                       "\0"s,
                     .expected_ = {.error_ = StompError::Ok,
                                   .command_ = StompCommand::Connect,
                                   .headers_ =
                                       {
                                           {StompHeader::AcceptVersion, "42"},
                                           {StompHeader::Host, "host.com"},
                                           {StompHeader::ContentLength, "0"},
                                       },
                                   .body_ = ""}},
            TestData{.context_name_ = "Empty headers allowed",
                     .stomp_message_ = "DISCONNECT\n"
                                       "\n"
                                       "Frame body\0"s,
                     .expected_ = {.error_ = StompError::Ok,
                                   .command_ = StompCommand::Disconnect,
                                   .headers_ = {},
                                   .body_ = "Frame body"}},
            TestData{.context_name_ = "Only command allowed",
                     .stomp_message_ = "DISCONNECT\n"
                                       "\n"
                                       "\0"s,
                     .expected_ = {.error_ = StompError::Ok,
                                   .command_ = StompCommand::Disconnect,
                                   .headers_ = {},
                                   .body_ = ""}},
            TestData{
                .context_name_ = "Newline after command allowed",
                .stomp_message_ = "DISCONNECT\n"
                                  "\n"
                                  "version:42\n"
                                  "host:host.com\n"
                                  "\n"
                                  "Frame body\0"s,
                .expected_ =
                    {.error_ = StompError::Ok,
                     .command_ = StompCommand::Disconnect,
                     .body_ = "version:42\nhost:host.com\n\nFrame body\0"}},
            TestData{.context_name_ = "Repeated the same header allowed",
                     .stomp_message_ = "CONNECT\n"
                                       "accept-version:42\n"
                                       "accept-version:43\n"
                                       "host:host.com\n"
                                       "\n"
                                       "Frame body\0"s,
                     .expected_ = {.error_ = StompError::Ok,
                                   .command_ = StompCommand::Connect,
                                   .headers_ =
                                       {
                                           {StompHeader::AcceptVersion, "42"},
                                           {StompHeader::Host, "host.com"},
                                       },
                                   .body_ = "Frame body\0"}},
        }),
    test_data) {
  VerifyFrame(test_data);
}

BOOST_DATA_TEST_CASE(
    InvalidFormat,
    boost::unit_test::data::make(std::vector<TestData>{
        TestData{.context_name_ = "Empty content",
                 .stomp_message_ = ""s,
                 .expected_{.error_ = StompError::NoData}},
        TestData{.context_name_ = "Missing command",
                 .stomp_message_ = "\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "content-length:0\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingCommand}},
        TestData{.context_name_ = "Missing newline characters",
                 .stomp_message_ = "CONNECT"
                                   "accept-version:42"
                                   "\0"s,
                 .expected_{.error_ = StompError::NoNewlineCharacters}},
        TestData{.context_name_ = "Missing body newline",
                 .stomp_message_ = "CONNECT\n\0"s,
                 .expected_ = {.error_ = StompError::MissingBodyNewline}},
        TestData{.context_name_ = "Invalid command",
                 .stomp_message_ = "CONNECT_INVALID\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "\n"
                                   "Frame body\0"s,
                 .expected_ = {.error_ = StompError::InvalidCommand}},
        TestData{.context_name_ = "Invalid header",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "header_invalid:value\n"
                                   "\n"
                                   "Frame body\0"s,
                 .expected_ = {.error_ = StompError::InvalidHeader}},
        TestData{.context_name_ = "Missing header value",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "login\n"
                                   "\n"
                                   "Frame body\0"s,
                 .expected_ = {.error_ = StompError::NoHeaderValue}},
        TestData{.context_name_ = "Missing body newline, headers present",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingBodyNewline}},
        TestData{.context_name_ = "Missing body newline, no headers",
                 .stomp_message_ = "CONNECT\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingBodyNewline}},
        TestData{.context_name_ = "Missing body newline, body present",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "Frame body\0"s,
                 .expected_ = {.error_ = StompError::MissingBodyNewline}},
        TestData{.context_name_ = "Missing last header newline",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingBodyNewline}},
        TestData{.context_name_ = "Empty header value",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:\n"
                                   "host:host.com\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::EmptyHeaderValue}},
        // TODO: error or ok?
        // TestData{.context_name_ = "Parse - double colon in header line",
        //          .stomp_message_ = "CONNECT\n"
        //                            "accept-version:42:43\n"
        //                            "host:host.com\n"
        //                            "\n"
        //                            "Frame body\0"s,
        //          .expected_ = {}},
        TestData{
            .context_name_ = "Repeated headers, second one with missing value",
            .stomp_message_ = "CONNECT\n"
                              "accept-version:42\n"
                              "accept-version:\n"
                              "\n"
                              "Frame body\0"s,
            .expected_ = {.error_ = StompError::EmptyHeaderValue}},
        TestData{
            .context_name_ = "Unterminated body",
            .stomp_message_ = "CONNECT\n"
                              "accept-version:42\n"
                              "host:host.com\n"
                              "\n"
                              "Frame body"s,
            .expected_ = {.error_ = StompError::MissingClosingNullCharacter}},
        TestData{
            .context_name_ = "Unterminated body, content-length header present",
            .stomp_message_ = "CONNECT\n"
                              "accept-version:42\n"
                              "host:host.com\n"
                              "content-length:10\n"
                              "\n"
                              "Frame body"s,
            .expected_ = {.error_ = StompError::MissingClosingNullCharacter}},
        TestData{.context_name_ = "Junk after body",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "\n"
                                   "Frame body\0\n\njunk\n\0"s,
                 .expected_ = {.error_ = StompError::JunkAfterBody}},
        TestData{
            .context_name_ = "Junk after body, content-length header present",
            .stomp_message_ = "CONNECT\n"
                              "accept-version:42\n"
                              "host:host.com\n"
                              "content-length:10\n"
                              "\n"
                              "Frame body\0\n\njunk\n\0"s,
            .expected_ = {.error_ = StompError::ContentLengthsDontMatch}},
        TestData{.context_name_ = "Newlines after body",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "\n"
                                   "Frame body\0\n\n\n\0"s,
                 .expected_ = {.error_ = StompError::JunkAfterBody}},
        TestData{
            .context_name_ =
                "Newlines after body, content-length header present",
            .stomp_message_ = "CONNECT\n"
                              "accept-version:42\n"
                              "host:host.com\n"
                              "content-length:10\n"
                              "\n"
                              "Frame body\0\n\n\n"s,
            .expected_ = {.error_ = StompError::MissingClosingNullCharacter}},
        TestData{.context_name_ = "content-length header value lower than the "
                                  "actual body length",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "content-length:9\n"  // This is one byte off
                                   "\n"
                                   "Frame body\0"s,
                 .expected_ = {.error_ = StompError::ContentLengthsDontMatch}},
        TestData{.context_name_ = "content-length heaer value greater than the "
                                  "actual body length",
                 .stomp_message_ =
                     "CONNECT\n"
                     "accept-version:42\n"
                     "host:host.com\n"
                     "content-length:15\n"  // Way above the actual body length
                     "\n"
                     "Frame body\0"s,
                 .expected_ = {.error_ = StompError::ContentLengthsDontMatch}},
        TestData{
            .context_name_ = "value of content-length header is not a number",
            .stomp_message_ = "CONNECT\n"
                              "accept-version:42\n"
                              "host:host.com\n"
                              "content-length:five\n"
                              "\n"
                              "Frame body\0"s,
            .expected_ = {.error_ = StompError::InvalidHeaderValue}},
    }),
    test_data) {
  VerifyFrame(test_data);
}

BOOST_DATA_TEST_CASE(
    RequiredHeaders,
    boost::unit_test::data::make(std::vector<TestData>{
        // CONNECT
        TestData{.context_name_ = "CONNECT - empty headers",
                 .stomp_message_ = "CONNECT\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "CONNECT - missing host",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "CONNECT - missing accept-version",
                 .stomp_message_ = "CONNECT\n"
                                   "host:host.com\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "CONNECT - ok",
                 .stomp_message_ = "CONNECT\n"
                                   "accept-version:42\n"
                                   "host:host.com\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Connect,
                               .headers_ =
                                   {
                                       {StompHeader::AcceptVersion, "42"},
                                       {StompHeader::Host, "host.com"},
                                   }}},
        // CONNECTED
        TestData{.context_name_ = "CONNECTED - missing version",
                 .stomp_message_ = "CONNECTED\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "CONNECTED - ok",
                 .stomp_message_ = "CONNECTED\n"
                                   "version:42\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Connected,
                               .headers_ =
                                   {
                                       {StompHeader::Version, "42"},
                                   }}},
        //  SEND
        TestData{.context_name_ = "SEND - missing destination",
                 .stomp_message_ = "SEND\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "SEND - ok",
                 .stomp_message_ = "SEND\n"
                                   "destination:/queue/a\n"
                                   "\n"
                                   "Frame body\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Send,
                               .headers_ =
                                   {
                                       {StompHeader::Destination, "/queue/a"},
                                   },
                               .body_ = "Frame body"}},
        // SUBSCRIBE
        TestData{.context_name_ = "SUBSCRIBE - empty headers",
                 .stomp_message_ = "SUBSCRIBE\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "SUBSCRIBE - missing destination",
                 .stomp_message_ = "SUBSCRIBE\n"
                                   "id:0\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "SUBSCRIBE - missing id",
                 .stomp_message_ = "SUBSCRIBE\n"
                                   "destination:/queue/foo\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "SUBSCRIBE - ok",
                 .stomp_message_ = "SUBSCRIBE\n"
                                   "id:0\n"
                                   "destination:/queue/foo\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Subscribe,
                               .headers_ =
                                   {
                                       {StompHeader::Id, "0"},
                                       {StompHeader::Destination, "/queue/foo"},
                                   }}},
        // UNSUBSCRIBE
        TestData{.context_name_ = "UNSUBSCRIBE - missing id",
                 .stomp_message_ = "UNSUBSCRIBE\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "UNSUBSCRIBE - ok",
                 .stomp_message_ = "UNSUBSCRIBE\n"
                                   "id:0\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Unsubscribe,
                               .headers_ =
                                   {
                                       {StompHeader::Id, "0"},
                                   }}},
        // ACK
        TestData{.context_name_ = "ACK - missing id",
                 .stomp_message_ = "ACK\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "ACK - ok",
                 .stomp_message_ = "ACK\n"
                                   "id:12345\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Ack,
                               .headers_ =
                                   {
                                       {StompHeader::Id, "12345"},
                                   }}},
        // NACK
        TestData{.context_name_ = "NACK - missing id",
                 .stomp_message_ = "NACK\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "NACK - ok",
                 .stomp_message_ = "NACK\n"
                                   "id:12345\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::NAck,
                               .headers_ =
                                   {
                                       {StompHeader::Id, "12345"},
                                   }}},
        // BEGIN
        TestData{.context_name_ = "BEGIN - missing transaction",
                 .stomp_message_ = "BEGIN\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "BEGIN - ok",
                 .stomp_message_ = "BEGIN\n"
                                   "transaction:tx1\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Begin,
                               .headers_ =
                                   {
                                       {StompHeader::Transaction, "tx1"},
                                   }}},
        // COMMIT
        TestData{.context_name_ = "COMMIT - missing transaction",
                 .stomp_message_ = "COMMIT\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "COMMIT - ok",
                 .stomp_message_ = "COMMIT\n"
                                   "transaction:tx1\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Commit,
                               .headers_ =
                                   {
                                       {StompHeader::Transaction, "tx1"},
                                   }}},
        // ABORT
        TestData{.context_name_ = "ABORT - missing transaction",
                 .stomp_message_ = "ABORT\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "ABORT - ok",
                 .stomp_message_ = "ABORT\n"
                                   "transaction:tx1\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Abort,
                               .headers_ =
                                   {
                                       {StompHeader::Transaction, "tx1"},
                                   }}},
        // DISCONNECT
        TestData{.context_name_ = "DISCONNECT - ok",
                 .stomp_message_ = "DISCONNECT\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Disconnect}},
        // MESSAGE
        TestData{.context_name_ = "MESSAGE - empty headers",
                 .stomp_message_ = "MESSAGE\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{
            .context_name_ = "MESSAGE - missing message-id and destination",
            .stomp_message_ = "MESSAGE\n"
                              "subscription:0\n"
                              "\n"
                              "\0"s,
            .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{
            .context_name_ = "MESSAGE - missing subscription and destination",
            .stomp_message_ = "MESSAGE\n"
                              "message-id:007\n"
                              "\n"
                              "\0"s,
            .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{
            .context_name_ = "MESSAGE - missing subscription and message-id",
            .stomp_message_ = "MESSAGE\n"
                              "destination:/queue/a\n"
                              "\n"
                              "\0"s,
            .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "MESSAGE - missing destination",
                 .stomp_message_ = "MESSAGE\n"
                                   "subscription:0\n"
                                   "message-id:007\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "MESSAGE - missing message-id",
                 .stomp_message_ = "MESSAGE\n"
                                   "subscription:0\n"
                                   "destination:/queue/a\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "MESSAGE - missing subscription",
                 .stomp_message_ = "MESSAGE\n"
                                   "message-id:007\n"
                                   "destination:/queue/a\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "MESSAGE - ok",
                 .stomp_message_ = "MESSAGE\n"
                                   "subscription:0\n"
                                   "message-id:007\n"
                                   "destination:/queue/a\n"
                                   "\n"
                                   "hello queue a\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Message,
                               .headers_ =
                                   {
                                       {StompHeader::Subscription, "0"},
                                       {StompHeader::MessageId, "007"},
                                       {StompHeader::Destination, "/queue/a"},
                                   },
                               .body_ = "hello queue a"}},
        // RECEIPT
        TestData{.context_name_ = "RECEIPT - missing receipt-id",
                 .stomp_message_ = "RECEIPT\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::MissingRequiredHeader}},
        TestData{.context_name_ = "RECEIPT - ok",
                 .stomp_message_ = "RECEIPT\n"
                                   "receipt-id:77\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Receipt,
                               .headers_ =
                                   {
                                       {StompHeader::ReceiptId, "77"},
                                   }}},
        // ERROR
        // TODO: doesn't it need to carry any message?
        TestData{.context_name_ = "ERROR - ok",
                 .stomp_message_ = "ERROR\n"
                                   "\n"
                                   "\0"s,
                 .expected_ = {.error_ = StompError::Ok,
                               .command_ = StompCommand::Error}},
    }),
    test_data) {
  VerifyFrame(test_data);
}

BOOST_AUTO_TEST_SUITE_END();  // parse

BOOST_AUTO_TEST_SUITE(constructors_and_operators)

BOOST_AUTO_TEST_CASE(copy_constructor) {
  const auto expected_frame =
      ExpectedFrame{.error_ = StompError::Ok,
                    .command_ = StompCommand::Connect,
                    .headers_ =
                        {
                            {StompHeader::AcceptVersion, "42"},
                            {StompHeader::Host, "host.com"},
                            {StompHeader::ContentLength, "10"},
                        },
                    .body_ = "Frame body"};

  const auto parsed_frame = StompFrame{
      "CONNECT\n"
      "accept-version:42\n"
      "host:host.com\n"
      "content-length:10\n"
      "\n"
      "Frame body\0"s};

  VerifyFrame(parsed_frame, expected_frame);

  // NOLINTBEGIN(performance-unnecessary-copy-initialization)
  const auto copied_frame{parsed_frame};
  // NOLINTEND(performance-unnecessary-copy-initialization)
  // TODO: verify copied_frame.body_ points at copied_frame.plain_content_
  VerifyFrame(copied_frame, expected_frame);
}

BOOST_AUTO_TEST_CASE(move_constructor) {
  const auto expected_frame =
      ExpectedFrame{.error_ = StompError::Ok,
                    .command_ = StompCommand::Connect,
                    .headers_ =
                        {
                            {StompHeader::AcceptVersion, "42"},
                            {StompHeader::Host, "host.com"},
                            {StompHeader::ContentLength, "10"},
                        },
                    .body_ = "Frame body"};
  // TODO: moved-from std::string_view body_ should be reset
  // ExpectedFrame expected_from_move_frame{.error_ = StompError::Ok,
  //                                        .command_ = StompCommand::Connect,
  //                                        .headers_ = {},
  //                                        .body_ = ""};

  const auto parsed_frame = StompFrame{
      "CONNECT\n"
      "accept-version:42\n"
      "host:host.com\n"
      "content-length:10\n"
      "\n"
      "Frame body\0"s};

  VerifyFrame(parsed_frame, expected_frame);

  // NOLINTBEGIN(performance-unnecessary-copy-initialization)
  const auto from_move_frame{parsed_frame};
  // NOLINTEND(performance-unnecessary-copy-initialization)
  VerifyFrame(from_move_frame, expected_frame);
  // TODO: moved-from std::string_view body_ should be reset
  // VerifyFrame(parsed_frame, expected_from_move_frame);
}

BOOST_AUTO_TEST_CASE(copy_assignment_operator) {
  const auto expected_frame =
      ExpectedFrame{.error_ = StompError::Ok,
                    .command_ = StompCommand::Connect,
                    .headers_ =
                        {
                            {StompHeader::AcceptVersion, "42"},
                            {StompHeader::Host, "host.com"},
                            {StompHeader::ContentLength, "10"},
                        },
                    .body_ = "Frame body"};
  // TODO: moved-from std::string_view body_ should be reset
  // ExpectedFrame expected_from_move_frame{.error_ = StompError::Ok,
  //                                        .command_ = StompCommand::Connect,
  //                                        .headers_ = {},
  //                                        .body_ = ""};

  const auto parsed_frame = StompFrame{
      "CONNECT\n"
      "accept-version:42\n"
      "host:host.com\n"
      "content-length:10\n"
      "\n"
      "Frame body\0"s};

  VerifyFrame(parsed_frame, expected_frame);

  // NOLINTBEGIN(performance-unnecessary-copy-initialization)
  const auto copied_frame = parsed_frame;
  // NOLINTEND(performance-unnecessary-copy-initialization)
  VerifyFrame(copied_frame, expected_frame);
  // TODO: moved-from std::string_view body_ should be reset
  // VerifyFrame(parsed_frame, expected_from_move_frame);
}

BOOST_AUTO_TEST_CASE(move_assignment_operator) {
  const auto expected_frame =
      ExpectedFrame{.error_ = StompError::Ok,
                    .command_ = StompCommand::Connect,
                    .headers_ =
                        {
                            {StompHeader::AcceptVersion, "42"},
                            {StompHeader::Host, "host.com"},
                            {StompHeader::ContentLength, "10"},
                        },
                    .body_ = "Frame body"};

  const auto parsed_frame = StompFrame{
      "CONNECT\n"
      "accept-version:42\n"
      "host:host.com\n"
      "content-length:10\n"
      "\n"
      "Frame body\0"s};

  VerifyFrame(parsed_frame, expected_frame);

  // NOLINTBEGIN(performance-unnecessary-copy-initialization,
  //             performance-move-const-arg)
  const auto copied_frame = std::move(parsed_frame);
  // NOLINTEND(performance-unnecessary-copy-initialization,
  //           performance-move-const-arg)
  // TODO: verify copied_frame.body_ points at copied_frame.plain_content_
  VerifyFrame(copied_frame, expected_frame);
}

BOOST_AUTO_TEST_CASE(to_string_method) {
  std::string plain{
      "MESSAGE\n"
      "subscription:0\n"
      "message-id:007\n"
      "destination:/queue/a\n"
      "\n"
      "hello queue a\0"s};
  const auto& plain_size{plain.size()};

  const auto frame = StompFrame{std::move(plain)};

  BOOST_REQUIRE(frame.GetStompError() == StompError::Ok);

  const auto& frame_text{frame.ToString()};

  // Sizes match.
  BOOST_CHECK_EQUAL(plain_size, frame_text.size());
  // Starts with the command.
  BOOST_CHECK_EQUAL(frame_text.find("MESSAGE\n"), 0);
  // Ends with the body with a new line preceding.
  BOOST_CHECK_EQUAL(frame_text.find("\nhello queue a\0"), 59);
  // Has all the headers in any order.
  BOOST_CHECK(frame_text.find("subscription:0\n"));
  BOOST_CHECK(frame_text.find("message-id:007\n"));
  BOOST_CHECK(frame_text.find("destination:/queue/a\n"));
}

BOOST_AUTO_TEST_SUITE_END()  // constructors_and_operators

// TODO: test what GetHeaderValue returns when HasHeader returns false
// TODO: test enums' ToString and operator<<

BOOST_AUTO_TEST_SUITE_END();  // class_StompFrame

BOOST_AUTO_TEST_SUITE_END();  // stomp_frame

BOOST_AUTO_TEST_SUITE_END();  // network_monitor

}  // namespace
