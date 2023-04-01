#include <algorithm>
#include <network-monitor/transport-network.hpp>
#include <stdexcept>

using namespace NetworkMonitor;

bool Station::operator==(const Station& other) const
{
    return id == other.id;
}

bool Route::operator==(const Route& other) const
{
    return id == other.id;
}

bool Line::operator==(const Line& other) const
{
    return id == other.id;
}

TransportNetwork::TransportNetwork() {}

TransportNetwork::~TransportNetwork() {}

TransportNetwork::TransportNetwork(const TransportNetwork& copied)
    : stations_{copied.stations_},
      lines_{copied.lines_}
{
}

TransportNetwork::TransportNetwork(TransportNetwork&& moved)
    : stations_{std::move(moved.stations_)},
      lines_{std::move(moved.lines_)}
{
}

// TransportNetwork& TransportNetwork::operator=(const TransportNetwork& copied)
// {
//     // todo
//     return *this;
// }

// TransportNetwork& TransportNetwork::operator=(TransportNetwork&& moved)
// {
//     // todo
//     return *this;
// }

bool TransportNetwork::FromJson(nlohmann::json&& source)
{
    for (const auto& station : source.at("stations")) {
        Station new_station{};
        new_station.id = station.at("station_id").get<Id>();
        new_station.name = station.at("name").get<std::string>();
        auto result = AddStation(new_station);
        if (result == false) {
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
        auto result = AddLine(new_line);
        if (result == false) {
            throw std::runtime_error("Adding line failed [id: " + new_line.id +
                                     ", name: " + new_line.name + "]");
        }
    }

    for (const auto& travel_time : source.at("travel_times")) {
        auto result = SetTravelTime(travel_time.at("start_station_id").get<Id>(),
                                    travel_time.at("end_station_id").get<Id>(),
                                    travel_time.at("travel_time").get<unsigned>());
        if (result == false) {
            return false;
        }
    }

    return true;
}

bool TransportNetwork::AddStation(const Station& station)
{
    if (StationExists(station.id)) {
        return false;
    }

    AddStationInternal(station);
    return true;
}

bool TransportNetwork::AddLine(const Line& line)
{
    if (LineExists(line)) {
        return false;
    }

    // TODO: Move this check to the time of creating a list of stations.
    if (!StationsExist(line.routes)) {
        return false;
    }

    // TODO: Should not insert the newly created line into the lines_. The function
    //       actually becomes unneccessary anymore. Make here a direct call to
    //       std::make_shared and insert it only at the end.
    auto line_internal = CreateLineInternal(line.id, line.name);

    for (const auto& route : line.routes) {
        // TODO: Can be written more clearly or moved to the separate function. It does
        //       "return false if the route already existis in the line".
        if (std::find_if(line_internal->routes.begin(), line_internal->routes.end(),
                         [&route](auto line_route) {
                             return route.id == line_route->id;
                         }) != line_internal->routes.end()) {
            return false;
        }

        // TODO: Gather the list of stations first.
        std::vector<std::shared_ptr<GraphNode>> stops{};
        stops.reserve(route.stops.size());
        for (const auto& stop_id : route.stops) {
            // TODO: Create a separate function for getting a station. In purpose of
            //       readability.
            if (!StationExists(stop_id)) {
                return false;
            }
            const auto station = stations_.at(stop_id);
            stops.push_back(station);
        }

        auto route_internal = std::make_shared<RouteInternal>(route.id, line_internal);
        route_internal->stations = std::move(stops);

        for (unsigned index = 0; index < route.stops.size() - 1; index++) {
            auto& current_stop = route_internal->stations.at(index);
            auto& next_station = route_internal->stations.at(index + 1);
            auto graph_edge = std::make_shared<GraphEdge>(route_internal, next_station);
            current_stop->edges.push_back(std::move(graph_edge));
        }
        line_internal->routes.push_back(route_internal);
    }
    return true;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event)
{
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

long long int TransportNetwork::GetPassengerCount(const Id& station) const
{
    if (!StationExists(station)) {
        throw std::runtime_error("Station id '" + station + "' unknown");
    }
    return stations_.at(station)->passenger_count;
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(const Id& station) const
{
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
                                     const unsigned int travel_time)
{
    if (!StationExists(station_a) || !StationExists(station_b) ||
        station_a == station_b) {
        return false;
    }

    if (!StationsAreAdjacend(station_a, station_b)) {
        return false;
    }

    for (auto& edge : stations_.at(station_a)->FindEdgesToNextStation(station_b)) {
        edge->travel_time = travel_time;
    }
    for (auto& edge : stations_.at(station_b)->FindEdgesToNextStation(station_a)) {
        edge->travel_time = travel_time;
    }
    return true;
}

unsigned int TransportNetwork::GetTravelTime(const Id& station_a,
                                             const Id& station_b) const
{
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
                                             const Id& station_b) const
{
    unsigned int total_travel_time{0};

    if (station_a == station_b) {
        return total_travel_time;
    }

    if (!lines_.count(line)) {
        return total_travel_time;
    }

    // TODO: Make the FindRoute function from the below.
    const auto& line_routes = lines_.at(line)->routes;
    const auto route_internal =
        std::find_if(line_routes.begin(), line_routes.end(),
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
      name{station.name}
{
}

std::vector<std::shared_ptr<TransportNetwork::GraphEdge>>
TransportNetwork::GraphNode::FindEdgesToNextStation(const Id& next_station) const
{
    std::vector<std::shared_ptr<TransportNetwork::GraphEdge>> edges_to_next_station;
    for (const auto& edge : edges) {
        if (edge->next_station->id == next_station) {
            edges_to_next_station.push_back(edge);
        }
    }
    return edges_to_next_station;
}

std::vector<std::shared_ptr<TransportNetwork::GraphEdge>>
TransportNetwork::GraphNode::FindEdgesForRoute(
    const std::shared_ptr<RouteInternal>& route) const
{
    std::vector<std::shared_ptr<TransportNetwork::GraphEdge>> edges_for_route;
    std::copy_if(edges.cbegin(), edges.cend(), std::back_inserter(edges_for_route),
                 [&route](auto& edge) { return edge->route->id == route->id; });
    return edges_for_route;
}

TransportNetwork::GraphEdge::GraphEdge(const std::shared_ptr<RouteInternal>& route,
                                       const std::shared_ptr<GraphNode>& next_station)
    : route{route},
      next_station{next_station}
{
}

void TransportNetwork::AddStationInternal(const Station& station)
{
    auto node = std::make_shared<GraphNode>(station);
    stations_.emplace(station.id, std::move(node));
}

std::shared_ptr<TransportNetwork::LineInternal> TransportNetwork::CreateLineInternal(
    const Id& id, const std::string& name)
{
    return lines_.emplace(id, std::make_shared<LineInternal>(id, name)).first->second;
}

bool TransportNetwork::StationExists(const Id& station_id) const
{
    return stations_.count(station_id);
}

bool TransportNetwork::StationsExist(const std::vector<Route>& routes) const
{
    for (const auto& route : routes) {
        if (!StationsExist(route.stops)) {
            return false;
        }
    }
    return true;
}

bool TransportNetwork::StationsExist(const std::vector<Id>& stations) const
{
    for (const auto& station_id : stations) {
        if (!StationExists(station_id)) {
            return false;
        }
    }
    return true;
}

bool TransportNetwork::LineExists(const Line& line) const
{
    return lines_.count(line.id);
}

bool TransportNetwork::StationsAreAdjacend(const Id& station_a, const Id& station_b) const
{
    return StationConnectsAnother(station_a, station_b) ||
           StationConnectsAnother(station_b, station_a);
}

bool TransportNetwork::StationConnectsAnother(const Id& station_a,
                                              const Id& station_b) const
{
    const auto& station_a_internal = stations_.at(station_a);
    if (stations_.at(station_a)->FindEdgesToNextStation(station_b).empty()) {
        return false;
    }
    return true;
}

TransportNetwork::RouteInternal::RouteInternal(const Id& id,
                                               const std::shared_ptr<LineInternal>& line)
    : id{id},
      line{line}
{
}
TransportNetwork::LineInternal::LineInternal(const Id& id, const std::string& name)
    : id{id},
      name{name}
{
}
