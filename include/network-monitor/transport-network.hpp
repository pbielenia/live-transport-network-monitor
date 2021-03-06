#include <memory>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>
#include <map>

namespace network_monitor {

using Id = std::string;

/*! \brief Network station
 *
 *  A Station struct is well formed if:
 *  - `id` is unique across all stations in the network.
 */
struct Station {
    Id id{};
    std::string name{};

    /*! \brief
     *
     *  Two stations are "equal" if they have same ID.
     */
    bool operator==(const Station& other) const;
};

/*! \brief Network route
 *
 *  Each underground line has one or more routes. A route represents a single possible
 *  journey across a set of stops in a specified direction.
 *
 *  There may or may not be a corresponding route in the opposite direction of travel.
 *
 *  A route struct is well formed if:
 *  - `id` is unique across all lines and their routes in the network.
 *  - The `line_id` line exists and has this route among its routes.
 *  - `stops` has at least 2 stops.
 *  - `start_station_id` is the first stop in `stops`.
 *  - `end_station_id` is the last stop in `stops`.
 *  - Every `station_id` station in `stops` exists.
 *  - Every stop in `stops` appears only one.
 */
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

    /*! \brief Populate the network from a JSON object.
     *
     *  \param source Ownership of the source JSON object is moved to this method.
     *
     *  \return false if stations and lines where parsed successfully, but not the travel
     *          times
     *
     *  \throws std::runtime_error This method throws if the JSON items were parsed
     *                             correctly but there was an issue adding new stations
     *                             or lines to the network.
     *  \throws nlohmann::json::exception If there was a problem parsing the JSON object.
     */
    bool from_json(nlohmann::json&& source);

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

        std::optional<std::vector<GraphEdge>::const_iterator>
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
