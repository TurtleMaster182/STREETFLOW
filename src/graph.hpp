#pragma once
#include <istream>
#include <string>
#include <unordered_map>
#include <vector>

namespace sf {
struct Node {
    int id;
    double x = 0, y = 0;
    std::vector<int> outgoing, incoming, neighbours, signal_sources;
};
struct Lane {
    int from, to;
    bool light;
    double length;
    int parallel_index = 0, parallel_count = 1;
    std::vector<int> alternatives;
};
struct Graph {
    std::vector<Node> nodes;
    std::vector<Lane> lanes;
    std::vector<int> origins;
    std::unordered_map<int, int> node_index;
    static Graph read(std::istream& input);
    void automatic_layout();
    void read_layout(std::istream& input);
    std::string json() const;
};
}
