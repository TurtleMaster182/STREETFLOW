#include "simulation.hpp"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <queue>
#include <sstream>
#include <stdexcept>

namespace sf {
Simulation::Simulation(const Graph& g, Config c) : graph(g), config(c), random(c.seed) {
    if (!config.max_cars || !std::isfinite(c.spawn_rate) || c.spawn_rate < 0 ||
        !std::isfinite(c.car_speed) || c.car_speed <= 0 || !std::isfinite(c.green) || c.green <= 0 ||
        !std::isfinite(c.yellow) || c.yellow < 0 || !std::isfinite(c.all_red) || c.all_red < 0 ||
        !std::isfinite(c.max_wait) || c.max_wait < 0)
        throw std::runtime_error("Invalid simulation configuration");
    reset();
}
void Simulation::reset() {
    time = 0; stats = {}; active_count = 0; next_id = 1; ticks = 0;
    cars.clear(); free_slots.clear(); trees.clear(); traffic.reset(); random.seed(config.seed);
    queues.assign(graph.lanes.size(), {});
    junction_cursor.assign(graph.nodes.size(), 0);
    next_crossing.assign(graph.nodes.size(), 0);
}
int Simulation::available_lane(int representative) const {
    int best = -1; size_t count = std::numeric_limits<size_t>::max();
    for (int id : graph.lanes[representative].alternatives) {
        const auto& q = queues[id];
        if ((q.empty() || cars[q.back()].position >= spacing) && q.size() < count) {
            best = id; count = q.size();
        }
    }
    return best;
}
bool Simulation::add_trip(const std::vector<int>& route) {
    if(route.empty()) return false;
    for (size_t i=0; i<route.size(); ++i) {
        if (route[i]<0 || static_cast<size_t>(route[i])>=graph.lanes.size()) return false;
        if (i && graph.lanes[route[i-1]].to != graph.lanes[route[i]].from) return false;
    }
    if (active_count >= config.max_cars) { ++stats.rejected; return false; }
    int lane = available_lane(route[0]);
    if(lane < 0) { ++stats.rejected; return false; }
    size_t slot;
    if(free_slots.empty()) { slot = cars.size(); cars.emplace_back(); }
    else { slot = free_slots.back(); free_slots.pop_back(); }
    Car c; c.id = next_id++; c.lane = lane; c.route = route; c.born = time; c.active = true;
    cars[slot] = std::move(c); queues[lane].push_back(slot);
    ++active_count; ++stats.spawned;
    if(!spawning_batch) { traffic.reset(); trees.clear(); }
    return true;
}
const TravelForecast& Simulation::forecast() const {
    if(!traffic) { traffic.emplace(*this); trees.clear(); }
    return *traffic;
}
const RouteTree& Simulation::routing_tree(int source) {
    const auto& observation=forecast();
    for(const auto& tree:trees) if(tree.source==source) return tree;
    if(trees.size()>=32) trees.pop_front();
    trees.push_back(observation.tree(source,!config.time_routing));
    return trees.back();
}
bool Simulation::spawn() {
    if(active_count>=config.max_cars) { ++stats.rejected; return false; }
    std::uniform_int_distribution<size_t> origin(0, graph.origins.size()-1);
    const auto& tree=routing_tree(graph.origins[origin(random)]);
    if(tree.reachable.empty()) { ++stats.rejected; return false; }
    std::uniform_int_distribution<size_t> destination(0, tree.reachable.size()-1);
    int node=tree.reachable[destination(random)]; std::vector<int> route;
    while(node!=tree.source) {
        int lane=tree.predecessor[node]; route.push_back(lane); node=graph.lanes[lane].from;
    }
    std::reverse(route.begin(), route.end()); return add_trip(route);
}
int Simulation::signal(int lane_id) const {
    const auto& l=graph.lanes[lane_id]; if(!l.light) return 1;
    const auto& sources=graph.nodes[l.to].signal_sources;
    double phase_length=config.green+config.yellow+config.all_red;
    double cycle=std::fmod(time,phase_length*sources.size());
    size_t phase=static_cast<size_t>(cycle/phase_length);
    if(sources[phase]!=l.from) return 0;
    double elapsed=cycle-phase*phase_length;
    return elapsed<config.green ? 1 : elapsed<config.green+config.yellow ? 2 : 0;
}
double Simulation::signal_remaining(int lane_id) const {
    const auto& l=graph.lanes[lane_id]; if(!l.light) return 0;
    const auto& sources=graph.nodes[l.to].signal_sources;
    double phase_length=config.green+config.yellow+config.all_red;
    double cycle_length=phase_length*sources.size();
    double cycle=std::fmod(time,cycle_length);
    double start=std::distance(sources.begin(),std::find(sources.begin(),sources.end(),l.from))*phase_length;
    if (cycle>=start && cycle<start+config.green) return start+config.green-cycle;
    if (cycle>=start+config.green && cycle<start+config.green+config.yellow) return start+config.green+config.yellow-cycle;
    return std::fmod(start-cycle+cycle_length,cycle_length);
}
void Simulation::finish_trip(size_t slot, bool completed) {
    auto& c=cars[slot];
    c.active=false; --active_count;
    if(completed) {
        ++stats.completed;
        stats.total_trip_time+=time+dt-c.born; stats.completed_wait+=c.waiting;
    } else ++stats.abandoned;
    c.route.clear(); free_slots.push_back(slot);
}
void Simulation::tick() {
    std::poisson_distribution<int> arrivals(config.spawn_rate*dt);
    spawning_batch=true;
    int count=arrivals(random); for(int i=0;i<count;++i) spawn();
    spawning_batch=false;
    // Move each lane front-to-back. Every vehicle advances at most speed * dt.
    for(size_t lane=0;lane<queues.size();++lane) {
        double limit=graph.lanes[lane].length;
        for(size_t slot : queues[lane]) {
            auto& c=cars[slot]; double before=c.position;
            c.position=std::max(before,std::min(before+config.car_speed*dt,limit));
            double moved=c.position-before; c.velocity=moved/dt; stats.distance+=moved;
            double delay=std::clamp(dt-moved/config.car_speed,0.0,dt);
            c.waiting+=delay; stats.total_wait+=delay;
            if(config.max_wait>0 && c.waiting+1e-9>=config.max_wait) c.abandoning=true;
            limit=c.position-spacing;
        }
    }
    // Drivers who give up still reach the downstream node in queue order.
    // Leaving the network uses no junction crossing or downstream lane space.
    for(size_t lane=0;lane<queues.size();++lane) {
        auto& q=queues[lane];
        if(q.empty()) continue;
        size_t slot=q.front(); const auto& c=cars[slot];
        if(c.abandoning && c.position+1e-9>=graph.lanes[lane].length) {
            q.pop_front(); finish_trip(slot,false);
        }
    }
    // At most one crossing per junction per headway. Rotating priority prevents starvation.
    for(size_t node=0;node<graph.nodes.size();++node) {
        const auto& incoming=graph.nodes[node].incoming;
        if(incoming.empty() || time+1e-9<next_crossing[node]) continue;
        for(size_t attempt=0;attempt<incoming.size();++attempt) {
            size_t choice=(junction_cursor[node]+attempt)%incoming.size();
            int lane=incoming[choice]; auto& q=queues[lane];
            if(q.empty() || signal(lane)!=1) continue;
            size_t slot=q.front(); auto& c=cars[slot];
            if(c.position+1e-9<graph.lanes[lane].length) continue;
            int next=-1;
            if(c.route_index+1<c.route.size()) {
                next=available_lane(c.route[c.route_index+1]); if(next<0) continue;
            }
            q.pop_front(); junction_cursor[node]=(choice+1)%incoming.size();
            next_crossing[node]=time+std::max(dt,spacing/config.car_speed);
            if(next<0) {
                finish_trip(slot,true);
            } else {
                c.lane=next; ++c.route_index; c.position=0; queues[next].push_back(slot);
            }
            break;
        }
    }
    time=static_cast<double>(++ticks)*dt;
    traffic.reset(); trees.clear();
}
std::string Simulation::summary() const {
    size_t stopped=0; for(const auto& c:cars) if(c.active && c.velocity<0.05) ++stopped;
    std::ostringstream out; out << std::fixed << std::setprecision(3)
        << "{\"time\":" << time << ",\"active\":" << active_count << ",\"stopped\":" << stopped
        << ",\"spawned\":" << stats.spawned << ",\"completed\":" << stats.completed
        << ",\"abandoned\":" << stats.abandoned
        << ",\"rejected\":" << stats.rejected << ",\"avgTrip\":" << (stats.completed?stats.total_trip_time/stats.completed:0)
        << ",\"avgWait\":" << (stats.completed?stats.completed_wait/stats.completed:0)
        << ",\"totalWait\":" << stats.total_wait << ",\"distance\":" << stats.distance << '}';
    return out.str();
}
std::string Simulation::snapshot(bool paused,double time_scale,size_t render_limit) const {
    const auto& prediction=forecast();
    std::ostringstream out; out << std::fixed << std::setprecision(3)
        << "{\"stats\":" << summary() << ",\"paused\":" << (paused?"true":"false")
        << ",\"timeScale\":" << time_scale << ",\"spawnRate\":" << config.spawn_rate
        << ",\"maxWait\":" << config.max_wait
        << ",\"seed\":" << config.seed << ",\"maxCars\":" << config.max_cars
        << ",\"routing\":\"" << (config.time_routing?"time":"distance") << "\""
        << ",\"speed\":" << config.car_speed << ",\"renderLimit\":" << render_limit << ",\"cars\":[";
    size_t emitted=0;
    for(const auto& c:cars) {
        if(!c.active) continue;
        if(emitted>=render_limit) break;
        if(emitted++) out << ',';
        out << '[' << c.id << ',' << c.lane << ',' << c.position/graph.lanes[c.lane].length << ',' << c.velocity << ']';
    }
    out << "],\"lanes\":[";
    for(size_t i=0;i<queues.size();++i) {
        if(i) out << ',';
        out << '[' << queues[i].size() << ',' << signal(static_cast<int>(i)) << ',' << signal_remaining(static_cast<int>(i)) << ',' << prediction.roads[i].seconds
            << ',' << prediction.roads[i].congestion << ',' << prediction.roads[i].occupancy
            << ',' << prediction.roads[i].traffic_delay << ',' << prediction.roads[i].signal_delay << ']';
    }
    out << "],\"route\":";
    if(selected_route) out << route_comparison(selected_route->first,selected_route->second);
    else out << "null";
    return out.str()+"}";
}
std::string Simulation::route_comparison(int from,int to) const {
    if(!graph.node_index.contains(from) || !graph.node_index.contains(to))
        return R"({"error":"Unknown start or destination node"})";
    const auto& prediction=forecast();
    int source=graph.node_index.at(from),target=graph.node_index.at(to);
    auto fastest=prediction.route(source,target),shortest=prediction.route(source,target,true);
    std::ostringstream out;out<<std::fixed<<std::setprecision(3);
    out<<"{\"from\":"<<from<<",\"to\":"<<to<<",\"at\":"<<time;
    auto emit=[&](const char* name,const Route& route) {
        out<<",\""<<name<<"\":{\"reachable\":"<<(route.reachable?"true":"false")
           <<",\"seconds\":"<<route.seconds<<",\"distance\":"<<route.distance<<",\"lanes\":[";
        for(size_t i=0;i<route.lanes.size();++i) {if(i)out<<',';out<<route.lanes[i];}
        out<<"],\"nodes\":[";
        if(route.reachable) {
            out<<from;for(int lane:route.lanes)out<<','<<graph.nodes[graph.lanes[lane].to].id;
        }
        out<<"]}";
    };
    emit("fastest",fastest);emit("shortest",shortest);
    out<<",\"saving\":"<<std::max(0.0,shortest.seconds-fastest.seconds)<<'}';
    return out.str();
}

}
