#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>

namespace sf {
namespace {
std::string content(std::string line) { return line.substr(0, line.find('#')); }
void unique(std::vector<int>& values) {
    std::sort(values.begin(), values.end());
    values.erase(std::unique(values.begin(), values.end()), values.end());
}
}
Graph Graph::read(std::istream& input, bool arrange) {
    Graph g;
    std::string line;
    size_t line_no = 0;
    auto node = [&](int id) {
        auto [it, inserted] = g.node_index.emplace(id, static_cast<int>(g.nodes.size()));
        if (inserted) { Node n; n.id = id; g.nodes.push_back(n); }
        return it->second;
    };
    std::map<std::pair<int,int>, std::vector<int>> groups;
    while (std::getline(input, line)) {
        ++line_no;
        std::istringstream row(content(line));
        row >> std::ws;
        if (row.eof()) continue;
        int a, b, c; double d; std::string extra;
        if (!(row >> a >> b >> c >> d) || (row >> extra) || a < 0 || b < 0 ||
            a == b || (c != 0 && c != 1) || !std::isfinite(d) || d <= 0) {
            throw std::runtime_error("CITY.IN line " + std::to_string(line_no) +
                ": expected a b c d (distinct nonnegative integer nodes, light 0/1, positive length)");
        }
        int from = node(a), to = node(b), index = static_cast<int>(g.lanes.size());
        Lane lane; lane.from = from; lane.to = to; lane.light = c; lane.length = d;
        g.lanes.push_back(lane);
        g.nodes[from].outgoing.push_back(index);
        g.nodes[to].incoming.push_back(index);
        g.nodes[from].neighbours.push_back(to);
        g.nodes[to].neighbours.push_back(from);
        if (c) g.nodes[to].signal_sources.push_back(from);
        groups[{from, to}].push_back(index);
    }
    if (g.lanes.empty()) throw std::runtime_error("CITY.IN contains no roads");
    for (const auto& [key, group] : groups) {
        (void)key;
        for (size_t i = 0; i < group.size(); ++i) {
            auto& lane = g.lanes[group[i]];
            lane.parallel_index = static_cast<int>(i);
            lane.parallel_count = static_cast<int>(group.size());
            lane.alternatives = group;
        }
    }
    std::vector<int> order;
    for (size_t i=0; i<g.nodes.size(); ++i) order.push_back(static_cast<int>(i));
    std::sort(order.begin(), order.end(), [&](int a, int b){ return g.nodes[a].id < g.nodes[b].id; });
    for (size_t p = 0; p < order.size(); ++p) {
        auto& n = g.nodes[order[p]];
        unique(n.neighbours); unique(n.signal_sources);
        if (!n.outgoing.empty()) g.origins.push_back(order[p]);
    }
    if(arrange) g.automatic_layout();
    return g;
}
void Graph::read_layout(std::istream& input, bool fixed) {
    std::string line; size_t line_no = 0;
    std::vector<bool> seen(nodes.size(), false);
    auto positions = nodes;
    while (std::getline(input, line)) {
        ++line_no;
        std::istringstream row(content(line)); row >> std::ws;
        if (row.eof()) continue;
        int id; double x, y; std::string extra;
        if (!(row >> id >> x >> y) || (row >> extra) || !node_index.contains(id) ||
            !std::isfinite(x) || !std::isfinite(y) || std::abs(x)>1e7 || std::abs(y)>1e7)
            throw std::runtime_error("Layout line " + std::to_string(line_no) + ": expected existing_node x y");
        int idx = node_index.at(id);
        if (seen[idx]) throw std::runtime_error("Duplicate node in layout: " + std::to_string(id));
        seen[idx] = true; positions[idx].x = x; positions[idx].y = y;
    }
    nodes = std::move(positions);
    if(!fixed) automatic_layout({},true);
    else if (std::find(seen.begin(), seen.end(), false) != seen.end()) automatic_layout(seen);
}
std::string Graph::json() const {
    std::ostringstream out; out << std::setprecision(10) << "{\"nodes\":[";
    for (size_t i=0; i<nodes.size(); ++i) {
        const auto& n = nodes[i]; if(i) out << ',';
        out << "{\"id\":" << n.id << ",\"x\":" << n.x << ",\"y\":" << n.y
            << ",\"degree\":" << n.neighbours.size() << ",\"signal\":" << (!n.signal_sources.empty() ? "true":"false") << '}';
    }
    out << "],\"lanes\":[";
    for (size_t i=0; i<lanes.size(); ++i) {
        const auto& l = lanes[i]; if(i) out << ',';
        out << "{\"id\":" << i << ",\"from\":" << l.from << ",\"to\":" << l.to
            << ",\"length\":" << l.length << ",\"light\":" << (l.light?"true":"false")
            << ",\"index\":" << l.parallel_index << ",\"count\":" << l.parallel_count << '}';
    }
    return out.str() + "]}";
}
}
