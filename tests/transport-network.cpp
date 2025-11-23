#include "network-monitor/transport-network.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <nlohmann/json.hpp>

#include "network-monitor/file-downloader.hpp"

namespace network_monitor {

BOOST_AUTO_TEST_SUITE(network_monitor);

BOOST_AUTO_TEST_SUITE(class_TransportNetwork);

BOOST_AUTO_TEST_SUITE(AddStation);

BOOST_AUTO_TEST_CASE(basic) {
  auto network = TransportNetwork{};
  BOOST_CHECK_EQUAL(network.AddStation({
                        .id = "station_000",
                        .name = "Station Name",
                    }),
                    true);
}

BOOST_AUTO_TEST_CASE(duplicated_id) {
  auto network = TransportNetwork{};

  // Can't add the same station twice.
  const auto station = Station{
      .id = "station_000",
      .name = "Station Name",
  };
  BOOST_CHECK_EQUAL(network.AddStation(station), true);
  BOOST_CHECK_EQUAL(network.AddStation(station), false);
}

BOOST_AUTO_TEST_CASE(duplicate_name) {
  auto network = TransportNetwork{};

  // It's ok to add a station with the same name, but different ID.
  BOOST_CHECK_EQUAL(network.AddStation(Station{
                        .id = "station_000",
                        .name = "Station Name",
                    }),
                    true);
  BOOST_CHECK_EQUAL(network.AddStation(Station{
                        .id = "station_001",
                        .name = "Station Name",
                    }),
                    true);
}

BOOST_AUTO_TEST_SUITE_END();  // AddStation

BOOST_AUTO_TEST_SUITE(AddLine);

BOOST_AUTO_TEST_CASE(basic) {
  auto network = TransportNetwork{};

  // Add a line with 1 route.
  // route_0: 0 ---> 1

  // Add the stations.
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });

  // Add the line with one route.
  BOOST_CHECK_EQUAL(network.AddLine(Line{
                        .id = "line_000",
                        .name = "Line Name",
                        .routes =
                            {
                                Route{
                                    .id = "route_000",
                                    .direction = "inbound",
                                    .line_id = "line_000",
                                    .start_station_id = "station_000",
                                    .end_station_id = "station_001",
                                    .stops =
                                        {
                                            "station_000",
                                            "station_001",
                                        },
                                },
                            },
                    }),
                    true);
}

BOOST_AUTO_TEST_CASE(shared_stations) {
  auto network = TransportNetwork{};

  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });
  network.AddStation(Station{
      .id = "station_002",
      .name = "Station Name 2",
  });
  network.AddStation(Station{
      .id = "station_003",
      .name = "Station Name 3",
  });

  // Define a line with 2 routes going through some shared stations.
  // route_0: 0 ---> 1 ---> 2
  // route_1: 3 ---> 1 ---> 2
  BOOST_CHECK_EQUAL(network.AddLine(Line{
                        .id = "line_000",
                        .name = "Line Name",
                        .routes =
                            {
                                Route{
                                    .id = "route_000",
                                    .direction = "inbound",
                                    .line_id = "line_000",
                                    .start_station_id = "station_000",
                                    .end_station_id = "station_002",
                                    .stops =
                                        {
                                            "station_000",
                                            "station_001",
                                            "station_002",
                                        },
                                },
                                Route{
                                    .id = "route_001",
                                    .direction = "inbound",
                                    .line_id = "line_000",
                                    .start_station_id = "station_003",
                                    .end_station_id = "station_002",
                                    .stops =
                                        {
                                            "station_003",
                                            "station_001",
                                            "station_002",
                                        },
                                },
                            },
                    }),
                    true);
}

BOOST_AUTO_TEST_CASE(duplicated) {
  auto network = TransportNetwork{};

  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });

  const auto line = Line{
      .id = "line_000",
      .name = "Line Name",
      .routes =
          {
              Route{
                  .id = "route_000",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_000",
                  .end_station_id = "station_001",
                  .stops =
                      {
                          "station_000",
                          "station_001",
                      },
              },
          },
  };

  // Can't add the same line twice.
  BOOST_CHECK_EQUAL(network.AddLine(line), true);
  BOOST_CHECK_EQUAL(network.AddLine(line), false);
}

BOOST_AUTO_TEST_SUITE_END();  // AddLine

