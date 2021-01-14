#include <string>
#include <vector>

namespace network_monitor {

using Id = std::string;

struct Station {
    Id id{};
    std::string name{};

    bool operator==(const Station& other) const;
};

struct Route {
    Id id{};
    std::string name{};
    Id line_id{};
    Id start_station_id{};
    Id end_station_id{};
    std::vector<Id> stops{};

    bool operator==(const Route& other) const;
};

struct Line {
    Id id{};
    std::string name{};
    std::vector<Route> routes{};

    bool operator==(const Line& other) const;
};

class TransportNetwork {
public:
    enum class PassengerEvent {
        In,
        Out,
    };

    TransportNetwork();
    TransportNetwork(const TransportNetwork& other);
    TransportNetwork(TransportNetwork&& other) noexcept;
    ~TransportNetwork() = default;

    TransportNetwork& operator=(const TransportNetwork& other);
    TransportNetwork& operator=(TransportNetwork&& other) noexcept;

    bool add_station(const Station& station);
    bool add_line(const Line& line);
    bool record_passenger_event(const Id& station, const PassengerEvent& event);
    long long get_passenger_count(const Id& station) const;
    std::vector<Id> get_routes_serving_station(const Id& station) const;

    bool
    set_travel_time(const Id& station_a, const Id& station_b, const unsigned travel_time);

    unsigned get_travel_time(const Id& station_a, const Id& station_b);
    unsigned get_travel_time(const Id& line,
                             const Id& route,
                             const Id& station_a,
                             const Id& station_b);

private:
    bool station_is_in_network(const Station& station);
    bool station_is_in_network(const Id& id);
    bool line_is_in_network(const Line& line);
    bool all_line_stops_are_in_network(const Line& line);

    std::vector<Station> stations;
    std::vector<Line> lines;
};

} // namespace network_monitor
