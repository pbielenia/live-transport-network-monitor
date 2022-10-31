#include <network-monitor/transport-network.hpp>

using namespace NetworkMonitor;

bool Station::operator==(const Station& other) const
{
    // todo
    return false;
}

bool Route::operator==(const Route& other) const
{
    // todo
    return false;
}

bool Line::operator==(const Line& other) const
{
    // todo
    return false;
}

TransportNetwork::TransportNetwork()
{
    //
}

TransportNetwork::~TransportNetwork()
{
    //
}

TransportNetwork::TransportNetwork(const TransportNetwork& copied)
{
    //
}

TransportNetwork::TransportNetwork(TransportNetwork&& moved)
{
    //
}

TransportNetwork& TransportNetwork::operator=(const TransportNetwork& copied)
{
    // todo
    return *this;
}

TransportNetwork& TransportNetwork::operator=(TransportNetwork&& moved)
{
    // todo
    return *this;
}

bool TransportNetwork::AddStation(const Station& station)
{
    // todo
    return false;
}

bool TransportNetwork::AddLine(const Line& line)
{
    // todo
    return false;
}

bool TransportNetwork::RecordPassengerEvent(const PassengerEvent& event)
{
    // todo
    return false;
}

long long int TransportNetwork::GetPassengerCount(const Id& station) const
{
    // todo
    return 0;
}

std::vector<Id> TransportNetwork::GetRoutesServingStation(const Id& station) const
{
    // todo
    return {};
}

bool TransportNetwork::SetTravelTime(const Id& station_a,
                                     const Id& station_b,
                                     const unsigned int travel_time)
{
    // todo
    return false;
}

unsigned int TransportNetwork::GetTravelTime(const Id& station_a,
                                             const Id& station_b) const
{
    // todo
    return 0;
}

unsigned int TransportNetwork::GetTravelTime(const Id& line,
                                             const Id& route,
                                             const Id& station_a,
                                             const Id& station_b) const
{
    // todo
    return 0;
}