BOOST_AUTO_TEST_SUITE(PassengerEvents);

BOOST_AUTO_TEST_CASE(basic) {
  auto network = TransportNetwork{};

  // Add a line with 1 route.
  // route_0: 0 ---> 1 ---> 2
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });
  network.AddStation(Station{
      .id = "station_002",
      .name = "Station Name 2",
  });
  network.AddLine(Line{
      .id = "line_000",
      .name = "Line Name",
      .routes =
          {
              Route{
                  .id = "route_000",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_000",
                  .end_station_id = "station_002",
                  .stops =
                      {
                          "station_000",
                          "station_001",
                          "station_002",
                      },
              },
          },
  });

  // Record events and check the count.
  BOOST_CHECK_EQUAL(network.RecordPassengerEvent(PassengerEvent{
                        .station_id = "station_000",
                        .type = PassengerEvent::Type::In,
                    }),
                    true);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_000"), 1);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_001"), 0);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_002"), 0);

  BOOST_CHECK_EQUAL(network.RecordPassengerEvent(PassengerEvent{
                        .station_id = "station_000",
                        .type = PassengerEvent::Type::In,
                    }),
                    true);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_000"), 2);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_001"), 0);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_002"), 0);

  BOOST_CHECK_EQUAL(network.RecordPassengerEvent(PassengerEvent{
                        .station_id = "station_001",
                        .type = PassengerEvent::Type::In,
                    }),
                    true);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_000"), 2);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_001"), 1);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_002"), 0);

  BOOST_CHECK_EQUAL(network.RecordPassengerEvent(PassengerEvent{
                        .station_id = "station_000",
                        .type = PassengerEvent::Type::Out,
                    }),
                    true);
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_000"), 1);
}

BOOST_AUTO_TEST_CASE(GetPassengerCountThrowsErrorOnNonExistingStation) {
  const auto network = TransportNetwork{};
  BOOST_CHECK_THROW(network.GetPassengerCount("non_existing"),
                    std::runtime_error);
}

BOOST_AUTO_TEST_CASE(GetPassengerCountStartWithZero) {
  auto network = TransportNetwork{};
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_000"), 0);
}

BOOST_AUTO_TEST_CASE(GetPassengerCountCanGoNegativeNumbers) {
  auto network = TransportNetwork{};
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.RecordPassengerEvent(PassengerEvent{
      .station_id = "station_000",
      .type = PassengerEvent::Type::Out,
  });
  BOOST_CHECK_EQUAL(network.GetPassengerCount("station_000"), -1);
}

BOOST_AUTO_TEST_SUITE_END();  // PassengerEvents

BOOST_AUTO_TEST_SUITE(GetRoutesServingStation);

BOOST_AUTO_TEST_CASE(basic) {
  auto network = TransportNetwork{};

  // Add a line with 1 route.
  // route_0: 0 ---> 1 ---> 2
  // Plus a station served by no routes: 3.
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });
  network.AddStation(Station{
      .id = "station_002",
      .name = "Station Name 2",
  });
  network.AddStation(Station{
      .id = "station_003",
      .name = "Station Name 3",
  });
  network.AddLine(Line{
      .id = "line_000",
      .name = "Line Name",
      .routes =
          {
              Route{
                  .id = "route_000",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_000",
                  .end_station_id = "station_002",
                  .stops =
                      {
                          "station_000",
                          "station_001",
                          "station_002",
                      },
              },
          },
  });

  // TODO: check multiple elements with gtest
  BOOST_TEST(network.GetRoutesServingStation("station_000") ==
                 std::vector<Id>{"route_000"},
             boost::test_tools::per_element());
  BOOST_TEST(network.GetRoutesServingStation("station_001") ==
                 std::vector<Id>{"route_000"},
             boost::test_tools::per_element());
  BOOST_TEST(network.GetRoutesServingStation("station_002") ==
                 std::vector<Id>{"route_000"},
             boost::test_tools::per_element());
  BOOST_TEST(
      network.GetRoutesServingStation("station_003") == std::vector<Id>{},
      boost::test_tools::per_element());
}

BOOST_AUTO_TEST_SUITE_END();  // GetRoutesServingStation

BOOST_AUTO_TEST_SUITE(TravelTime);

