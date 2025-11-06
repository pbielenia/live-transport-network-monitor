#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <network-monitor/file-downloader.hpp>
#include <network-monitor/transport-network.hpp>
#include <stdexcept>
#include <string>

using network_monitor::Id;
using network_monitor::Line;
using network_monitor::PassengerEvent;
using network_monitor::Route;
using network_monitor::Station;
using network_monitor::TransportNetwork;

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_TransportNetwork);

BOOST_AUTO_TEST_SUITE(AddStation);

BOOST_AUTO_TEST_CASE(basic)
{
  TransportNetwork network{};
  bool ok{false};

  // Add a station.
  Station station{
      "station_000",
      "Station Name",
  };
  ok = network.AddStation(station);
  BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(duplicate_id)
{
  TransportNetwork network{};
  bool ok{false};

  // Can't add the same station twice.
  Station station{
      "station_000",
      "Station Name",
  };
  ok = network.AddStation(station);
  BOOST_REQUIRE(ok);
  ok = network.AddStation(station);
  BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_CASE(duplicate_name)
{
  TransportNetwork network{};
  bool ok{false};

  // It's ok to add a station with the same name, but different ID.
  Station station_0{
      "station_000",
      "Same Name",
  };
  ok = network.AddStation(station_0);
  BOOST_REQUIRE(ok);
  Station station_1{
      "station_001",
      "Same Name",
  };
  ok = network.AddStation(station_1);
  BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_SUITE_END();  // AddStation

BOOST_AUTO_TEST_SUITE(AddLine);

BOOST_AUTO_TEST_CASE(basic)
{
  TransportNetwork network{};
  bool ok{false};

  // Add a line with 1 route.
  // route_0: 0 ---> 1

  // First, add the stations.
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  BOOST_REQUIRE(ok);

  // Then, add the line, with the one route.
  Route route_0{
      "route_000",   "inbound",     "line_000",
      "station_000", "station_001", {"station_000", "station_001"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0},
  };
  ok = network.AddLine(line);
  BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(shared_stations)
{
  TransportNetwork network{};
  bool ok{false};

  // Define a line with 2 routes going through some shared stations.
  // route_0: 0 ---> 1 ---> 2
  // route_1: 3 ---> 1 ---> 2
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  Station station_2{
      "station_002",
      "Station Name 2",
  };
  Station station_3{
      "station_003",
      "Station Name 3",
  };
  Route route_0{
      "route_000",   "inbound",     "line_000",
      "station_000", "station_002", {"station_000", "station_001", "station_002"},
  };
  Route route_1{
      "route_001",   "inbound",     "line_000",
      "station_003", "station_002", {"station_003", "station_001", "station_002"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0, route_1},
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  ok &= network.AddStation(station_2);
  ok &= network.AddStation(station_3);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_CHECK(ok);
}

BOOST_AUTO_TEST_CASE(duplicate)
{
  TransportNetwork network{};
  bool ok{false};

  // Can't add the same line twice.
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  Route route_0{
      "route_000",   "inbound",     "line_000",
      "station_000", "station_001", {"station_000", "station_001"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0},
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_CHECK(!ok);
}

BOOST_AUTO_TEST_SUITE_END();  // AddLine

BOOST_AUTO_TEST_SUITE(PassengerEvents);

BOOST_AUTO_TEST_CASE(basic)
{
  TransportNetwork network{};
  bool ok{false};

  // Add a line with 1 route.
  // route_0: 0 ---> 1 ---> 2
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  Station station_2{
      "station_002",
      "Station Name 2",
  };
  Route route_0{
      "route_000",   "inbound",     "line_000",
      "station_000", "station_002", {"station_000", "station_001", "station_002"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0},
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  ok &= network.AddStation(station_2);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_REQUIRE(ok);

  // Check that the network starts empty.
  BOOST_REQUIRE_EQUAL(network.GetPassengerCount(station_0.id), 0);
  BOOST_REQUIRE_EQUAL(network.GetPassengerCount(station_1.id), 0);
  BOOST_REQUIRE_EQUAL(network.GetPassengerCount(station_2.id), 0);
  try {
    auto count{network.GetPassengerCount("station_42")};  // Not in the network
    BOOST_REQUIRE(false);
  } catch (const std::runtime_error& e) {
    BOOST_REQUIRE(true);
  }

  // Record events and check the count.
  using EventType = PassengerEvent::Type;
  ok = network.RecordPassengerEvent({station_0.id, EventType::In});
  BOOST_REQUIRE(ok);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_0.id), 1);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_1.id), 0);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_2.id), 0);
  ok = network.RecordPassengerEvent({station_0.id, EventType::In});
  BOOST_REQUIRE(ok);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_0.id), 2);
  ok = network.RecordPassengerEvent({station_1.id, EventType::In});
  BOOST_REQUIRE(ok);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_0.id), 2);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_1.id), 1);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_2.id), 0);
  ok = network.RecordPassengerEvent({station_0.id, EventType::Out});
  BOOST_REQUIRE(ok);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_0.id), 1);
  ok = network.RecordPassengerEvent({station_2.id, EventType::Out});  // Negative
  BOOST_REQUIRE(ok);
  BOOST_CHECK_EQUAL(network.GetPassengerCount(station_2.id), -1);
}

