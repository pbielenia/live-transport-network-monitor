#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <network-monitor/transport-network.hpp>
#include <stdexcept>
#include <string>

using network_monitor::Id;
using network_monitor::Line;
using network_monitor::Route;
using network_monitor::Station;
using network_monitor::TransportNetwork;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_TransportNetwork);

BOOST_AUTO_TEST_SUITE(add_station);

BOOST_AUTO_TEST_CASE(basic)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station{"station_000", "Station Name"};
    is_ok = network.add_station(station);
    BOOST_CHECK(is_ok);
}

BOOST_AUTO_TEST_CASE(duplicate_id)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station{"station_000", "Station Name"};
    is_ok = network.add_station(station);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_station(station);
    BOOST_CHECK(!is_ok);
}

BOOST_AUTO_TEST_CASE(duplicate_name)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Same Name"};
    is_ok = network.add_station(station_0);
    BOOST_REQUIRE(is_ok);
    Station station_1{"station_001", "Same Name"};
    is_ok = network.add_station(station_1);
    BOOST_CHECK(is_ok);
}

BOOST_AUTO_TEST_SUITE_END(); // add_station

BOOST_AUTO_TEST_SUITE(add_line);

BOOST_AUTO_TEST_CASE(basic)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    BOOST_REQUIRE(is_ok);

    Route route_0{"route_000",   "Route Name 0", "line_000",
                  "station_000", "station_001",  {"station_000", "station_001"}};
    Line line{"line_000", "Line Name", {route_0}};
    is_ok = network.add_line(line);
    BOOST_CHECK(is_ok);
}

BOOST_AUTO_TEST_CASE(shared_stations)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};
    Station station_2{"station_002", "Station Name 2"};
    Station station_3{"station_003", "Station Name 3"};
    Route route_0{"route_000",   "Route Name 0",
                  "line_000",    "station_000",
                  "station_002", {"station_000", "station_001", "station_002"}};
    Route route_1{"route_001",   "Route Name 1",
                  "line_000",    "station_003",
                  "station_002", {"station_003", "station_001", "station_002"}};
    Line line{"line_000", "Line Name", {route_0, route_1}};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    is_ok &= network.add_station(station_2);
    is_ok &= network.add_station(station_3);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_CHECK(is_ok);
}

BOOST_AUTO_TEST_CASE(duplicate)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};
    Route route_0{"route_000",   "Route Name 0", "line_000",
                  "station_000", "station_001",  {"station_000", "station_001"}};
    Line line{"line_000", "Line Name", {route_0}};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_CHECK(!is_ok);
}

BOOST_AUTO_TEST_SUITE_END(); // add_line

BOOST_AUTO_TEST_SUITE(passenger_events);

BOOST_AUTO_TEST_CASE(basic)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};
    Station station_2{"station_002", "Station Name 2"};
    Route route_0{"route_000",   "Route Name 0",
                  "line_000",    "station_000",
                  "station_002", {"station_000", "station_001", "station_002"}};
    Line line{"line_000", "Line Name", {route_0}};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    is_ok &= network.add_station(station_2);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_REQUIRE(is_ok);

    BOOST_REQUIRE_EQUAL(network.get_passenger_count(station_0.id), 0);
    BOOST_REQUIRE_EQUAL(network.get_passenger_count(station_1.id), 0);
    BOOST_REQUIRE_EQUAL(network.get_passenger_count(station_2.id), 0);
    try {
        auto count{network.get_passenger_count("station_42")};
        BOOST_REQUIRE(false);
    } catch (const std::runtime_error& error) {
        BOOST_REQUIRE(true);
    }

    using PassengerEvent = TransportNetwork::PassengerEvent;
    is_ok = network.record_passenger_event(station_0.id, PassengerEvent::In);
    BOOST_REQUIRE(is_ok);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_0.id), 1);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_1.id), 0);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_2.id), 0);

    is_ok = network.record_passenger_event(station_0.id, PassengerEvent::In);
    BOOST_REQUIRE(is_ok);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_0.id), 2);

    is_ok = network.record_passenger_event(station_1.id, PassengerEvent::In);
    BOOST_REQUIRE(is_ok);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_0.id), 2);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_1.id), 1);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_2.id), 0);

    is_ok = network.record_passenger_event(station_0.id, PassengerEvent::Out);
    BOOST_REQUIRE(is_ok);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_0.id), 1);

    is_ok = network.record_passenger_event(station_2.id, PassengerEvent::Out);
    BOOST_REQUIRE(is_ok);
    BOOST_CHECK_EQUAL(network.get_passenger_count(station_2.id), -1);
}

BOOST_AUTO_TEST_SUITE_END(); // passenger_events

BOOST_AUTO_TEST_SUITE(get_routes_serving_station);