BOOST_AUTO_TEST_CASE(basic) {
  auto network = TransportNetwork{};

  // Add a line with 1 route.
  // route_0: 0 ---> 1 ---> 2
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });
  network.AddStation(Station{
      .id = "station_002",
      .name = "Station Name 2",
  });
  network.AddLine(Line{
      .id = "line_000",
      .name = "Line Name",
      .routes =
          {
              Route{
                  .id = "route_000",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_000",
                  .end_station_id = "station_002",
                  .stops =
                      {
                          "station_000",
                          "station_001",
                          "station_002",
                      },
              },
          },
  });

  // Get travel time before setting it.
  BOOST_CHECK_EQUAL(network.GetTravelTime("station_000", "station_001"), 0);

  // Cannot set the travel time between non-adjacent stations.
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_000", "station_002", 1),
                    false);

  // Set the travel time between adjacent stations.
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_000", "station_001", 2),
                    true);
  BOOST_CHECK_EQUAL(network.GetTravelTime("station_000", "station_001"), 2);

  // Set travel time between adjacend stations, even if they appear in the
  // reverse order in the route.
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_001", "station_000", 3),
                    true);
  BOOST_CHECK_EQUAL(network.GetTravelTime("station_001", "station_000"), 3);
}

BOOST_AUTO_TEST_CASE(over_route) {
  auto network = TransportNetwork{};

  // Add a line with 3 routes.
  // route_0: 0 ---> 1 ---> 2 ---> 3
  // route_1: 3 ---> 1 ---> 2
  // route_2: 3 ---> 1 ---> 0
  network.AddStation(Station{
      .id = "station_000",
      .name = "Station Name 0",
  });
  network.AddStation(Station{
      .id = "station_001",
      .name = "Station Name 1",
  });
  network.AddStation(Station{
      .id = "station_002",
      .name = "Station Name 2",
  });
  network.AddStation(Station{
      .id = "station_003",
      .name = "Station Name 3",
  });
  network.AddLine(Line{
      .id = "line_000",
      .name = "Line Name",
      .routes =
          {
              Route{
                  .id = "route_000",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_000",
                  .end_station_id = "station_003",
                  .stops =
                      {
                          "station_000",
                          "station_001",
                          "station_002",
                          "station_003",
                      },
              },
              Route{
                  .id = "route_001",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_003",
                  .end_station_id = "station_002",
                  .stops =
                      {
                          "station_003",
                          "station_001",
                          "station_002",
                      },
              },
              Route{
                  .id = "route_002",
                  .direction = "inbound",
                  .line_id = "line_000",
                  .start_station_id = "station_003",
                  .end_station_id = "station_000",
                  .stops =
                      {
                          "station_003",
                          "station_001",
                          "station_000",
                      },
              },
          },
  });

  // Set all travel times.
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_000", "station_001", 1),
                    true);
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_001", "station_002", 2),
                    true);
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_002", "station_003", 3),
                    true);
  BOOST_CHECK_EQUAL(network.SetTravelTime("station_003", "station_001", 4),
                    true);

  // Check the cumulative travel times.
  // route_0
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_000", "station_001"),
                    1);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_000", "station_002"),
                    1 + 2);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_000", "station_003"),
                    1 + 2 + 3);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_001", "station_003"),
                    2 + 3);
  // route_1
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_001",
                                          "station_003", "station_001"),
                    4);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_001",
                                          "station_003", "station_002"),
                    4 + 2);
  // route_2
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_002",
                                          "station_003", "station_001"),
                    4);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_002",
                                          "station_003", "station_000"),
                    4 + 1);
  // Invalid routes
  // -- 3 -> 1 is possible, but only over route_1 and route_2.
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_003", "station_001"),
                    0);
  // -- 1 -> 0 is possible, but only over route3.
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_001", "station_000"),
                    0);
  BOOST_CHECK_EQUAL(network.GetTravelTime("line_000", "route_000",
                                          "station_001", "station_001"),
                    0);
}

BOOST_AUTO_TEST_SUITE_END();  // TravelTime

BOOST_AUTO_TEST_SUITE(FromJson);

namespace {
std::vector<Id> SortIds(std::vector<Id>& routes) {
  std::vector<Id> ids{routes};
  std::ranges::sort(ids);
  return ids;
}
}  // namespace