BOOST_AUTO_TEST_SUITE_END();  // PassengerEvents

BOOST_AUTO_TEST_SUITE(GetRoutesServingStation);

BOOST_AUTO_TEST_CASE(basic)
{
  TransportNetwork network{};
  bool ok{false};

  // Add a line with 1 route.
  // route_0: 0 ---> 1 ---> 2
  // Plus a station served by no routes: 3.
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  Station station_2{
      "station_002",
      "Station Name 2",
  };
  Station station_3{
      "station_003",
      "Station Name 3",
  };
  Route route_0{
      "route_000",   "inbound",     "line_000",
      "station_000", "station_002", {"station_000", "station_001", "station_002"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0},
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  ok &= network.AddStation(station_2);
  ok &= network.AddStation(station_3);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_REQUIRE(ok);

  // Check the routes served.
  std::vector<Id> routes{};
  routes = network.GetRoutesServingStation(station_0.id);
  BOOST_REQUIRE_EQUAL(routes.size(), 1);
  BOOST_CHECK(routes[0] == route_0.id);
  routes = network.GetRoutesServingStation(station_1.id);
  BOOST_REQUIRE_EQUAL(routes.size(), 1);
  BOOST_CHECK(routes[0] == route_0.id);
  routes = network.GetRoutesServingStation(station_2.id);
  BOOST_REQUIRE_EQUAL(routes.size(), 1);
  BOOST_CHECK(routes[0] == route_0.id);
  routes = network.GetRoutesServingStation(station_3.id);
  BOOST_CHECK_EQUAL(routes.size(), 0);
}

BOOST_AUTO_TEST_SUITE_END();  // GetRoutesServingStation

BOOST_AUTO_TEST_SUITE(TravelTime);

BOOST_AUTO_TEST_CASE(basic)
{
  TransportNetwork network{};
  bool ok{false};

  // Add a line with 1 route.
  // route_0: 0 ---> 1 ---> 2
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  Station station_2{
      "station_002",
      "Station Name 2",
  };
  Route route_0{
      "route_000",   "inbound",     "line_000",
      "station_000", "station_002", {"station_000", "station_001", "station_002"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0},
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  ok &= network.AddStation(station_2);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_REQUIRE(ok);

  unsigned int travel_time{0};

  // Get travel time before setting it.
  travel_time = network.GetTravelTime(station_0.id, station_1.id);
  BOOST_CHECK_EQUAL(travel_time, 0);

  // Cannot set the travel time between non-adjacent stations.
  ok = network.SetTravelTime(station_0.id, station_2.id, 1);
  BOOST_CHECK(!ok);

  // Set the travel time between adjacent stations.
  ok = network.SetTravelTime(station_0.id, station_1.id, 2);
  BOOST_CHECK(ok);
  BOOST_CHECK_EQUAL(network.GetTravelTime(station_0.id, station_1.id), 2);

  // Set travel time between adjacend stations, even if they appear in the
  // reverse order in the route.
  ok = network.SetTravelTime(station_1.id, station_0.id, 3);
  BOOST_CHECK(ok);
  BOOST_CHECK_EQUAL(network.GetTravelTime(station_1.id, station_0.id), 3);
}

BOOST_AUTO_TEST_CASE(over_route)
{
  TransportNetwork network{};
  bool ok{false};

  // Add a line with 3 routes.
  // route_0: 0 ---> 1 ---> 2 ---> 3
  // route_1: 3 ---> 1 ---> 2
  // route_2: 3 ---> 1 ---> 0
  Station station_0{
      "station_000",
      "Station Name 0",
  };
  Station station_1{
      "station_001",
      "Station Name 1",
  };
  Station station_2{
      "station_002",
      "Station Name 2",
  };
  Station station_3{
      "station_003",
      "Station Name 3",
  };
  Route route_0{
      "route_000",   "inbound",
      "line_000",    "station_000",
      "station_003", {"station_000", "station_001", "station_002", "station_003"},
  };
  Route route_1{
      "route_001",   "inbound",     "line_000",
      "station_003", "station_002", {"station_003", "station_001", "station_002"},
  };
  Route route_2{
      "route_002",   "inbound",     "line_000",
      "station_003", "station_000", {"station_003", "station_001", "station_000"},
  };
  Line line{
      "line_000",
      "Line Name",
      {route_0, route_1, route_2},
  };
  ok = true;
  ok &= network.AddStation(station_0);
  ok &= network.AddStation(station_1);
  ok &= network.AddStation(station_2);
  ok &= network.AddStation(station_3);
  BOOST_REQUIRE(ok);
  ok = network.AddLine(line);
  BOOST_REQUIRE(ok);

  // Set all travel times.
  ok = true;
  ok &= network.SetTravelTime(station_0.id, station_1.id, 1);
  ok &= network.SetTravelTime(station_1.id, station_2.id, 2);
  ok &= network.SetTravelTime(station_2.id, station_3.id, 3);
  ok &= network.SetTravelTime(station_3.id, station_1.id, 4);
  BOOST_REQUIRE(ok);

  // Check the cumulative travel times.
  unsigned int travel_time{0};
  // route_0
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_0.id, station_1.id), 1);
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_0.id, station_2.id), 1 + 2);
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_0.id, station_3.id), 1 + 2 + 3);
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_1.id, station_3.id), 2 + 3);
  // route_1
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_1.id, station_3.id, station_1.id), 4);
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_1.id, station_3.id, station_2.id), 4 + 2);
  // route_2
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_2.id, station_3.id, station_1.id), 4);
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_2.id, station_3.id, station_0.id), 4 + 1);
  // Invalid routes
  // -- 3 -> 1 is possible, but only over route_1 and route_2.
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_3.id, station_1.id), 0);
  // -- 1 -> 0 is possible, but only over route3.
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_1.id, station_0.id), 0);
  BOOST_CHECK_EQUAL(
      network.GetTravelTime(line.id, route_0.id, station_1.id, station_1.id), 0);
}

