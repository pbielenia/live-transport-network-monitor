#include <boost/test/unit_test.hpp>
#include <network-monitor/stomp-frame.hpp>

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_StompFrame);

BOOST_AUTO_TEST_CASE(parse_well_formed)
{
    using namespace NetworkMonitor;

    std::string plain{
        "CONNECT\n"
        "accept-version:42\n"
        "host:host.com\n"
        "\n"
        "Frame body\0"s};

    StompError error;
    StompFrame frame{error, std::move(plain)};

    BOOST_CHECK_EQUAL(error, StompError::kOk);
    BOOST_CHECK_EQUAL(frame.GetCommand(), StompCommand::kConnect);
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kAcceptVersion), "42");
    BOOST_CHECK_EQUAL(frame.GetHeaderValue(StompHeader::kHost), "host.com");
    BOOST_CHECK_EQUAL(frame.GetBody(), "Frame body");
}

// TODO: test each ctor
// TODO: test copy and move operators

BOOST_AUTO_TEST_SUITE_END();  // class_StompFrame

BOOST_AUTO_TEST_SUITE_END();  // network_monitor