#include "assets.hpp"
#include "graph.hpp"
#include "http_server.hpp"
#include "simulation.hpp"
#include <chrono>
#include <cmath>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <sys/wait.h>
#include <unistd.h>

namespace {
volatile std::sig_atomic_t interrupted=0;
void interrupt(int) { interrupted=1; }
const char* help=R"HELP(STREETFLOW 0.2.0 — graph traffic simulator

Usage: streetflow [CITY.IN] [options]

Reads ./CITY.IN when present; otherwise runs the built-in demo city.
Each row: starting_node ending_node traffic_light_at_end length
Repeated rows are parallel lanes. Reverse travel needs its own row.

  --city FILE         Load a city file (or use a positional filename)
  --layout FILE       Node drawing coordinates: node_id x y
                      Defaults to CITY.LAYOUT alongside the input file
  --port N            Local web port (default 8080)
  --no-browser        Print the URL without opening a browser
  --paused            Start paused
  --seed N            Repeatable random seed (default 42)
  --routing MODE      time (default) or distance, for spawned cars
  --route A B         Compare fastest and shortest routes between node IDs
  --spawn-rate N      Arrival attempts per simulated second (default 3)
  --max-cars N        Maximum active cars (default 5000)
  --max-wait N        Give up after N seconds of accumulated delay (default 60)
                      Despawn at the next node; 0 disables this behavior
  --speed N           Car speed, length units/second (default 2)
  --time-scale N      Simulation speed multiplier, 0.25–20 (default 1)
  --green N           Green seconds per incoming approach (default 8)
  --yellow N          Amber seconds; new crossings stop (default 2)
  --all-red N         Clearance seconds between approaches (default 1)
  --validate          Check the city and layout, then exit
  --headless          Run without browser/server, as fast as possible
  --duration N        Headless simulated seconds (default 60)
  --help              Show this help
  --version           Show version

Ctrl+C stops the app. All traffic and statistics are simulated.
)HELP";
double number(const std::string& s,const std::string& name,double min,double max) {
    size_t used=0; double value;
    try { value=std::stod(s,&used); } catch(...) { throw std::runtime_error(name+": expected a number"); }
    if(used!=s.size() || !std::isfinite(value) || value<min || value>max)
        throw std::runtime_error(name+": value outside allowed range");
    return value;
}
uint64_t integer(const std::string& s,const std::string& name,uint64_t min,uint64_t max) {
    double value=number(s,name,static_cast<double>(min),static_cast<double>(max));
    if(std::floor(value)!=value) throw std::runtime_error(name+": expected an integer");
    return static_cast<uint64_t>(value);
}
void open_browser(const std::string& url) {
    // Double fork keeps browser lifetime independent of the simulator. No shell interpolation.
    pid_t child=fork();
    if(child==0) {
        pid_t grandchild=fork();
        if(grandchild==0) {
            setsid();
            execlp("xdg-open","xdg-open",url.c_str(),static_cast<char*>(nullptr));
            _exit(127);
        }
        _exit(0);
    }
    if(child>0) waitpid(child,nullptr,0);
}
}
int main(int argc,char** argv) {
    try {
        sf::Config config; std::string city_file,layout_file;
        int port=8080; bool no_browser=false,paused=false,headless=false,validate=false;
        double time_scale=1,duration=60;
        std::optional<std::pair<int,int>> route_request;
        for(int i=1;i<argc;++i) {
            std::string arg=argv[i];
            auto value=[&] { if(i+1>=argc) throw std::runtime_error(arg+": missing value"); return std::string(argv[++i]); };
            if(arg=="--help" || arg=="-h") { std::cout<<help; return 0; }
            else if(arg=="--version") { std::cout<<"streetflow 0.2.0\n"; return 0; }
            else if(arg=="--city") city_file=value();
            else if(arg=="--layout") layout_file=value();
            else if(arg=="--no-browser") no_browser=true;
            else if(arg=="--paused") paused=true;
            else if(arg=="--headless") headless=true;
            else if(arg=="--validate") validate=true;
            else if(arg=="--port") port=static_cast<int>(integer(value(),arg,1024,65535));
            else if(arg=="--seed") config.seed=static_cast<uint32_t>(integer(value(),arg,0,UINT32_MAX));
            else if(arg=="--route") {
                int from=static_cast<int>(integer(value(),arg,0,INT32_MAX));
                int to=static_cast<int>(integer(value(),arg,0,INT32_MAX));
                route_request=std::pair{from,to};
            }
            else if(arg=="--routing") {
                auto mode=value();
                if(mode!="time" && mode!="distance") throw std::runtime_error("--routing: expected time or distance");
                config.time_routing=mode=="time";
            }
            else if(arg=="--max-cars") config.max_cars=integer(value(),arg,1,1000000);
            else if(arg=="--spawn-rate") config.spawn_rate=number(value(),arg,0,2000);
            else if(arg=="--max-wait") config.max_wait=number(value(),arg,0,10000000);
            else if(arg=="--speed") config.car_speed=number(value(),arg,0.001,10000);
            else if(arg=="--green") config.green=number(value(),arg,0.05,3600);
            else if(arg=="--yellow") config.yellow=number(value(),arg,0,3600);
            else if(arg=="--all-red") config.all_red=number(value(),arg,0,3600);
            else if(arg=="--time-scale") time_scale=number(value(),arg,0.25,20);
            else if(arg=="--duration") duration=number(value(),arg,0.05,10000000);
            else if(!arg.empty() && arg[0]!='-' && city_file.empty()) city_file=arg;
            else throw std::runtime_error("Unknown argument: "+arg+". Use --help.");
        }
        if(city_file.empty() && std::filesystem::exists("CITY.IN")) city_file="CITY.IN";
        sf::Graph graph;
        if(city_file.empty()) { std::istringstream stream{std::string(demo_city)}; graph=sf::Graph::read(stream); }
        else {
            std::ifstream stream(city_file); if(!stream) throw std::runtime_error("Cannot open city file: "+city_file);
            graph=sf::Graph::read(stream);
            if(layout_file.empty()) {
                auto candidate=std::filesystem::path(city_file).parent_path()/"CITY.LAYOUT";
                if(std::filesystem::exists(candidate)) layout_file=candidate.string();
            }
        }
        if(!layout_file.empty()) {
            std::ifstream stream(layout_file); if(!stream) throw std::runtime_error("Cannot open layout: "+layout_file);
            graph.read_layout(stream);
        } else if(city_file.empty()) { std::istringstream stream{std::string(demo_layout)}; graph.read_layout(stream); }
        if(validate) {
            std::cout<<"Valid city: "<<graph.nodes.size()<<" nodes, "<<graph.lanes.size()<<" directed lanes\n"; return 0;
        }
        sf::Simulation sim(graph,config);
        if(route_request) {
            if(!graph.node_index.contains(route_request->first) || !graph.node_index.contains(route_request->second))
                throw std::runtime_error("--route: starting and ending nodes must exist in the city");
            sim.selected_route=route_request;
        }
        std::signal(SIGINT,interrupt); std::signal(SIGTERM,interrupt); std::signal(SIGPIPE,SIG_IGN);
        if(headless) {
            auto start=std::chrono::steady_clock::now();
            uint64_t steps=static_cast<uint64_t>(std::ceil(duration/sf::Simulation::dt));
            for(uint64_t step=0;step<steps && !interrupted;++step) sim.tick();
            std::cout<<sim.summary()<<'\n';
            if(route_request) std::cout<<sim.route_comparison(route_request->first,route_request->second)<<'\n';
            std::cerr<<"Finished in "<<std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count()<<" wall seconds\n";
            return 0;
        }
        sf::SharedState shared; shared.snapshot=sim.snapshot(paused,time_scale);
        sf::HttpServer server(port,std::string(web_page),graph.json(),shared);
        std::string url="http://127.0.0.1:"+std::to_string(port);
        std::cout<<"\n  STREETFLOW\n  "<<graph.nodes.size()<<" nodes / "<<graph.lanes.size()<<" lanes / seed "<<config.seed
            <<"\n  City: "<<(city_file.empty()?"built-in demo":city_file)<<"\n\n  "<<url<<"\n\n  Ctrl+C to stop. Use --help for options.\n"<<std::flush;
        if(!no_browser) open_browser(url);
        auto previous=std::chrono::steady_clock::now(),last_snapshot=previous;
        double accumulated=0;
        while(!interrupted) {
            auto now=std::chrono::steady_clock::now();
            double elapsed=std::chrono::duration<double>(now-previous).count(); previous=now;
            std::vector<std::string> commands;
            { std::lock_guard lock(shared.mutex); commands.swap(shared.commands); }
            bool changed=!commands.empty();
            for(const auto& command:commands) {
                if(command=="pause") { paused=true; accumulated=0; }
                else if(command=="resume") paused=false;
                else if(command=="reset") { sim.reset(); accumulated=0; }
                else if(command.starts_with("speed=")) time_scale=std::stod(command.substr(6));
                else if(command.starts_with("spawn=")) sim.config.spawn_rate=std::stod(command.substr(6));
                else if(command.starts_with("route=")) {
                    auto comma=command.find(',');
                    sim.selected_route=std::pair{std::stoi(command.substr(6,comma-6)),std::stoi(command.substr(comma+1))};
                }
                else if(command=="clear-route") sim.selected_route.reset();
            }
            // Cap catch-up after suspension. Fixed steps preserve movement/queue invariants.
            if(!paused) accumulated+=std::min(elapsed,0.25)*time_scale;
            int steps=0;
            while(accumulated>=sf::Simulation::dt && steps<100) {
                sim.tick(); accumulated-=sf::Simulation::dt; ++steps;
            }
            if(changed || now-last_snapshot>=std::chrono::milliseconds(100)) {
                auto snapshot=sim.snapshot(paused,time_scale);
                { std::lock_guard lock(shared.mutex); shared.snapshot=std::move(snapshot); }
                last_snapshot=now;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2));
        }
        std::cout<<"\nStopped. "<<sim.summary()<<'\n';
        return 0;
    } catch(const std::exception& error) { std::cerr<<"streetflow: "<<error.what()<<'\n'; return 1; }
}
