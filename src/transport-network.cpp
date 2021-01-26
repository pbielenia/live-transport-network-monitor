
#include <set>
#include <transport-network.hpp>

bool network_monitor::Station::operator==(const Station& other) const
{
    return id == other.id;
}

bool network_monitor::Route::operator==(const Route& other) const
{
    return id == other.id;
}

bool network_monitor::Line::operator==(const network_monitor::Line& other) const
{
    return id == other.id;
}

network_monitor::TransportNetwork::TransportNetwork() {}

network_monitor::TransportNetwork::TransportNetwork(
    const network_monitor::TransportNetwork& other)
{
}

network_monitor::TransportNetwork::TransportNetwork(
    network_monitor::TransportNetwork&& other) noexcept
{
}

network_monitor::TransportNetwork& network_monitor::TransportNetwork::operator=(
    const network_monitor::TransportNetwork& other)
{
    return *this;
}

network_monitor::TransportNetwork& network_monitor::TransportNetwork::operator=(
    network_monitor::TransportNetwork&& other) noexcept
{
    return *this;
}

network_monitor::TransportNetwork::GraphNode::GraphNode(Station station)
    : station(station)
{
}

network_monitor::TransportNetwork::LineInternal::LineInternal(Id id, std::string name)
    : id{std::move(id)},
      name{std::move(name)}
{
}

bool network_monitor::TransportNetwork::add_station(
    const network_monitor::Station& station)
{
    if (stations.contains(station.id)) {
        return false;
    }

    stations[station.id] = std::make_shared<GraphNode>(station);
    return true;
}

bool network_monitor::TransportNetwork::add_line(const network_monitor::Line& line)
{
    //  All stations served by this line must already be in the network.
    for (const auto& route : line.routes) {
        if (!stations.contains(route.start_station_id)
            or !stations.contains(route.end_station_id)) {
            return false;
        }
        for (const auto& stop : route.stops) {
            if (!stations.contains(stop)) {
                return false;
            }
        }
    }

    //  The line cannot already be in the network.
    if (lines.contains(line.id)) {
        return false;
    }

    lines[line.id] = make_internal_line(line);
    update_graph_edges(lines.at(line.id));

    return true;
}

auto network_monitor::TransportNetwork::make_internal_line(const Line& line) const
    -> std::shared_ptr<LineInternal>
{
    auto internal_line = std::make_shared<LineInternal>(line.id, line.name);

    for (const auto& route : line.routes) {
        internal_line->routes[route.id] = make_internal_route(route, internal_line);
    }
    return internal_line;
}

auto network_monitor::TransportNetwork::make_internal_route(
    const Route& route, const std::shared_ptr<LineInternal>& line) const
    -> std::shared_ptr<RouteInternal>
{
    auto internal_route = std::make_shared<RouteInternal>();
    internal_route->id = route.id;
    internal_route->name = route.name;
    internal_route->line = line;

    for (const auto& stop : route.stops) {
        internal_route->stops.push_back(stations.at(stop));
    }
    return internal_route;
}

void network_monitor::TransportNetwork::update_graph_edges(
    const std::shared_ptr<LineInternal>& internal_line)
{
    for (const auto& [route_id, route] : internal_line->routes) {
        for (unsigned i = 0; i < route->stops.size() - 1; ++i) {
            auto& current_station = stations.at(route->stops.at(i)->station.id);
            auto graph_edge = GraphEdge();
            graph_edge.route = route;
            graph_edge.next_stop = route->stops.at(i + 1);
            current_station->edges.push_back(graph_edge);
        }
    }
}

bool network_monitor::TransportNetwork::record_passenger_event(
    const network_monitor::Id& station,
    const network_monitor::TransportNetwork::PassengerEvent& event)
{
    if (!stations.contains(station)) {
        return false;
    }

    if (event == PassengerEvent::In) {
        stations.at(station)->passenger_count++;
    } else if (event == PassengerEvent::Out) {
        stations.at(station)->passenger_count--;
    } else {
        return false;
    }

    return true;
}

long long network_monitor::TransportNetwork::get_passenger_count(
    const network_monitor::Id& station) const
{
    if (stations.contains(station)) {
        return stations.at(station)->passenger_count;
    }
    throw std::runtime_error("Station is not in the network.");
}

std::vector<network_monitor::Id>
network_monitor::TransportNetwork::get_routes_serving_station(
    const network_monitor::Id& station) const
{
    std::set<Id> routes;
    for (const auto& [line_id, line] : lines) {
        for (const auto& [route_id, route] : line->routes) {
            if (route_serves_station(*route, station)) {
                routes.emplace(route_id);
            }
        }
    }
    return {routes.begin(), routes.end()};
}

bool network_monitor::TransportNetwork::route_serves_station(const RouteInternal& route,
                                                             const Id& station)
{
    return std::find_if(route.stops.begin(), route.stops.end(),
                        [&station](const auto& graph_node) {
                            return graph_node->station.id == station;
                        })
           != route.stops.end();
}

bool network_monitor::TransportNetwork::set_travel_time(
    const network_monitor::Id& station_a,
    const network_monitor::Id& station_b,
    const unsigned int travel_time)
{
    if (!stations.contains(station_a) or !stations.contains(station_b)) {
        return false;
    }

    auto& station_a_edges = stations.at(station_a)->edges;
    auto station_a_graph_edge =
        std::find_if(station_a_edges.begin(), station_a_edges.end(), [&station_b](const auto& graph_edge) {
            return graph_edge.next_stop->station.id == station_b;
        });
    if (station_a_graph_edge != station_a_edges.end()) {
        station_a_graph_edge->travel_time = travel_time;
        return true;
    }

    auto& station_b_edges = stations.at(station_b)->edges;
    auto station_b_graph_edge =
        std::find_if(station_b_edges.begin(), station_b_edges.end(), [&station_a](const auto& graph_edge) {
          return graph_edge.next_stop->station.id == station_a;
        });
    if (station_b_graph_edge != station_b_edges.end()) {
        station_b_graph_edge->travel_time = travel_time;
        return true;
    }

    return false;
}

unsigned
network_monitor::TransportNetwork::get_travel_time(const network_monitor::Id& station_a,
                                                   const network_monitor::Id& station_b) const
{
    if (!stations.contains(station_a) or !stations.contains(station_b)) {
        return 0;
    }

    auto& station_a_edges = stations.at(station_a)->edges;
    auto station_a_graph_edge =
        std::find_if(station_a_edges.begin(), station_a_edges.end(), [&station_b](const auto& graph_edge) {
            return graph_edge.next_stop->station.id == station_b;
        });

    if (station_a_graph_edge != station_a_edges.end()) {
        return station_a_graph_edge->travel_time;
    }

    auto& station_b_edges = stations.at(station_b)->edges;
    auto station_b_graph_edge =
        std::find_if(station_b_edges.begin(), station_b_edges.end(), [&station_a](const auto& graph_edge) {
          return graph_edge.next_stop->station.id == station_a;
        });

    if (station_b_graph_edge != station_b_edges.end()) {
        return station_b_graph_edge->travel_time;
    }

    return 0;
}

unsigned
network_monitor::TransportNetwork::get_travel_time(const network_monitor::Id& line,
                                                   const network_monitor::Id& route,
                                                   const network_monitor::Id& station_a,
                                                   const network_monitor::Id& station_b) const
{
    return 0;
}
