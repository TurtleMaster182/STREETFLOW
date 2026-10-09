#include "graph.hpp"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {
void require(bool condition,const std::string& message){if(!condition)throw std::runtime_error(message);}
sf::Graph city(const std::string& text,bool arrange=true){std::istringstream input(text);return sf::Graph::read(input,arrange);}
const sf::Node& node(const sf::Graph& graph,int id){return graph.nodes.at(graph.node_index.at(id));}
double distance(const sf::Graph& graph,int from,int to) {
    const auto& a=node(graph,from);const auto& b=node(graph,to);
    return std::hypot(a.x-b.x,a.y-b.y);
}
void proportional(const sf::Graph& graph,int a,int b,int c,int d,double ratio) {
    double drawn_ratio=distance(graph,a,b)/distance(graph,c,d);
    require(std::abs(drawn_ratio/ratio-1)<.05,"Drawing does not preserve road length proportions");
}
void clearance(const sf::Graph& graph) {
    std::vector<double> radii(graph.nodes.size(),30);
    for(const auto& lane:graph.lanes) {
        double radius=6+9.0*(lane.parallel_count-1)+3.5+18;
        radii[lane.from]=std::max(radii[lane.from],radius);radii[lane.to]=std::max(radii[lane.to],radius);
    }
    for(size_t i=0;i<graph.nodes.size();++i) {
        const auto& a=graph.nodes[i];require(std::isfinite(a.x)&&std::isfinite(a.y),"Non-finite drawing coordinate");
        for(size_t j=i+1;j<graph.nodes.size();++j) {
            const auto& b=graph.nodes[j];require(std::hypot(a.x-b.x,a.y-b.y)>=radii[i]+radii[j]+23.9,"Nodes or lane bundles overlap");
        }
    }
}
}
int main() {
    try {
        std::string roads="10 20 0 10\n20 30 0 15\n30 40 0 10\n40 10 0 15\n20 50 0 12\n50 40 0 12\n";
        auto a=city(roads),b=city(roads);clearance(a);
        require(a.json()==b.json(),"Automatic layout is not deterministic");
        // A 300-unit highway must be about 30 times a 10-unit road, both in
        // one component and across disconnected components (one city scale).
        auto highway=city("1 2 0 10\n2 3 0 300\n");clearance(highway);
        proportional(highway,2,3,1,2,30);
        auto islands=city("1 2 0 10\n3 4 0 300\n");clearance(islands);
        proportional(islands,3,4,1,2,30);
        auto triangle=city("1 2 0 10\n2 3 0 20\n1 3 0 25\n");clearance(triangle);
        proportional(triangle,2,3,1,2,2);
        proportional(triangle,1,3,1,2,2.5);
        auto hinted=city("1 2 0 10\n2 3 0 300\n",false);
        std::istringstream equal_hints("1 0 0\n2 220 0\n3 440 0\n");hinted.read_layout(equal_hints,false);
        clearance(hinted);proportional(hinted,2,3,1,2,30);
        auto fixed=city("1 2 0 10\n2 3 0 300\n",false);
        std::istringstream fixed_positions("1 0 0\n2 220 0\n3 440 0\n");fixed.read_layout(fixed_positions);
        require(distance(fixed,2,3)==220 && node(fixed,3).x==440,"Explicit fixed coordinates were changed");
        // Inconsistent cycles cannot satisfy all lengths; placement still
        // needs to be finite, deterministic and clear of overlapping nodes.
        std::string impossible="1 2 0 10\n2 3 0 10\n1 3 0 300\n";
        auto conflict=city(impossible);clearance(conflict);
        require(conflict.json()==city(impossible).json(),"Conflicting road lengths produce unstable placement");
        auto reordered=city("50 40 0 12\n40 10 0 15\n20 50 0 12\n30 40 0 10\n20 30 0 15\n10 20 0 10\n20 10 0 10\n");
        for(const auto& original:a.nodes) {
            const auto& same=node(reordered,original.id);
            require(std::hypot(original.x-same.x,original.y-same.y)<1e-8,"Lane order or reverse roads changed the drawing");
        }
        auto extended=city(roads+"20 99 0 8\n70 80 0 10\n",false);
        std::istringstream partial("10 0 0\n20 200 0\n30 200 200\n40 0 200\n50 100 100\n");
        extended.read_layout(partial);clearance(extended);
        require(node(extended,20).x==200 && node(extended,20).y==0,"Saved node moved while adding a new node");
        require(node(extended,50).x==100 && node(extended,50).y==100,"Saved layout was not preserved");
        const auto& added=node(extended,99);
        require(std::hypot(added.x-200,added.y)<600,"New node was not positioned near its connection");
        double anchored_right=std::max({200.0,added.x});
        require(std::min(node(extended,70).x,node(extended,80).x)>anchored_right+100,"New disconnected cluster overlaps the saved city");
        auto grouped=city("1 2 0 10\n2 3 0 10\n4 5 0 10\n5 6 0 10\n");clearance(grouped);
        double first_right=std::max({node(grouped,1).x,node(grouped,2).x,node(grouped,3).x});
        double second_left=std::min({node(grouped,4).x,node(grouped,5).x,node(grouped,6).x});
        double first_bottom=std::max({node(grouped,1).y,node(grouped,2).y,node(grouped,3).y});
        double second_top=std::min({node(grouped,4).y,node(grouped,5).y,node(grouped,6).y});
        require(second_left>first_right+100 || second_top>first_bottom+100,"Disconnected graph components overlap");
        extended.automatic_layout();clearance(extended);
        require(node(extended,20).x!=200 || node(extended,20).y!=0,"Full automatic rearrangement kept saved anchors");
        std::ostringstream star;
        for(int id=2;id<=24;++id)star<<"1 "<<id<<" 0 10\n";
        clearance(city(star.str()));
        // A saved, collinear third node must be moved out of a wide road corridor.
        for(int lanes:{2,8}) {
            std::ostringstream roads;
            for(int i=0;i<lanes;++i)roads<<"1 2 0 10\n2 3 0 10\n1 3 0 20\n";
            auto graph=city(roads.str(),false);
            std::istringstream hints("1 0 0\n2 110 0\n3 220 0\n");graph.read_layout(hints,false);clearance(graph);
            double required=6+9.0*(lanes-1)+3.5+30;
            for(const auto& road:graph.lanes)for(size_t i=0;i<graph.nodes.size();++i) {
                if(static_cast<int>(i)==road.from || static_cast<int>(i)==road.to)continue;
                const auto& a=graph.nodes[road.from];const auto& b=graph.nodes[road.to];const auto& p=graph.nodes[i];
                double dx=b.x-a.x,dy=b.y-a.y,den=dx*dx+dy*dy,t=((p.x-a.x)*dx+(p.y-a.y)*dy)/den;
                if(t>0 && t<1)require(std::hypot(p.x-a.x-t*dx,p.y-a.y-t*dy)>=required-.1,"Node sits inside an unrelated lane bundle");
            }
        }
        std::cout<<"PASS: proportional road lengths, layout clearance, deterministic placement, new nodes, saved anchors, disconnected components and lane-independent geometry\n";
        return 0;
    } catch(const std::exception& error){std::cerr<<"FAIL: "<<error.what()<<'\n';return 1;}
}
