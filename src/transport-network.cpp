#include <algorithm>
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
bool network_monitor::TransportNetwork::add_station(
    const network_monitor::Station& station)
{
    if (station_is_in_network(station)) {
        return false;
    }
    stations.push_back(station);
    return true;
}

bool network_monitor::TransportNetwork::station_is_in_network(const Station& station)
{
    return std::find(stations.begin(), stations.end(), station) != stations.end();
}

bool network_monitor::TransportNetwork::station_is_in_network(const Id& id)
{
    return std::find_if(stations.begin(), stations.end(),
                     [&id](const auto& station) { return station.id.compare(id) == 0U; })
           != stations.end();
}

bool network_monitor::TransportNetwork::add_line(const network_monitor::Line& line)
{
    if (line_is_in_network(line) or not all_line_stops_are_in_network(line)) {
        return false;
    }
    lines.push_back(line);
    return true;
}

bool network_monitor::TransportNetwork::line_is_in_network(const Line& line)
{
    return std::find(lines.begin(), lines.end(), line) != lines.end();
}

bool network_monitor::TransportNetwork::all_line_stops_are_in_network(const Line& line)
{
    for (const auto& route : line.routes) {
        if (!station_is_in_network(route.start_station_id)
            or !station_is_in_network(route.end_station_id)) {
            return false;
        }

        for (const auto& stop : route.stops) {
            if (!station_is_in_network(stop)) {
                return false;
            }
        }
    }

    return true;
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