BOOST_AUTO_TEST_CASE(basic)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};
    Station station_2{"station_002", "Station Name 2"};
    Station station_3{"station_003", "Station Name 3"};
    Route route_0{"route_000",   "Route Name 0",
                  "line_000",    "station_000",
                  "station_002", {"station_000", "station_001", "station_002"}};
    Line line{"line_000", "Line Name", {route_0}};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    is_ok &= network.add_station(station_2);
    is_ok &= network.add_station(station_3);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_REQUIRE(is_ok);

    std::vector<Id> routes;
    routes = network.get_routes_serving_station(station_0.id);
    BOOST_REQUIRE_EQUAL(routes.size(), 1);
    BOOST_CHECK(routes[0] == route_0.id);

    routes = network.get_routes_serving_station(station_1.id);
    BOOST_REQUIRE_EQUAL(routes.size(), 1);
    BOOST_CHECK(routes[0] == route_0.id);

    routes = network.get_routes_serving_station(station_2.id);
    BOOST_REQUIRE_EQUAL(routes.size(), 1);
    BOOST_CHECK(routes[0] == route_0.id);

    routes = network.get_routes_serving_station(station_3.id);
    BOOST_REQUIRE_EQUAL(routes.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END(); // get_routes_serving_station

BOOST_AUTO_TEST_SUITE(travel_time);

BOOST_AUTO_TEST_CASE(basic)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};
    Station station_2{"station_002", "Station Name 2"};
    Route route_0{"route_000",   "Route Name 0",
                  "line_000",    "station_000",
                  "station_002", {"station_000", "station_001", "station_002"}};
    Line line{"line_000", "Line Name", {route_0}};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    is_ok &= network.add_station(station_2);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_REQUIRE(is_ok);

    unsigned travel_time{0};
    travel_time = network.get_travel_time(station_0.id, station_1.id);
    BOOST_CHECK_EQUAL(travel_time, 0);

    is_ok = network.set_travel_time(station_0.id, station_2.id, 1);
    BOOST_CHECK(!is_ok);

    is_ok = network.set_travel_time(station_0.id, station_1.id, 2);
    BOOST_CHECK(is_ok);
    BOOST_CHECK_EQUAL(network.get_travel_time(station_0.id, station_1.id), 2);

    is_ok = network.set_travel_time(station_1.id, station_0.id, 3);
    BOOST_CHECK(is_ok);
    BOOST_CHECK_EQUAL(network.get_travel_time(station_1.id, station_0.id), 3);
}

BOOST_AUTO_TEST_CASE(over_route)
{
    TransportNetwork network{};
    bool is_ok{false};

    Station station_0{"station_000", "Station Name 0"};
    Station station_1{"station_001", "Station Name 1"};
    Station station_2{"station_002", "Station Name 2"};
    Station station_3{"station_003", "Station Name 3"};
    Route route_0{
        "route_000",   "Route Name 0",
        "line_000",    "station_000",
        "station_003", {"station_000", "station_001", "station_002", "station_003"}};
    Route route_1{"route_001",   "Route Name 1",
                  "line_000",    "station_003",
                  "station_002", {"station_003", "station_001", "station_002"}};
    Route route_2{"route_002",   "Route Name 2",
                  "line_000",    "station_003",
                  "station_000", {"station_003", "station_001", "station_000"}};
    Line line{"line_000", "Line Name", {route_0, route_1, route_2}};

    is_ok = true;
    is_ok &= network.add_station(station_0);
    is_ok &= network.add_station(station_1);
    is_ok &= network.add_station(station_2);
    is_ok &= network.add_station(station_3);
    BOOST_REQUIRE(is_ok);
    is_ok = network.add_line(line);
    BOOST_REQUIRE(is_ok);

    is_ok = true;
    is_ok &= network.set_travel_time(station_0.id, station_1.id, 1);
    is_ok &= network.set_travel_time(station_1.id, station_2.id, 2);
    is_ok &= network.set_travel_time(station_2.id, station_3.id, 3);
    is_ok &= network.set_travel_time(station_3.id, station_1.id, 4);

    unsigned travel_time{0};

    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_0.id, station_1.id), 1);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_0.id, station_2.id), 1 + 2);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_0.id, station_3.id),
        1 + 2 + 3);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_1.id, station_3.id), 2 + 3);

    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_1.id, station_3.id, station_1.id), 4);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_1.id, station_3.id, station_2.id), 4 + 2);

    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_2.id, station_3.id, station_1.id), 4);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_2.id, station_3.id, station_0.id), 4 + 1);

    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_3.id, station_1.id), 0);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_1.id, station_0.id), 0);
    BOOST_CHECK_EQUAL(
        network.get_travel_time(line.id, route_0.id, station_1.id, station_1.id), 0);
}

BOOST_AUTO_TEST_SUITE_END(); // travel_time

BOOST_AUTO_TEST_SUITE_END(); // class_TransportNetwork

BOOST_AUTO_TEST_SUITE_END(); // network_monitor
