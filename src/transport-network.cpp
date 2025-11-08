#include "network-monitor/transport-network.hpp"

#include <algorithm>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

using namespace network_monitor;

bool Station::operator==(const Station& other) const {
  return id == other.id;
}

bool Route::operator==(const Route& other) const {
  return id == other.id;
}

bool Line::operator==(const Line& other) const {
  return id == other.id;
}

TransportNetwork::TransportNetwork() = default;

TransportNetwork::~TransportNetwork() = default;

TransportNetwork::TransportNetwork(const TransportNetwork& copied) = default;

TransportNetwork::TransportNetwork(TransportNetwork&& moved) noexcept
    : stations_{std::move(moved.stations_)},
      lines_{std::move(moved.lines_)} {}

bool TransportNetwork::FromJson(const nlohmann::json& source) {
  for (const auto& station : source.at("stations")) {
    Station new_station{};
    new_station.id = station.at("station_id").get<Id>();
    new_station.name = station.at("name").get<std::string>();
    auto success = AddStation(new_station);
    if (!success) {
      throw std::runtime_error("Adding station failed [id: " + new_station.id +
                               ", name: " + new_station.name + "]");
    }
  }

  for (const auto& line : source.at("lines")) {
    Line new_line{};
    new_line.id = line.at("line_id").get<Id>();
    new_line.name = line.at("name").get<std::string>();
    for (const auto& route : line.at("routes")) {
      Route new_route{};
      new_route.id = route.at("route_id").get<Id>();
      new_route.direction = route.at("direction").get<std::string>();
      new_route.start_station_id = route.at("start_station_id").get<Id>();
      new_route.end_station_id = route.at("end_station_id").get<Id>();
      for (const auto& stop : route.at("route_stops")) {
        new_route.stops.push_back(stop.get<Id>());
      }
      new_line.routes.push_back(std::move(new_route));
    }
    auto success = AddLine(new_line);
    if (!success) {
      throw std::runtime_error("Adding line failed [id: " + new_line.id +
                               ", name: " + new_line.name + "]");
    }
  }

  const auto& travel_times = source.at("travel_times");
  return std::ranges::all_of(travel_times, [this](const auto& travel_time) {
    return SetTravelTime(
        travel_time.at("start_station_id").template get<Id>(),
        travel_time.at("end_station_id").template get<Id>(),
        travel_time.at("travel_time").template get<unsigned>());
  });
}

bool TransportNetwork::AddStation(const Station& station) {
  if (StationExists(station.id)) {
    return false;
  }

  AddStationInternal(station);
  return true;
}

bool TransportNetwork::AddLine(const Line& line) {
  if (LineExists(line)) {
    return false;
  }

  // FIXME: Move this check to the time of creating a list of stations.
  //        So to the interation over route.stops down below?
  //        Do this rather as a part of the optimization phase.
  if (!StationsExist(line.routes)) {
    return false;
  }

  // FIXME: Should not insert the newly created line into the lines_.
  //        The function actually becomes unneccessary anymore. Make here
  //        a direct call to std::make_shared and insert it only at the end.
  auto line_internal = CreateLineInternal(line.id, line.name);

  for (const auto& route : line.routes) {
    // FIXME: Can be written more clearly or moved to the separate function.
    //        It does "return false if the route already existis in the line".
    if (std::find_if(line_internal->routes.begin(), line_internal->routes.end(),
                     [&route](auto line_route) {
                       return route.id == line_route->id;
                     }) != line_internal->routes.end()) {
      return false;
    }

    // FIXME: Gather the list of stations first.
    std::vector<std::shared_ptr<GraphNode>> stops{};
    stops.reserve(route.stops.size());
    for (const auto& stop_id : route.stops) {
      // FIXME: Create a separate function for getting a station. In purpose of
      //       readability.
      if (!StationExists(stop_id)) {
        return false;
      }
      const auto station = stations_.at(stop_id);
      stops.push_back(station);
    }

    auto route_internal =
        std::make_shared<RouteInternal>(route.id, line_internal);
    route_internal->stations = std::move(stops);

    for (unsigned index = 0; index < route.stops.size() - 1; index++) {
      auto& current_stop = route_internal->stations.at(index);
      auto& next_station = route_internal->stations.at(index + 1);
      auto graph_edge =
          std::make_shared<GraphEdge>(route_internal, next_station);
      current_stop->edges.push_back(std::move(graph_edge));
    }
    line_internal->routes.push_back(route_internal);
  }
  return true;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event) {
  if (!StationExists(event.station_id)) {
    return false;
  }

  auto& passenger_count = stations_.at(event.station_id)->passenger_count;

  switch (event.type) {
    case PassengerEvent::Type::In: {
      passenger_count++;
      break;
    }
    case PassengerEvent::Type::Out: {
      passenger_count--;
      break;
    }
    default:
      return false;
  }

  return true;
}

