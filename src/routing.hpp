#pragma once
#include "graph.hpp"
#include <vector>

namespace sf {
class Simulation;
struct RouteTree {
    int source = -1;
    std::vector<int> predecessor, reachable;
};
struct Route {
    bool reachable = false;
    std::vector<int> lanes;
    double seconds = 0, distance = 0;
};
struct RoadForecast {
    double clears_at = 0;
    double entry_at = 0;
    double occupancy = 0;
    double seconds = 0, traffic_delay = 0, signal_delay = 0, congestion = 0;
};

// One immutable traffic observation. Its FIFO exit functions support earliest-arrival Dijkstra.
class TravelForecast {
public:
    explicit TravelForecast(const Simulation& sim);
    double next_green(int lane, double at) const;
    double exit_time(int lane, double entry) const;
    RouteTree tree(int source, bool by_distance = false) const;
    Route route(int source, int destination, bool by_distance = false) const;
    std::vector<RoadForecast> roads;
    double observed_at;
private:
    const Graph& graph;
    double speed, green, yellow, all_red;
};
}
