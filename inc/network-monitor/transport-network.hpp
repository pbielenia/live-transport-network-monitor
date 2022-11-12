#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace NetworkMonitor {

/*! \brief A station, line, or route ID.
 */
using Id = std::string;

/*! \brief Network station
 *
 *  A Station struct is well formed if:
 *  - `id` is unique across all stations in the network.
 */
struct Station {
    Id id{};
    std::string name{};

    /*! \brief Station comparison
     *
     *  Two stations are "equal" if they have the same ID.
     */
    bool operator==(const Station& other) const;
};

/*! \brief Network route
 *
 *  Each underground line has one or more routes. A route represents a single
 *  possible journey across a set of stops in a specified direction.
 *
 *  There may or may not be a corresponding route in the opposite direction of
 *  travel.
 *
 *  A Route struct is well formed if:
 *  - `id` is unique across all lines and their routes in the network.
 *  - The `line_id` line exists and has this route among its routes.
 *  - `stops` has at least 2 stops.
 *  - `start_station_id` is the first stop in `stops`.
 *  - `end_station_id` is the last stop in `stops`.
 *  - Every `station_id` station in `stops` exists.
 *  - Every stop in `stops` appears only once.
 */
struct Route {
    Id id{};
    std::string direction{};
    Id line_id{};
    Id start_station_id{};
    Id end_station_id{};
    std::vector<Id> stops{};

    /*! \brief Route comparison
     *
     *  Two routes are "equal" if they have the same ID.
     */
    bool operator==(const Route& other) const;
};

/*! \brief Network line
 *
 *  A line is a collection of routes serving multiple stations.
 *
 *  A Line struct is well formed if:
 *  - `id` is unique across all lines in the network.
 *  - `routes` has at least 1 route.
 *  - Every route in `routes` is well formed.
 *  - Every route in `routes` has a `line_id` that is equal to this line `id`.
 */
struct Line {
    Id id{};
    std::string name{};
    std::vector<Route> routes{};

    /*! \brief Line comparison
     *
     *  Two lines are "equal" if they have the same ID.
     */
    bool operator==(const Line& other) const;
};

/*! \brief Passenger event
 */
struct PassengerEvent {
    enum class Type { In, Out };

    Id station_id{};
    Type type{Type::In};
};

/*! \brief Underground network representation
 */
class TransportNetwork {
public:
    /*! \brief Default constructor
     */
    TransportNetwork();

    /*! \brief Destructor
     */
    ~TransportNetwork();

    /*! \brief Copy constructor
     */
    TransportNetwork(const TransportNetwork& copied);

    /*! \brief Move constructor
     */
    TransportNetwork(TransportNetwork&& moved);

    /*! \brief Copy assignment operator
     */
    TransportNetwork& operator=(const TransportNetwork& copied) = default;

    /*! \brief Move assignment operator
     */
    TransportNetwork& operator=(TransportNetwork&& moved) = default;

    /*! \brief Add a station to the network.
     *
     *  \returns false if there was an error while adding the station to the
     *           network.
     *
     *  This function assumes that the Station object is well-formed.
     *
     *  The station cannot already be in the network.
     */
    bool AddStation(const Station& station);

    /*! \brief Add a line to the network.
     *
     *  \returns false if there was an error while adding the station to the
     *           network.
     *
     *  This function assumes that the Line object is well-formed.
     *
     *  All stations served by this line must already be in the network. The
     *  line cannot already be in the network.
     */
    bool AddLine(const Line& line);

    /*! \brief Record a passenger event at a station.
     *
     *  \returns false if the station is not in the network or if the passenger
     *           event is not reconized.
     */
    bool RecordPassengerEvent(const PassengerEvent& event);

    /*! \brief Get the number of passengers currently recorded at a station.
     *
     *  The returned number can be negative: This happens if we start recording
     *  in the middle of the day and we record more exiting than entering
     *  passengers.
     *
     *  \throws std::runtime_error if the station is not in the network.
     */
    long long int GetPassengerCount(const Id& station) const;