BOOST_AUTO_TEST_SUITE_END();  // TravelTime

BOOST_AUTO_TEST_SUITE(FromJson);

std::vector<Id> GetSortedIds(std::vector<Id>& routes)
{
  std::vector<Id> ids{routes};
  std::sort(ids.begin(), ids.end());
  return ids;
}

BOOST_AUTO_TEST_CASE(from_json_1line_1route)
{
  const auto test_file_path =
      std::filesystem::path(TESTS_RESOURCES_DIR) / "from_json_1line_1route.json";
  auto json_source = network_monitor::ParseJsonFile(test_file_path);

  TransportNetwork network{};
  auto ok{network.FromJson(std::move(json_source))};
  BOOST_REQUIRE(ok);

  auto routes{network.GetRoutesServingStation("station_0")};
  BOOST_REQUIRE_EQUAL(routes.size(), 1);
  BOOST_CHECK_EQUAL(routes[0], "route_0");
}

BOOST_AUTO_TEST_CASE(from_json_1line_2routes)
{
  auto test_file_path{std::filesystem::path(TESTS_RESOURCES_DIR) /
                      "from_json_1line_2routes.json"};
  auto json_source = network_monitor::ParseJsonFile(test_file_path);

  TransportNetwork network{};
  auto ok{network.FromJson(std::move(json_source))};
  BOOST_REQUIRE(ok);

  std::vector<Id> routes{};
  routes = network.GetRoutesServingStation("station_0");
  BOOST_REQUIRE_EQUAL(routes.size(), 1);
  BOOST_CHECK_EQUAL(routes[0], "route_0");
  routes = network.GetRoutesServingStation("station_1");
  BOOST_REQUIRE_EQUAL(routes.size(), 2);
  BOOST_CHECK(GetSortedIds(routes) == std::vector<Id>({"route_0", "route_1"}));
}