long long int TransportNetwork::GetPassengerCount(const Id& station) const {
  if (!StationExists(station)) {
    throw std::runtime_error("Station id '" + station + "' unknown");
  }
  return stations_.at(station)->passenger_count;
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(
    const Id& station) const {
  std::vector<Id> routes;
  if (!StationExists(station)) {
    return routes;
  }

  const auto& edges = stations_.at(station)->edges;
  routes.reserve(edges.size());
  for (const auto& edge : edges) {
    routes.push_back(edge->route->id);
  }

  for (const auto& [_, line] : lines_) {
    for (const auto& route : line->routes) {
      const auto& end_stop = route->stations.back();
      if (station == end_stop->id) {
        routes.push_back(route->id);
      }
    }
  }

  return routes;
}

bool TransportNetwork::SetTravelTime(const Id& station_a,
                                     const Id& station_b,
                                     const unsigned int travel_time) {
  if (!StationExists(station_a) || !StationExists(station_b) ||
      station_a == station_b) {
    return false;
  }

  if (!StationsAreAdjacend(station_a, station_b)) {
    return false;
  }

  for (auto& edge :
       stations_.at(station_a)->FindEdgesToNextStation(station_b)) {
    edge->travel_time = travel_time;
  }
  for (auto& edge :
       stations_.at(station_b)->FindEdgesToNextStation(station_a)) {
    edge->travel_time = travel_time;
  }
  return true;
}

unsigned int TransportNetwork::GetTravelTime(const Id& station_a,
                                             const Id& station_b) const {
  if (!StationExists(station_a) || !StationExists(station_b) ||
      station_a == station_b) {
    return 0;
  }

  const auto edges_from_a_to_b =
      stations_.at(station_a)->FindEdgesToNextStation(station_b);
  if (!edges_from_a_to_b.empty()) {
    return edges_from_a_to_b.front()->travel_time;
  }

  const auto edges_from_b_to_a =
      stations_.at(station_b)->FindEdgesToNextStation(station_a);
  if (!edges_from_b_to_a.empty()) {
    return edges_from_b_to_a.front()->travel_time;
  }

  return 0;
}

unsigned int TransportNetwork::GetTravelTime(const Id& line,
                                             const Id& route,
                                             const Id& station_a,
                                             const Id& station_b) const {
  unsigned int total_travel_time{0};

  if (station_a == station_b) {
    return total_travel_time;
  }

  if (!lines_.count(line)) {
    return total_travel_time;
  }

  // FIXME: Turn the below code into the new private method named FindRoute.
  const auto& line_routes = lines_.at(line)->routes;
  const auto route_internal = std::find_if(
      line_routes.begin(), line_routes.end(),
      [route](auto line_route) { return line_route->id == route; });
  if (route_internal == line_routes.end()) {
    return total_travel_time;
  }

  // Find the stations.
  const auto station_a_node = stations_.at(station_a);
  const auto station_b_node = stations_.at(station_b);

  // Walk the route looking for station A.
  bool in_range{false};
  for (const auto& stop : (*route_internal)->stations) {
    if (stop->id == station_a) {
      in_range = true;
    }
    if (stop->id == station_b) {
      return total_travel_time;
    }

    if (in_range) {
      auto edge = stop->FindEdgesForRoute(*route_internal);
      if (edge.empty()) {
        return 0;
      }
      total_travel_time += edge.front()->travel_time;
    }
  }

  return total_travel_time;
}

TransportNetwork::GraphNode::GraphNode(const Station& station)
    : id{station.id},
      name{station.name} {}

std::vector<std::shared_ptr<TransportNetwork::GraphEdge>>
TransportNetwork::GraphNode::FindEdgesToNextStation(
    const Id& next_station) const {
  std::vector<std::shared_ptr<TransportNetwork::GraphEdge>>
      edges_to_next_station;
  for (const auto& edge : edges) {
    if (edge->next_station->id == next_station) {
      edges_to_next_station.push_back(edge);
    }
  }
  return edges_to_next_station;
}

std::vector<std::shared_ptr<TransportNetwork::GraphEdge>>
TransportNetwork::GraphNode::FindEdgesForRoute(
    const std::shared_ptr<RouteInternal>& route) const {
  std::vector<std::shared_ptr<TransportNetwork::GraphEdge>> edges_for_route;
  std::copy_if(edges.cbegin(), edges.cend(),
               std::back_inserter(edges_for_route),
               [&route](auto& edge) { return edge->route->id == route->id; });
  return edges_for_route;
}

TransportNetwork::GraphEdge::GraphEdge(
    const std::shared_ptr<RouteInternal>& route,
    const std::shared_ptr<GraphNode>& next_station)
    : route{route},
      next_station{next_station} {}

void TransportNetwork::AddStationInternal(const Station& station) {
  auto node = std::make_shared<GraphNode>(station);
  stations_.emplace(station.id, std::move(node));
}

std::shared_ptr<TransportNetwork::LineInternal>
TransportNetwork::CreateLineInternal(const Id& id, const std::string& name) {
  return lines_.emplace(id, std::make_shared<LineInternal>(id, name))
      .first->second;
}

bool TransportNetwork::StationExists(const Id& station_id) const {
  return stations_.count(station_id);
}

bool TransportNetwork::StationsExist(const std::vector<Route>& routes) const {
  return std::ranges::all_of(
      routes, [this](const auto& route) { return StationsExist(route.stops); });
}

bool TransportNetwork::StationsExist(const std::vector<Id>& stations) const {
  return std::ranges::all_of(stations, [this](const auto& station_id) {
    return StationExists(station_id);
  });
}

bool TransportNetwork::LineExists(const Line& line) const {
  return lines_.count(line.id);
}

bool TransportNetwork::StationsAreAdjacend(const Id& station_a,
                                           const Id& station_b) const {
  return StationConnectsAnother(station_a, station_b) ||
         StationConnectsAnother(station_b, station_a);
}

bool TransportNetwork::StationConnectsAnother(const Id& station_a,
                                              const Id& station_b) const {
  const auto& station_a_internal = stations_.at(station_a);
  return !stations_.at(station_a)->FindEdgesToNextStation(station_b).empty();
}

TransportNetwork::RouteInternal::RouteInternal(
    Id id, const std::shared_ptr<LineInternal>& line)
    : id{std::move(id)},
      line{line} {}

TransportNetwork::LineInternal::LineInternal(Id id, std::string name)
    : id{std::move(id)},
      name{std::move(name)} {}