    /*! \brief Get list of routes serving a given station.
     *
     *  \returns An empty vector if there was an error getting the list of
     *           routes serving the station, or if the station has legitimately
     *           no routes serving it.
     *
     *  The station must already be in the network.
     */
    std::vector<Id> GetRoutesServingStation(const Id& station) const;

    /*! \brief Set the travel time between 2 adjacent stations.
     *
     *  \returns false if there was an error while setting the travel time
     *           between the two stations.
     *
     *  The travel time is the same for all routes connecting the two stations
     *  directly.
     *
     *  The two stations must be adjacent in at least one line route. The two
     *  stations must already be in the network.
     */
    bool SetTravelTime(const Id& station_a,
                       const Id& station_b,
                       const unsigned int travel_time);

    /*! \brief Get the travel time between 2 adjacent stations.
     *
     *  \returns 0 if the function could not find the travel time between the
     *           two stations, or if station A and B are the same station.
     *
     *  The travel time is the same for all routes connecting the two stations
     *  directly.
     *
     *  The two stations must be adjacent in at least one line route. The two
     *  stations must already be in the network.
     */
    unsigned int GetTravelTime(const Id& station_a, const Id& station_b) const;

    /*! \brief Get the total travel time between any 2 stations, on a specific
     *         route.
     *
     *  The total travel time is the cumulative sum of the travel times between
     *  all stations between `station_a` and `station_b`.
     *
     *  \returns 0 if the function could not find the travel time between the
     *           two stations, or if station A and B are the same station.
     *
     *  The two stations must be both served by the `route`. The two stations
     *  must already be in the network.
     */
    unsigned int GetTravelTime(const Id& line,
                               const Id& route,
                               const Id& station_a,
                               const Id& station_b) const;

private:
    struct GraphNode;
    struct GraphEdge;
    struct RouteInternal;
    struct LineInternal;

    struct GraphNode {
        GraphNode(const Station& station);

        Id id;
        std::string name;
        long long int passenger_count{0};
        std::vector<std::shared_ptr<GraphEdge>> edges{};

        // FIXME: Possibly could be successfully turned into FindAnyEdgeToNextStation to
        //        return only the first found match.
        std::vector<std::shared_ptr<GraphEdge>>
        FindEdgesToNextStation(const Id& next_station) const;
        std::vector<std::shared_ptr<GraphEdge>>
        FindEdgesForRoute(const std::shared_ptr<RouteInternal>& route) const;
        // std::vector<std::shared_ptr<GraphEdge>>
        // FindEdges(std::function<bool(const std::shared_ptr<GraphEdge>&)>) const;
    };

    struct GraphEdge {
        GraphEdge(const std::shared_ptr<RouteInternal>& route,
                  const std::shared_ptr<GraphNode>& next_station);

        std::shared_ptr<RouteInternal> route;
        std::shared_ptr<GraphNode> next_station;
        unsigned int travel_time{0};
    };

    struct RouteInternal {
        RouteInternal(const Id& id, const std::shared_ptr<LineInternal>& line);

        Id id;
        std::shared_ptr<LineInternal> line;
        std::vector<std::shared_ptr<GraphNode>> stations{};
    };

    struct LineInternal {
        LineInternal(const Id& id, const std::string& name);

        Id id;
        std::string name;
        std::vector<std::shared_ptr<RouteInternal>> routes{};
    };

    void AddStationInternal(const Station& station);
    std::shared_ptr<LineInternal> CreateLineInternal(const Id& id,
                                                     const std::string& name);
    bool StationExists(const Id& station_id) const;
    bool StationsExist(const std::vector<Route>& routes) const;
    bool StationsExist(const std::vector<Id>& stations) const;
    bool LineExists(const Line& line) const;
    bool StationsAreAdjacend(const Id& station_a, const Id& station_b) const;
    bool StationConnectsAnother(const Id& station_a, const Id& station_b) const;

    std::map<Id, std::shared_ptr<GraphNode>> stations_{};
    std::map<Id, std::shared_ptr<LineInternal>> lines_{};
};

} // namespace NetworkMonitor
