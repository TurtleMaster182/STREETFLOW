#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <map>
#include <random>
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
Graph Graph::read(std::istream& input) {
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
    g.automatic_layout();
    return g;
}
void Graph::automatic_layout() {
    // Seeded spiral initialization plus local repulsion and road springs. No square grid.
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> jitter(-0.3,0.3);
    for(size_t i=0;i<nodes.size();++i) {
        double angle=i*2.399963229728653+jitter(rng),radius=75*std::sqrt(i+1.0);
        nodes[i].x=radius*std::cos(angle);nodes[i].y=radius*std::sin(angle);
    }
    constexpr double cell=160;
    const int iterations=nodes.size()>2000?50:160;
    std::vector<std::pair<double,double>> forces(nodes.size());
    for(int step=0;step<iterations;++step) {
        std::map<std::pair<int,int>,std::vector<int>> buckets;
        for(size_t i=0;i<nodes.size();++i) buckets[{static_cast<int>(std::floor(nodes[i].x/cell)),static_cast<int>(std::floor(nodes[i].y/cell))}].push_back(static_cast<int>(i));
        std::fill(forces.begin(),forces.end(),std::pair<double,double>{0,0});
        for(size_t i=0;i<nodes.size();++i) {
            int x=static_cast<int>(std::floor(nodes[i].x/cell)),y=static_cast<int>(std::floor(nodes[i].y/cell));
            for(int dx=-1;dx<=1;++dx) for(int dy=-1;dy<=1;++dy) {
                auto found=buckets.find({x+dx,y+dy});if(found==buckets.end()) continue;
                for(int j:found->second) {
                    if(j<=static_cast<int>(i))continue;
                    double rx=nodes[i].x-nodes[j].x,ry=nodes[i].y-nodes[j].y;
                    double d=std::max(1.0,std::hypot(rx,ry));if(d>cell)continue;
                    double force=3000/(d*d);
                    forces[i].first+=rx*force;forces[i].second+=ry*force;
                    forces[j].first-=rx*force;forces[j].second-=ry*force;
                }
            }
        }
        for(const auto& lane:lanes) {
            if(lane.parallel_index)continue;
            double dx=nodes[lane.to].x-nodes[lane.from].x,dy=nodes[lane.to].y-nodes[lane.from].y;
            double d=std::max(1.0,std::hypot(dx,dy)),target=80+std::min(lane.length,60.0)*2;
            double force=(d-target)*0.025/d;
            forces[lane.from].first+=dx*force;forces[lane.from].second+=dy*force;
            forces[lane.to].first-=dx*force;forces[lane.to].second-=dy*force;
        }
        double limit=18*(1-static_cast<double>(step)/iterations)+0.5;
        for(size_t i=0;i<nodes.size();++i) {
            auto [fx,fy]=forces[i];fx-=nodes[i].x*0.002;fy-=nodes[i].y*0.002;
            double magnitude=std::max(1.0,std::hypot(fx,fy)),scale=std::min(1.0,limit/magnitude);
            nodes[i].x+=fx*scale;nodes[i].y+=fy*scale;
        }
    }
}
void Graph::read_layout(std::istream& input) {
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
    if (std::find(seen.begin(), seen.end(), false) != seen.end())
        throw std::runtime_error("Layout must provide a position for every city node");
    nodes = std::move(positions);
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
