#include "network_monitor/stomp_frame_builder.hpp"

#include <iomanip>
#include <vector>

#include <boost/test/data/test_case.hpp>
#include <boost/test/tools/old/interface.hpp>
#include <boost/test/unit_test.hpp>

#include "network_monitor/stomp_frame.hpp"
#include "utils/formatters.hpp"

using namespace network_monitor;
using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_frame);

namespace {
using network_monitor::StompCommand;

struct TestData {
  std::string context_name_;
  std::optional<StompCommand> command_;
  StompFrame::Headers headers_;
  std::string body_;
  std::string expected_message_;
};

std::ostream& operator<<(std::ostream& os, const TestData& data) {
  os << "{" << std::quoted("context_name_") << ": "
     << std::quoted(data.context_name_) << ", " << std::quoted("command_")
     << ": "
     << std::quoted(data.command_.has_value() ? ToString(data.command_.value())
                                              : "none")
     << ", " << std::quoted("headers_") << ": " << data.headers_ << ", "
     << std::quoted("body_") << ": " << std::quoted(data.body_) << ", "
     << std::quoted("expected_message_") << ": "
     << std::quoted(data.expected_message_) << "}";
  return os;
}

void VerifyFrame(const TestData& test_data) {
  StompFrameBuilder builder;

  if (test_data.command_.has_value()) {
    builder.SetCommand(test_data.command_.value());
  }

  for (const auto& [header, value] : test_data.headers_) {
    builder.AddHeader(header, value);
  }

  if (!test_data.body_.empty()) {
    builder.SetBody(test_data.body_);
  }

  BOOST_TEST(builder.BuildString() == test_data.expected_message_,
             boost::test_tools::per_element());
}

}  // namespace

BOOST_DATA_TEST_CASE(
    StompFrameBuilder,
    boost::unit_test::data::make(std::vector<TestData>{
        TestData{.context_name_ = "Only command",
                 .command_ = StompCommand::Disconnect,
                 .expected_message_ = "DISCONNECT\n"
                                      "\n\0"s},
        TestData{.context_name_ = "Command with single header",
                 .command_ = StompCommand::Receipt,
                 .headers_ =
                     {
                         {StompHeader::ReceiptId, "25"},
                     },
                 .expected_message_ = "RECEIPT\n"
                                      "receipt-id:25\n"
                                      "\n\0"s},
        TestData{.context_name_ = "Command with multiple headers",
                 .command_ = StompCommand::Message,
                 .headers_ =
                     {
                         {StompHeader::Destination, "/queue_a/"},
                         {StompHeader::MessageId, "10"},
                         {StompHeader::Subscription, "20"},
                     },
                 .expected_message_ = "MESSAGE\n"
                                      "destination:/queue_a/\n"
                                      "subscription:20\n"
                                      "message-id:10\n"
                                      "\n\0"s},
        TestData{.context_name_ = "Command with single header and oneline body",
                 .command_ = StompCommand::Ack,
                 .headers_ =
                     {
                         {StompHeader::Id, "30"},
                     },
                 .body_ = "Frame body",
                 .expected_message_ = "ACK\n"
                                      "id:30\n"
                                      "\n"
                                      "Frame body\0"s},
        TestData{.context_name_ = "Inserts quotes for headers with empty value",
                 .command_ = StompCommand::Connect,
                 .headers_ =
                     {
                         {StompHeader::AcceptVersion, "1.2"},
                         {StompHeader::Host, "host.com"},
                         {StompHeader::Login, ""},
                         {StompHeader::Passcode, ""},
                     },
                 .expected_message_ = "CONNECT\n"
                                      "accept-version:1.2\n"
                                      "host:host.com\n"
                                      "login:\"\"\n"
                                      "passcode:\"\"\n"
                                      "\n\0"s},
        //
    }),
    test_data) {
  VerifyFrame(test_data);
}

BOOST_AUTO_TEST_SUITE_END();  // stomp_frame

BOOST_AUTO_TEST_SUITE_END();  // network_monitor