BOOST_AUTO_TEST_CASE(from_json_2lines_2routes)
{
  auto test_file_path{std::filesystem::path(TESTS_RESOURCES_DIR) /
                      "from_json_2lines_2routes.json"};
  auto json_source = network_monitor::ParseJsonFile(test_file_path);

  TransportNetwork network{};
  auto ok{network.FromJson(std::move(json_source))};
  BOOST_REQUIRE(ok);

  std::vector<Id> routes{};
  routes = network.GetRoutesServingStation("station_0");
  BOOST_REQUIRE_EQUAL(routes.size(), 2);
  BOOST_CHECK_EQUAL(routes[0], "route_0");
  BOOST_CHECK_EQUAL(routes[1], "route_1");
  routes = network.GetRoutesServingStation("station_1");
  BOOST_REQUIRE_EQUAL(routes.size(), 2);
  BOOST_CHECK(GetSortedIds(routes) == std::vector<Id>({"route_0", "route_1"}));
}

BOOST_AUTO_TEST_CASE(from_json_travel_times)
{
  auto test_file_path{std::filesystem::path(TESTS_RESOURCES_DIR) /
                      "from_json_travel_times.json"};
  auto json_source = network_monitor::ParseJsonFile(test_file_path);

  TransportNetwork network{};
  auto ok{network.FromJson(std::move(json_source))};
  BOOST_REQUIRE(ok);

  BOOST_CHECK_EQUAL(network.GetTravelTime("station_0", "station_1"), 1);
  BOOST_CHECK_EQUAL(network.GetTravelTime("station_1", "station_0"), 1);
  BOOST_CHECK_EQUAL(network.GetTravelTime("station_1", "station_2"), 2);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_0", "route_0", "station_0", "station_2"),
                    1 + 2);
}

BOOST_AUTO_TEST_CASE(fail_on_good_json_bad_times)
{
  nlohmann::json source{{"stations",
                         {{
                              {"station_id", "station_0"},
                              {"name", "Station 0 Name"},
                          },
                          {
                              {"station_id", "station_0"},
                              {"name", "Station 0 Name"},
                          }}

                        },
                        {"lines", {}},
                        {"travel_times", {}}};
  TransportNetwork network{};
  BOOST_CHECK_THROW(network.FromJson(std::move(source)), std::runtime_error);
}
BOOST_AUTO_TEST_CASE(fail_on_bad_travel_times)
{
  const auto test_file_path =
      std::filesystem::path(TESTS_RESOURCES_DIR) / "from_json_bad_travel_times.json";
  auto json_source = network_monitor::ParseJsonFile(test_file_path);

  TransportNetwork network{};
  auto ok{network.FromJson(std::move(json_source))};
  BOOST_REQUIRE(!ok);
}

BOOST_AUTO_TEST_SUITE_END();  // FromJson

BOOST_AUTO_TEST_SUITE_END();  // class_TransportNetwork

BOOST_AUTO_TEST_SUITE_END();  // websocket_client
