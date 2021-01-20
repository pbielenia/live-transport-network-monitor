
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

bool network_monitor::TransportNetwork::record_passenger_event(
    const network_monitor::Id& station,
    const network_monitor::TransportNetwork::PassengerEvent& event)
{
    return false;
}

long long network_monitor::TransportNetwork::get_passenger_count(
    const network_monitor::Id& station) const
{
    return 0;
}

std::vector<network_monitor::Id>
network_monitor::TransportNetwork::get_routes_serving_station(
    const network_monitor::Id& station) const
{
    return std::vector<Id>();
}

bool network_monitor::TransportNetwork::set_travel_time(
    const network_monitor::Id& station_a,
    const network_monitor::Id& station_b,
    const unsigned int travel_time)
{
    return false;
}

unsigned
network_monitor::TransportNetwork::get_travel_time(const network_monitor::Id& station_a,
                                                   const network_monitor::Id& station_b)
{
    return 0;
}

unsigned
network_monitor::TransportNetwork::get_travel_time(const network_monitor::Id& line,
                                                   const network_monitor::Id& route,
                                                   const network_monitor::Id& station_a,
                                                   const network_monitor::Id& station_b)
{
    return 0;
}
