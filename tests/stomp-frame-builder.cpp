#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-frame-builder.hpp>
#include <sstream>
#include <vector>

using NetworkMonitor::StompCommand;
using NetworkMonitor::StompError;
using NetworkMonitor::StompFrame;
using NetworkMonitor::StompHeader;
using namespace NetworkMonitor;

using namespace std::string_literals;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(stomp_frame_builder);

BOOST_AUTO_TEST_CASE(BuildsOnlyCommand)
{
  stomp_frame::BuildParameters parameters(StompCommand::Disconnect);

  StompError error_code;
  auto frame = stomp_frame::Build(error_code, parameters);

  BOOST_CHECK_EQUAL(error_code, StompError::Ok);
  BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::Disconnect);
}

BOOST_AUTO_TEST_CASE(BuildsCommandAndHeader)
{
  stomp_frame::BuildParameters parameters(StompCommand::Receipt);
  parameters.headers.emplace(StompHeader::ReceiptId, "25");

  StompError error_code;
  auto frame = stomp_frame::Build(error_code, parameters);

  BOOST_CHECK_EQUAL(error_code, StompError::Ok);
  BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::Receipt);
  BOOST_REQUIRE(frame.HasHeader(StompHeader::ReceiptId));
  BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::ReceiptId), "25");
}

BOOST_AUTO_TEST_CASE(BuildsCommandAndMultipleHeaders)
{
  stomp_frame::BuildParameters parameters(StompCommand::Message);
  parameters.headers.emplace(StompHeader::Destination, "/queue_a/");
  parameters.headers.emplace(StompHeader::MessageId, "10");
  parameters.headers.emplace(StompHeader::Subscription, "20");

  StompError error_code;
  auto frame = stomp_frame::Build(error_code, parameters);

  BOOST_CHECK_EQUAL(error_code, StompError::Ok);
  BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::Message);
  BOOST_REQUIRE(frame.HasHeader(StompHeader::Destination));
  BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::Destination), "/queue_a/");
  BOOST_REQUIRE(frame.HasHeader(StompHeader::MessageId));
  BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::MessageId), "10");
  BOOST_REQUIRE(frame.HasHeader(StompHeader::Subscription));
  BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::Subscription), "20");
}

BOOST_AUTO_TEST_CASE(BuildsFullFrame)
{
  stomp_frame::BuildParameters parameters(StompCommand::Ack);
  parameters.headers.emplace(StompHeader::Id, "30");
  parameters.body = "Frame body";

  StompError error_code;
  auto frame = stomp_frame::Build(error_code, parameters);

  BOOST_CHECK_EQUAL(error_code, StompError::Ok);
  BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::Ack);
  BOOST_REQUIRE(frame.HasHeader(StompHeader::Id));
  BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::Id), "30");
  BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
}

BOOST_AUTO_TEST_CASE(InsertsQuotesAtEmptySpecifiedHeaders)
{
  stomp_frame::BuildParameters parameters(StompCommand::Connect);
  parameters.headers.emplace(StompHeader::AcceptVersion, "1.2");
  parameters.headers.emplace(StompHeader::Host, "host.com");
  parameters.headers.emplace(StompHeader::Login, "");
  parameters.headers.emplace(StompHeader::Passcode, "");

  StompError error_code;
  auto frame = stomp_frame::Build(error_code, parameters);

  BOOST_CHECK_EQUAL(error_code, StompError::Ok);
  BOOST_CHECK(frame.ToString().find("login:\"\"\n"));
  BOOST_CHECK(frame.ToString().find("passcode:\"\"\n"));
}

BOOST_AUTO_TEST_SUITE_END();  // stomp_frame_builder

BOOST_AUTO_TEST_SUITE_END();  // network_monitor
