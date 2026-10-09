#include "routing.hpp"
#include "simulation.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

namespace sf {
TravelForecast::TravelForecast(const Simulation& sim)
    : observed_at(sim.time), graph(sim.graph), speed(sim.config.car_speed),
      green(sim.config.green), yellow(sim.config.yellow), all_red(sim.config.all_red) {
    roads.resize(graph.lanes.size());
    const double headway=std::ceil(std::max(Simulation::dt,Simulation::spacing/speed)/Simulation::dt)*Simulation::dt;
    for(size_t id=0;id<graph.lanes.size();++id) {
        const auto& lane=graph.lanes[id];
        if(lane.parallel_index) continue;
        RoadForecast forecast;
        std::vector<double> ready;
        double capacity=0, first_entry=std::numeric_limits<double>::infinity();
        for(int parallel:lane.alternatives) {
            const auto& q=sim.queues[parallel];
            capacity+=std::floor(graph.lanes[parallel].length/Simulation::spacing)+1;
            double opening=observed_at;
            if(!q.empty() && sim.cars[q.back()].position<Simulation::spacing) {
                const auto& back=sim.cars[q.back()];
                opening+=(Simulation::spacing-back.position)/speed;
                if(back.velocity<0.05) {
                    const auto& front=sim.cars[q.front()];
                    opening=std::max(opening,next_green(parallel,observed_at+(lane.length-front.position)/speed)+headway);
                }
            }
            first_entry=std::min(first_entry,opening);
            for(size_t slot:q) ready.push_back(observed_at+std::max(0.0,lane.length-sim.cars[slot].position)/speed);
        }
        forecast.occupancy=std::clamp(ready.size()/capacity,0.0,1.0);
        forecast.entry_at=first_entry;
        std::sort(ready.begin(),ready.end());
        // Parallel lanes share the simulator's one-vehicle-at-a-time junction service.
        // Existing vehicles clear in readiness order, only during their approach's green.
        double release=observed_at;
        for(double arrival:ready) release=next_green(static_cast<int>(id),std::max(arrival,release))+headway;
        forecast.clears_at=release;
        for(int parallel:lane.alternatives) roads[parallel]=forecast;
    }
    for(size_t id=0;id<graph.lanes.size();++id) {
        auto& road=roads[id];
        const double free=graph.lanes[id].length/speed;
        const double empty_exit=next_green(static_cast<int>(id),observed_at+free);
        road.seconds=exit_time(static_cast<int>(id),observed_at)-observed_at;
        road.signal_delay=std::max(0.0,empty_exit-observed_at-free);
        road.traffic_delay=std::max(0.0,road.seconds-free-road.signal_delay);
        road.congestion=road.traffic_delay/(free+road.traffic_delay);
    }
}
double TravelForecast::next_green(int id,double at) const {
    const auto& lane=graph.lanes[id];
    if(!lane.light) return at;
    const auto& sources=graph.nodes[lane.to].signal_sources;
    const double phase=green+yellow+all_red,cycle=phase*sources.size();
    const double start=std::distance(sources.begin(),std::find(sources.begin(),sources.end(),lane.from))*phase;
    double offset=std::fmod(at,cycle);
    if(offset<0) offset+=cycle;
    if(offset<start) return at+start-offset;
    if(offset<start+green) return at;
    return at+cycle-offset+start;
}
double TravelForecast::exit_time(int lane,double entry) const {
    const auto& road=roads[lane];
    // max() and next_green() are monotone: a later entry never exits earlier (FIFO).
    return next_green(lane,std::max(std::max(entry,road.entry_at)+graph.lanes[lane].length/speed,road.clears_at));
}
RouteTree TravelForecast::tree(int source,bool by_distance) const {
    RouteTree result;result.source=source;result.predecessor.assign(graph.nodes.size(),-1);
    std::vector<double> costs(graph.nodes.size(),std::numeric_limits<double>::infinity());
    using Entry=std::pair<double,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> frontier;
    costs[source]=by_distance?0:observed_at;frontier.push({costs[source],source});
    while(!frontier.empty()) {
        auto [cost,node]=frontier.top();frontier.pop();
        if(cost>costs[node]) continue;
        if(node!=source) result.reachable.push_back(node);
        for(int id:graph.nodes[node].outgoing) {
            const auto& lane=graph.lanes[id];if(lane.parallel_index) continue;
            const double candidate=by_distance?cost+lane.length:exit_time(id,cost);
            if(candidate<costs[lane.to]) {
                costs[lane.to]=candidate;result.predecessor[lane.to]=id;frontier.push({candidate,lane.to});
            }
        }
    }
    // Destination sampling stays independent of whether time or distance is optimized.
    std::sort(result.reachable.begin(),result.reachable.end());
    return result;
}
Route TravelForecast::route(int source,int destination,bool by_distance) const {
    const auto paths=tree(source,by_distance);
    Route result;
    if(source!=destination && paths.predecessor[destination]<0) return result;
    result.reachable=true;
    for(int node=destination;node!=source;) {
        int lane=paths.predecessor[node];result.lanes.push_back(lane);node=graph.lanes[lane].from;
    }
    std::reverse(result.lanes.begin(),result.lanes.end());
    double at=observed_at;
    for(int lane:result.lanes) {at=exit_time(lane,at);result.distance+=graph.lanes[lane].length;}
    result.seconds=at-observed_at;
    return result;
}
}
