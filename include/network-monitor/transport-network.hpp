#include <memory>
#include <string>
#include <unordered_map>
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

    unsigned get_travel_time(const Id& station_a, const Id& station_b) const;
    unsigned get_travel_time(const Id& line,
                             const Id& route,
                             const Id& station_a,
                             const Id& station_b) const;

private:
    struct GraphNode;
    struct GraphEdge;
    struct RouteInternal;
    struct LineInternal;

    struct GraphNode {
        GraphNode(Station station);

        Station station{};
        long long passenger_count{0};
        std::vector<GraphEdge> edges{};

        std::vector<GraphEdge>::const_iterator
        find_edge_for_route(const std::shared_ptr<const RouteInternal>& route) const;
    };

    struct GraphEdge {
        std::shared_ptr<RouteInternal> route{nullptr};
        std::shared_ptr<GraphNode> next_stop{nullptr};
        unsigned travel_time{0};
    };

    struct RouteInternal {
        Id id{};
        std::string name{};
        std::shared_ptr<LineInternal> line{nullptr};
        std::vector<std::shared_ptr<GraphNode>> stops{};
    };

    struct LineInternal {
        LineInternal(Id id, std::string name);

        Id id{};
        std::string name{};
        std::unordered_map<Id, std::shared_ptr<RouteInternal>> routes{};
    };

    std::shared_ptr<LineInternal> make_internal_line(const Line& line) const;

    std::shared_ptr<RouteInternal>
    make_internal_route(const Route& route,
                        const std::shared_ptr<LineInternal>& line) const;

    void update_graph_edges(const std::shared_ptr<LineInternal>& internal_line);
    static bool route_serves_station(const RouteInternal& route, const Id& station);

    std::unordered_map<Id, std::shared_ptr<GraphNode>> stations{};
    std::unordered_map<Id, std::shared_ptr<LineInternal>> lines{};
};

} // namespace network_monitor
