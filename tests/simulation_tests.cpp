#include "graph.hpp"
#include "simulation.hpp"
#include <cmath>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
void require(bool condition,const std::string& message) { if(!condition) throw std::runtime_error(message); }
sf::Graph city(const std::string& text) { std::istringstream in(text); return sf::Graph::read(in); }
sf::Config config() { sf::Config c; c.spawn_rate=0; c.car_speed=2; return c; }
void tick(sf::Simulation& sim,int count) { for(int i=0;i<count;++i)sim.tick(); }
void invariants(const sf::Simulation& sim) {
    size_t queued=0;
    for(size_t lane=0;lane<sim.queues.size();++lane) {
        double ahead=sim.graph.lanes[lane].length+sf::Simulation::spacing;
        for(auto slot:sim.queues[lane]) {
            const auto& car=sim.cars[slot]; ++queued;
            require(car.active && car.lane==static_cast<int>(lane),"Lane membership broken");
            require(car.position>=0 && car.position<=sim.graph.lanes[lane].length+1e-8,"Car outside lane");
            require(ahead-car.position>=sf::Simulation::spacing-1e-8,"Cars overlap");
            require(car.velocity<=sim.config.car_speed+1e-8,"Car exceeds speed");
            ahead=car.position;
        }
    }
    require(queued==sim.active_count,"Active cars not accounted for");
    require(sim.stats.spawned==sim.stats.completed+sim.stats.abandoned+sim.active_count,"Car conservation broken");
    require(std::accumulate(sim.lane_crashouts.begin(),sim.lane_crashouts.end(),uint64_t{0})==sim.stats.abandoned,"Road crashouts do not match total");
}
}
int main() {
    try {
        {
            auto g=city("# directed multigraph\n1 5 0 10\n1 5 0 10\n5 1 0 10\n5 8 1 12\n");
            require(g.nodes.size()==3 && g.lanes.size()==4,"Input graph incorrect");
            require(g.lanes[0].parallel_count==2 && g.lanes[1].parallel_index==1,"Repeated lines not lanes");
            require(g.nodes[g.node_index.at(5)].neighbours.size()==2,"Parallel/reverse edges inflate degree");
        }
        for(const auto* input:{"","1 2 2 10","1 2 0 -1","1 2 0 0","1 1 0 1","1 2 0 10 junk","a b 0 1"}) {
            bool rejected=false; try{city(input);}catch(const std::runtime_error&){rejected=true;}
            require(rejected,"Malformed input accepted");
        }
        {
            auto g=city("1 2 0 10\n2 3 0 10\n"); sf::Simulation sim(g,config());
            require(sim.add_trip({0,1}),"Could not add trip");
            require(sim.snapshot(false,1,0).find("\"intersectionQueues\":[0,1,0]")!=std::string::npos,"Queues must count the next intersection even with no rendered cars");
            tick(sim,50); require(std::abs(sim.cars[0].position-5)<1e-8,"Distance does not equal speed * time");
            require(sim.cars[0].lane==0 && sim.stats.completed==0,"Car teleported");
            require(sim.snapshot(false,1).find("\"intersectionQueues\":[0,0,0]")!=std::string::npos,"Moving cars must not count as queued");
            tick(sim,50); require(sim.cars[0].lane==1 && sim.cars[0].position==0,"Intersection transfer failed");
            tick(sim,100); require(sim.stats.completed==1,"Trip did not complete");
            require(std::abs(sim.stats.total_trip_time-10)<1e-8,"Wrong free-flow trip duration");
            require(sim.snapshot(false,1).find("\"intersectionQueues\":[0,0,0]")!=std::string::npos,"Completed trips must leave intersection queues");
            sim.add_trip({0,1});sim.reset();
            require(sim.snapshot(false,1).find("\"intersectionQueues\":[0,0,0]")!=std::string::npos,"Reset must clear intersection queues");
        }
        {
            // Two parallel approaches wait on red; unfinished trips leave on their actual lane.
            auto g=city("1 3 1 1\n2 3 1 1\n2 3 1 1\n3 4 0 2\n");
            auto c=config();c.green=10;c.max_wait=.2;
            sf::Simulation sim(g,c);
            require(sim.add_trip({1,3}) && sim.add_trip({1,3}),"Crashout setup failed");
            tick(sim,9);require(sim.stats.abandoned==0 && sim.active_count==2,"Cars disappeared before reaching a node");
            tick(sim,11);invariants(sim);
            require(sim.stats.abandoned==2 && sim.stats.completed==0,"Unfinished trips not counted as crashouts");
            require(sim.lane_crashouts==std::vector<uint64_t>({0,1,1,0}),"Crashouts attributed to the wrong lanes");
            require(sim.snapshot(false,1).find("\"laneCrashouts\":[0,1,1,0]")!=std::string::npos,"Snapshot missing lane crashout counts");
            require(sim.add_trip({1}),"Destination-arrival setup failed");tick(sim,20);invariants(sim);
            require(sim.stats.completed==1 && sim.stats.abandoned==2,"Arrival at destination was counted as a crashout");
            sim.reset();invariants(sim);
            require(sim.lane_crashouts==std::vector<uint64_t>(4,0),"Reset retained road crashouts");
        }
        {
            auto g=city("1 3 1 1\n2 3 1 1\n3 4 0 20\n"); auto c=config(); c.green=2;c.yellow=1;c.all_red=1;
            sf::Simulation sim(g,c); require(sim.add_trip({1,2}),"Could not add red approach car");
            tick(sim,60); require(sim.cars[0].lane==1 && sim.cars[0].position==1,"Car ran red light");
            require(sim.signal(0)==0,"All-red phase missing");
            tick(sim,22); require(sim.cars[0].lane==2,"Car did not leave on green");
            require(sim.stats.total_wait>3,"Red-light waiting not counted");
        }
        {
            auto g=city("1 2 0 0.1\n2 3 1 1\n4 3 1 1\n"); auto c=config();c.green=10;
            sf::Simulation sim(g,c); tick(sim,290); // Second approach is green; lane 1 is red.
            require(sim.add_trip({1}),"First blocking car not admitted");tick(sim,10);
            require(sim.add_trip({1}),"Second blocking car not admitted");
            require(sim.add_trip({0,1}),"Upstream car not admitted");tick(sim,10);
            require(sim.queues[0].size()==1,"Car entered blocked downstream lane");invariants(sim);
        }
        {
            auto g=city("1 2 0 10\n1 2 0 10\n"); sf::Simulation sim(g,config());
            require(sim.add_trip({0}) && sim.add_trip({0}),"Parallel capacity unavailable");
            require(sim.queues[0].size()==1 && sim.queues[1].size()==1,"Parallel lanes unused");
            require(!sim.add_trip({0}),"Car spawned over occupied lane entrance");
        }
        {
            auto g=city("1 2 1 10\n2 3 1 10\n3 1 1 10\n2 1 1 10\n1 3 1 10\n3 2 1 10\n");
            auto c=config();c.spawn_rate=15;c.max_cars=30;
            sf::Simulation a(g,c),b(g,c);
            for(int i=0;i<4000;++i) {a.tick();b.tick();invariants(a);require(a.active_count<=30,"Car cap exceeded");}
            require(a.snapshot(false,1)==b.snapshot(false,1),"Seed is not deterministic");
            require(a.stats.completed>0,"Network never completes trips");
            auto before=a.snapshot(false,1);a.reset();tick(a,4000);
            require(a.snapshot(false,1)==before,"Reset does not replay the seed");
        }
        {
            auto g=city("1 2 0 4\n5 6 0 4\n");auto c=config();c.spawn_rate=5;sf::Simulation sim(g,c);
            tick(sim,1000);invariants(sim);require(sim.stats.completed>0,"Disconnected/sink graph routing failed");
        }
        {
            auto g=city("1 2 1 2\n2 4 0 2\n1 3 0 2.5\n3 4 0 2.5\n9 2 1 2\n");
            auto c=config();c.green=1.2;c.yellow=.4;c.all_red=.4;
            sf::Simulation sim(g,c);
            int from=g.node_index.at(1),to=g.node_index.at(4);
            auto initially=sim.forecast().route(from,to);
            require(initially.lanes==std::vector<int>({0,1}),"Empty network should use the short route");
            require(std::abs(initially.seconds-2)<1e-8,"Empty route ETA wrong");
            require(sim.add_trip({0,1}),"Queue setup failed");sim.cars[0].position=1.8;
            require(sim.add_trip({0,1}),"Queue setup failed");sim.cars[1].position=.95;
            require(sim.add_trip({0,1}),"Queue setup failed");
            const auto& forecast=sim.forecast();
            auto fast=forecast.route(from,to),shortest=forecast.route(from,to,true);
            require(fast.lanes==std::vector<int>({2,3}),"Congested short road did not cause a detour");
            require(shortest.lanes==std::vector<int>({0,1}),"Distance baseline wrong");
            require(fast.distance>shortest.distance && fast.seconds<shortest.seconds,"Longer route must be faster here");
            require(forecast.roads[0].congestion>0 && forecast.roads[0].traffic_delay>0,"Queue congestion not measured");
            require(forecast.exit_time(0,1)>=4,"Signal phase at arrival was ignored");
            for(size_t lane=0;lane<g.lanes.size();++lane) {
                double previous=-1;
                for(int step=0;step<2000;++step) {
                    double enter=step*.01,leave=forecast.exit_time(static_cast<int>(lane),enter);
                    require(leave+1e-8>=previous,"Travel-time function violates FIFO");
                    require(leave+1e-8>=enter+g.lanes[lane].length/c.car_speed,"ETA faster than cruising speed");
                    previous=leave;
                }
            }
            require(forecast.route(from,from).seconds==0,"Same-node route must take zero time");
            require(!forecast.route(to,from).reachable,"A one-way route was reversed");
            sim.reset();require(sim.forecast().route(from,to).lanes==initially.lanes,"Reset retained stale congestion");
        }
        {
            auto g=city("1 2 0 10\n2 4 1 2\n9 4 1 2\n1 3 0 7.8\n3 4 0 7.8\n");
            auto c=config();c.green=2;c.yellow=1;c.all_red=1;
            sf::Simulation sim(g,c);
            const auto& forecast=sim.forecast();
            auto fastest=forecast.route(g.node_index.at(1),g.node_index.at(4));
            require(fastest.lanes==std::vector<int>({3,4}),"Routing evaluated a downstream light at departure instead of arrival");
            require(std::abs(fastest.seconds-7.8)<1e-8,"Future-phase route ETA wrong");
        }
        {
            auto a=city("10 20 0 10\n20 30 0 10\n30 10 0 10\n"),b=city("10 20 0 10\n20 30 0 10\n30 10 0 10\n");
            require(a.json()==b.json(),"Automatic layout is not repeatable");
            require(std::abs(a.nodes[0].x-a.nodes[1].x)>1 && std::abs(a.nodes[0].y-a.nodes[1].y)>1,"Automatic layout is still axis-aligned");
            std::istringstream layout("10 0 0\n20 100 0\n30 40 70\n");a.read_layout(layout);
            require(a.nodes[1].x==100 && a.nodes[2].y==70,"Custom layout was not preserved");
        }
        std::cout<<"PASS: simulation invariants, deterministic reset, layouts, fastest-time detours, congestion refresh, future signal phases, FIFO and unreachable routes\n";
        return 0;
    } catch(const std::exception& e){std::cerr<<"FAIL: "<<e.what()<<'\n';return 1;}
}