BOOST_AUTO_TEST_CASE(from_json_1line_1route) {
  const auto test_file_path = std::filesystem::path(TESTS_RESOURCES_DIR) /
                              "from_json_1line_1route.json";
  auto json_source = ParseJsonFile(test_file_path);

  auto network = TransportNetwork::FromJson(json_source);
  BOOST_REQUIRE(network.has_value());

  auto routes{network.value().GetRoutesServingStation("station_0")};
  BOOST_TEST(network.value().GetRoutesServingStation("station_0") ==
             std::vector<Id>{"route_0"});
}

BOOST_AUTO_TEST_CASE(from_json_1line_2routes) {
  auto test_file_path{std::filesystem::path(TESTS_RESOURCES_DIR) /
                      "from_json_1line_2routes.json"};
  auto json_source = ParseJsonFile(test_file_path);

  auto network = TransportNetwork::FromJson(json_source);
  BOOST_REQUIRE(network.has_value());

  std::vector<Id> routes{};
  routes = network.value().GetRoutesServingStation("station_0");
  BOOST_REQUIRE_EQUAL(routes.size(), 1);
  BOOST_CHECK_EQUAL(routes[0], "route_0");
  routes = network.value().GetRoutesServingStation("station_1");
  BOOST_REQUIRE_EQUAL(routes.size(), 2);
  BOOST_CHECK(SortIds(routes) == std::vector<Id>({"route_0", "route_1"}));
}

BOOST_AUTO_TEST_CASE(from_json_2lines_2routes) {
  auto test_file_path{std::filesystem::path(TESTS_RESOURCES_DIR) /
                      "from_json_2lines_2routes.json"};
  auto json_source = ParseJsonFile(test_file_path);

  auto network = TransportNetwork::FromJson(json_source);
  BOOST_REQUIRE(network.has_value());

  std::vector<Id> routes{};
  routes = network.value().GetRoutesServingStation("station_0");
  BOOST_REQUIRE_EQUAL(routes.size(), 2);
  BOOST_CHECK_EQUAL(routes[0], "route_0");
  BOOST_CHECK_EQUAL(routes[1], "route_1");
  routes = network.value().GetRoutesServingStation("station_1");
  BOOST_REQUIRE_EQUAL(routes.size(), 2);
  BOOST_CHECK(SortIds(routes) == std::vector<Id>({"route_0", "route_1"}));
}

BOOST_AUTO_TEST_CASE(from_json_travel_times) {
  auto test_file_path{std::filesystem::path(TESTS_RESOURCES_DIR) /
                      "from_json_travel_times.json"};
  auto json_source = ParseJsonFile(test_file_path);

  auto network = TransportNetwork::FromJson(json_source);
  BOOST_REQUIRE(network.has_value());

  BOOST_CHECK_EQUAL(network.value().GetTravelTime("station_0", "station_1"), 1);
  BOOST_CHECK_EQUAL(network.value().GetTravelTime("station_1", "station_0"), 1);
  BOOST_CHECK_EQUAL(network.value().GetTravelTime("station_1", "station_2"), 2);
  BOOST_CHECK_EQUAL(network.value().GetTravelTime("line_0", "route_0",
                                                  "station_0", "station_2"),
                    1 + 2);
}

BOOST_AUTO_TEST_CASE(fail_on_good_json_bad_times) {
  const auto json_source = nlohmann::json{R"JSON(
{
    "stations": [
        {
            "station_id": "station_0",
            "name": "Station 0 Name"
        },
        {
            "station_id": "station_1",
            "name": "Station 1 Name"
        }
    ],
    "lines": [],
    "travel_times": []
})JSON"_json};

  auto network = TransportNetwork::FromJson(json_source);
  BOOST_REQUIRE(!network.has_value());
}

BOOST_AUTO_TEST_CASE(fail_on_bad_travel_times) {
  const auto test_file_path = std::filesystem::path(TESTS_RESOURCES_DIR) /
                              "from_json_bad_travel_times.json";
  auto json_source = ParseJsonFile(test_file_path);

  auto network = TransportNetwork::FromJson(json_source);
  BOOST_REQUIRE(!network.has_value());
}

BOOST_AUTO_TEST_SUITE_END();  // FromJson

BOOST_AUTO_TEST_SUITE_END();  // class_TransportNetwork

BOOST_AUTO_TEST_SUITE_END();  // websocket_client

}  // namespace network_monitor
