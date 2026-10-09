#pragma once
#include "graph.hpp"
#include "routing.hpp"
#include <cstdint>
#include <deque>
#include <random>
#include <optional>
#include <string>
#include <vector>

namespace sf {
struct Config {
    uint32_t seed = 42;
    size_t max_cars = 5000;
    double spawn_rate = 3, car_speed = 2, green = 8, yellow = 2, all_red = 1;
    double max_wait = 60; // Accumulated delay before leaving at the next node; 0 disables.
    bool time_routing = true;
};
struct Car {
    uint64_t id = 0;
    int lane = -1;
    double position = 0, velocity = 0, born = 0, waiting = 0;
    size_t route_index = 0;
    std::vector<int> route;
    bool active = false, abandoning = false;
};
struct Stats {
    uint64_t spawned = 0, completed = 0, abandoned = 0, rejected = 0;
    double total_trip_time = 0, completed_wait = 0, total_wait = 0, distance = 0;
};
class Simulation {
public:
    static constexpr double dt = 0.05;
    static constexpr double spacing = 0.85;
    explicit Simulation(const Graph& graph, Config config = {});
    void tick();
    void reset();
    bool spawn();
    // Explicit trips support reproducible scenarios and core checks.
    bool add_trip(const std::vector<int>& route);
    int signal(int lane) const; // 0 red, 1 green, 2 amber
    double signal_remaining(int lane) const;
    std::string snapshot(bool paused, double time_scale, size_t render_limit = 4000) const;
    std::string summary() const;
    const TravelForecast& forecast() const;
    std::string route_comparison(int from, int to) const;
    std::optional<std::pair<int,int>> selected_route;
    const Graph& graph;
    Config config;
    double time = 0;
    Stats stats;
    std::vector<Car> cars;
    std::vector<std::deque<size_t>> queues;
    std::vector<uint64_t> lane_crashouts;
    size_t active_count = 0;
private:
    mutable std::optional<TravelForecast> traffic;
    mutable std::deque<RouteTree> trees;
    bool spawning_batch = false;
    std::vector<size_t> free_slots, junction_cursor;
    std::vector<double> next_crossing;
    std::mt19937 random;
    uint64_t next_id = 1;
    uint64_t ticks = 0;
    void finish_trip(size_t slot, bool completed);
    int available_lane(int representative) const;
    const RouteTree& routing_tree(int source);
};
}
