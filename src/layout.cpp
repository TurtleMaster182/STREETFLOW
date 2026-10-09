#include "graph.hpp"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <stdexcept>

namespace sf {
namespace {
constexpr double ideal = 220;
struct Point {
    double x=0,y=0;
    Point operator+(Point b) const {return {x+b.x,y+b.y};}
    Point operator-(Point b) const {return {x-b.x,y-b.y};}
    Point operator*(double scale) const {return {x*scale,y*scale};}
    Point& operator+=(Point b) {x+=b.x;y+=b.y;return *this;}
};
double length(Point p) {return std::hypot(p.x,p.y);}
double random_fraction(uint32_t id,uint32_t attempt) {
    uint32_t value=id^((attempt+1)*0x9e3779b9u);
    value^=value>>16;value*=0x7feb352du;value^=value>>15;value*=0x846ca68bu;value^=value>>16;
    return static_cast<double>(value)/4294967296.0;
}
Point direction(uint32_t id,uint32_t attempt) {
    double angle=6.283185307179586*random_fraction(id,attempt);
    return {std::cos(angle),std::sin(angle)};
}
struct Edge {int a,b;double target,clearance;};
struct Bounds {
    double left=std::numeric_limits<double>::infinity(),right=-left,top=left,bottom=-left;
    void add(Point p) {left=std::min(left,p.x);right=std::max(right,p.x);top=std::min(top,p.y);bottom=std::max(bottom,p.y);}
    double width() const {return right-left;}
    double height() const {return bottom-top;}
};

// Barnes-Hut groups distant nodes, avoiding an all-pairs repulsion pass on large cities.
class RepulsionTree {
    struct Cell {
        double left,top,size;
        Point center;
        size_t begin,end;
        std::array<int,4> children{-1,-1,-1,-1};
    };
    const std::vector<Point>& points;
    std::vector<int> order,scratch;
    std::vector<Cell> cells;
    int build(size_t begin,size_t end,double left,double top,double size,int depth) {
        int index=static_cast<int>(cells.size());
        Point center;for(size_t i=begin;i<end;++i)center+=points[order[i]];
        center=center*(1.0/(end-begin));
        cells.push_back({left,top,size,center,begin,end,{-1,-1,-1,-1}});
        if(end-begin<=1 || depth>=24 || size<1e-6)return index;
        double half=size/2;
        auto quadrant=[&](int body){const auto& p=points[body];return (p.x>=left+half?1:0)+(p.y>=top+half?2:0);};
        std::array<size_t,4> count{},start{},cursor{};
        for(size_t i=begin;i<end;++i)++count[quadrant(order[i])];
        start[0]=begin;for(size_t q=1;q<4;++q)start[q]=start[q-1]+count[q-1];cursor=start;
        for(size_t i=begin;i<end;++i)scratch[cursor[quadrant(order[i])]++]=order[i];
        std::copy(scratch.begin()+begin,scratch.begin()+end,order.begin()+begin);
        for(size_t q=0;q<4;++q)if(count[q]) {
            int child=build(start[q],start[q]+count[q],left+(q%2)*half,top+(q/2)*half,half,depth+1);
            cells[index].children[q]=child;
        }
        return index;
    }
    Point force(int body,int index) const {
        const auto& cell=cells[index];const auto& p=points[body];
        Point delta=p-cell.center;double distance=length(delta);
        bool contains=p.x>=cell.left && p.x<=cell.left+cell.size && p.y>=cell.top && p.y<=cell.top+cell.size;
        if(!contains && cell.size/std::max(1.0,distance)<0.7)
            return delta*(ideal*ideal*.12*(cell.end-cell.begin)/(distance*distance+25));
        Point result;
        if(std::all_of(cell.children.begin(),cell.children.end(),[](int child){return child<0;})) {
            for(size_t i=cell.begin;i<cell.end;++i)if(order[i]!=body) {
                Point apart=p-points[order[i]];
                if(length(apart)<1e-6)apart=direction(static_cast<uint32_t>(std::min(body,order[i])),static_cast<uint32_t>(std::max(body,order[i])))*(body<order[i]?1.0:-1.0);
                result+=apart*(ideal*ideal*.12/(apart.x*apart.x+apart.y*apart.y+25));
            }
        } else for(int child:cell.children)if(child>=0)result+=force(body,child);
        return result;
    }
public:
    explicit RepulsionTree(const std::vector<Point>& p):points(p),order(p.size()),scratch(p.size()){cells.reserve(p.size()*3);}
    void rebuild() {
        cells.clear();std::iota(order.begin(),order.end(),0);
        Bounds bounds;for(auto p:points)bounds.add(p);
        build(0,points.size(),bounds.left-1,bounds.top-1,std::max({bounds.width(),bounds.height(),1.0})+2,0);
    }
    Point force(int body) const {return force(body,0);}
};

void separate(std::vector<Point>& points,const std::vector<Edge>& edges,const std::vector<bool>& fixed,const std::vector<double>& radii,int iterations) {
    double cell=2.0*(*std::max_element(radii.begin(),radii.end()))+24;
    bool check_roads=points.size()<=250 && edges.size()<=150000/points.size();
    for(int step=0;step<iterations;++step) {
        double error=0;
        std::map<std::pair<int,int>,std::vector<int>> buckets;
        for(size_t i=0;i<points.size();++i)buckets[{static_cast<int>(std::floor(points[i].x/cell)),static_cast<int>(std::floor(points[i].y/cell))}].push_back(static_cast<int>(i));
        for(size_t i=0;i<points.size();++i) {
            int x=static_cast<int>(std::floor(points[i].x/cell)),y=static_cast<int>(std::floor(points[i].y/cell));
            for(int dx=-1;dx<=1;++dx)for(int dy=-1;dy<=1;++dy) {
                auto found=buckets.find({x+dx,y+dy});if(found==buckets.end())continue;
                for(int j:found->second) {
                    if(j<=static_cast<int>(i) || (fixed[i]&&fixed[j]))continue;
                    Point delta=points[i]-points[j];double distance=length(delta),gap=radii[i]+radii[j]+24;
                    if(distance>=gap)continue;
                    Point normal=distance>1e-8?delta*(1.0/distance):direction(static_cast<uint32_t>(i),static_cast<uint32_t>(j));
                    double overlap=gap-distance;error=std::max(error,overlap);
                    double first=fixed[i]?0:fixed[j]?1:.5;
                    points[i]+=normal*(overlap*first);points[j]+=normal*(-overlap*(1-first));
                }
            }
        }
        if(check_roads)for(const auto& edge:edges) {
            Point delta=points[edge.b]-points[edge.a];double squared=delta.x*delta.x+delta.y*delta.y;
            if(squared<1e-8)continue;
            for(size_t i=0;i<points.size();++i) {
                if(static_cast<int>(i)==edge.a || static_cast<int>(i)==edge.b)continue;
                Point relative=points[i]-points[edge.a];double t=(relative.x*delta.x+relative.y*delta.y)/squared;
                if(t<=0 || t>=1)continue;
                Point apart=relative-delta*t;double distance=length(apart);
                if(distance>=edge.clearance)continue;
                double weight=(fixed[i]?0:1)+(fixed[edge.a]?0:(1-t)*(1-t))+(fixed[edge.b]?0:t*t);
                if(weight==0)continue;
                Point normal=distance>1e-8?apart*(1.0/distance):Point{-delta.y,delta.x}*(1.0/std::sqrt(squared));
                double overlap=edge.clearance-distance;error=std::max(error,overlap);
                Point push=normal*(overlap/weight);
                if(!fixed[i])points[i]+=push;
                if(!fixed[edge.a])points[edge.a]+=push*(-(1-t));
                if(!fixed[edge.b])points[edge.b]+=push*(-t);
                // Endpoint movement changes the segment for the next node.
                delta=points[edge.b]-points[edge.a];squared=delta.x*delta.x+delta.y*delta.y;
                if(squared<1e-8)break;
            }
        }
        if(error<.01)break;
    }
}
void relax(std::vector<Point>& points,const std::vector<Edge>& edges,const std::vector<bool>& fixed,const std::vector<double>& radii) {
    if(points.size()<2)return;
    std::vector<Point> forces(points.size()),velocities(points.size());
    RepulsionTree tree(points);
    Point center;size_t anchored=0;
    for(size_t i=0;i<points.size();++i)if(fixed[i]){center+=points[i];++anchored;}
    if(anchored)center=center*(1.0/anchored);
    const int iterations=points.size()>2000?280:360;
    int settled=0;
    for(int step=0;step<iterations;++step) {
        tree.rebuild();
        for(size_t i=0;i<points.size();++i)forces[i]=tree.force(static_cast<int>(i))+(center-points[i])*.008;
        for(const auto& edge:edges) {
            Point delta=points[edge.b]-points[edge.a];double distance=std::max(1.0,length(delta));
            Point pull=delta*(.35*(distance-edge.target)/distance);
            forces[edge.a]+=pull;forces[edge.b]+=pull*(-1);
        }
        // Small diagrams also push unrelated nodes away from the interiors of roads.
        if(points.size()<=180 && edges.size()<=100000/points.size())for(const auto& edge:edges) {
            Point delta=points[edge.b]-points[edge.a];double squared=delta.x*delta.x+delta.y*delta.y;
            if(squared<1)continue;
            for(size_t i=0;i<points.size();++i) {
                if(static_cast<int>(i)==edge.a || static_cast<int>(i)==edge.b)continue;
                Point relative=points[i]-points[edge.a];double t=(relative.x*delta.x+relative.y*delta.y)/squared;
                if(t<=.08 || t>=.92)continue;
                Point apart=relative-delta*t;double distance=length(apart);
                if(distance>=edge.clearance)continue;
                if(distance<1e-6){apart={-delta.y,delta.x};distance=length(apart);apart=apart*(1.0/distance);distance=1;}
                Point push=apart*((edge.clearance-distance)*1.2/std::max(1.0,distance));
                forces[i]+=push;forces[edge.a]+=push*(-(1-t));forces[edge.b]+=push*(-t);
            }
        }
        double temperature=45*std::pow(.004,static_cast<double>(step)/iterations)+.1,max_move=0;
        for(size_t i=0;i<points.size();++i)if(!fixed[i]) {
            velocities[i]=velocities[i]*.45+forces[i]*.55;
            Point movement=velocities[i]*std::min(1.0,temperature/std::max(1e-9,length(velocities[i])));
            points[i]+=movement;max_move=std::max(max_move,length(movement));
        }
        if(step%12==0)separate(points,edges,fixed,radii,1);
        if(step>80 && max_move<.035){if(++settled>=12)break;}else settled=0;
    }
    separate(points,edges,fixed,radii,160);
}
double cross(Point a,Point b){return a.x*b.y-a.y*b.x;}
double quality(const std::vector<Point>& points,const std::vector<Edge>& edges,const std::vector<double>& radii) {
    double score=0;
    for(const auto& edge:edges){double error=length(points[edge.a]-points[edge.b])-edge.target;score+=error*error;}
    for(size_t i=0;i<points.size();++i)for(size_t j=i+1;j<points.size();++j) {
        double overlap=std::max(0.0,radii[i]+radii[j]+24-length(points[i]-points[j]));score+=overlap*overlap*100;
    }
    for(size_t i=0;i<edges.size();++i)for(size_t j=i+1;j<edges.size();++j) {
        const auto& a=edges[i];const auto& b=edges[j];
        if(a.a==b.a || a.a==b.b || a.b==b.a || a.b==b.b)continue;
        Point ab=points[a.b]-points[a.a],cd=points[b.b]-points[b.a];
        double first=cross(ab,points[b.a]-points[a.a]),second=cross(ab,points[b.b]-points[a.a]);
        double third=cross(cd,points[a.a]-points[b.a]),fourth=cross(cd,points[a.b]-points[b.a]);
        if(((first>0 && second<0)||(first<0 && second>0)) && ((third>0 && fourth<0)||(third<0 && fourth>0)))score+=ideal*ideal*12;
    }
    return score;
}
}

void Graph::automatic_layout(const std::vector<bool>& fixed,bool use_positions) {
    if(nodes.empty())return;
    if(!fixed.empty() && fixed.size()!=nodes.size())throw std::runtime_error("Layout anchor count does not match nodes");
    std::vector<bool> anchored=fixed.empty()?std::vector<bool>(nodes.size(),false):fixed;
    if(std::all_of(anchored.begin(),anchored.end(),[](bool value){return value;}))return;
    std::vector<int> order(nodes.size()),local_index(nodes.size(),-1);
    std::iota(order.begin(),order.end(),0);
    std::sort(order.begin(),order.end(),[&](int a,int b){return nodes[a].id<nodes[b].id;});
    std::vector<std::vector<int>> components;
    std::vector<bool> seen(nodes.size());
    for(int root:order)if(!seen[root]) {
        std::vector<int> component{root};seen[root]=true;
        for(size_t i=0;i<component.size();++i)for(int neighbour:nodes[component[i]].neighbours)
            if(!seen[neighbour]){seen[neighbour]=true;component.push_back(neighbour);}
        std::sort(component.begin(),component.end(),[&](int a,int b){return nodes[a].id<nodes[b].id;});
        components.push_back(std::move(component));
    }
    // Reverse roads and parallel lanes count as one spring between two nodes.
    std::map<std::pair<int,int>,double> lengths;
    std::map<std::pair<int,int>,double> widths;
    for(const auto& lane:lanes) {
        auto key=std::minmax(lane.from,lane.to);auto [it,inserted]=lengths.emplace(key,lane.length);
        if(!inserted)it->second=std::min(it->second,lane.length);
        widths[key]=std::max(widths[key],6+9.0*(lane.parallel_count-1)+3.5);
    }
    struct Floating {std::vector<int> nodes;Bounds bounds;};
    std::vector<Floating> floating;
    Bounds anchored_bounds;bool have_anchors=false;
    for(const auto& component:components) {
        std::vector<Point> initial(component.size());std::vector<bool> pins(component.size());
        std::vector<Edge> edges;std::vector<double> road_lengths;
        std::vector<double> radii(component.size(),30);
        std::vector<std::vector<int>> adjacent(component.size());
        bool pinned=false;
        for(size_t i=0;i<component.size();++i){local_index[component[i]]=static_cast<int>(i);pins[i]=anchored[component[i]];pinned=pinned||pins[i];initial[i]={nodes[component[i]].x,nodes[component[i]].y};}
        for(size_t i=0;i<component.size();++i)for(int neighbour:nodes[component[i]].neighbours) {
            int j=local_index[neighbour];adjacent[i].push_back(j);
            if(static_cast<int>(i)<j) {
                double road_length=lengths.at(std::minmax(component[i],neighbour));
                double width=widths.at(std::minmax(component[i],neighbour));
                edges.push_back({static_cast<int>(i),j,road_length,width+30});road_lengths.push_back(road_length);
                radii[i]=std::max(radii[i],width+18);radii[j]=std::max(radii[j],width+18);
            }
        }
        for(auto& neighbours:adjacent)std::sort(neighbours.begin(),neighbours.end());
        std::sort(edges.begin(),edges.end(),[](const Edge& a,const Edge& b){return std::pair{a.a,a.b}<std::pair{b.a,b.b};});
        std::sort(road_lengths.begin(),road_lengths.end());
        double median=road_lengths.empty()?1:road_lengths[road_lengths.size()/2];
        for(auto& edge:edges)edge.target=std::max(ideal*std::pow(std::clamp(edge.target/median,.4,2.5),.3),radii[edge.a]+radii[edge.b]+100);
        if(use_positions && !pinned) {
            std::vector<double> distances;Point center;
            for(const auto& edge:edges)distances.push_back(length(initial[edge.a]-initial[edge.b]));
            std::sort(distances.begin(),distances.end());
            double scale=distances.empty()?1:ideal/std::max(1.0,distances[distances.size()/2]);
            for(auto point:initial)center+=point;
            center=center*(1.0/initial.size());
            for(auto& point:initial)point=(point-center)*scale;
        }
        if(pinned) {
            std::queue<int> frontier;auto placed=pins;
            for(size_t i=0;i<pins.size();++i)if(pins[i])frontier.push(static_cast<int>(i));
            while(!frontier.empty()) {
                int source=frontier.front();frontier.pop();
                for(int target:adjacent[source])if(!placed[target]) {
                    Point center;size_t neighbours=0;
                    for(int neighbour:adjacent[target])if(placed[neighbour]){center+=initial[neighbour];++neighbours;}
                    initial[target]=center*(1.0/neighbours)+direction(static_cast<uint32_t>(nodes[component[target]].id),0)*ideal;
                    placed[target]=true;frontier.push(target);
                }
            }
        }
        std::vector<Point> best;double best_score=std::numeric_limits<double>::infinity();
        int attempts=!pinned && component.size()<=80?(use_positions?4:3):1;
        for(int attempt=0;attempt<attempts;++attempt) {
            auto points=initial;
            if(!pinned && (!use_positions || attempt>0))for(size_t i=0;i<points.size();++i) {
                double angle=i*2.399963229728653+random_fraction(static_cast<uint32_t>(nodes[component[i]].id),attempt)*(attempt?6.283185307179586:.3);
                double radius=ideal*.65*std::sqrt(i+1.0);points[i]={radius*std::cos(angle),radius*std::sin(angle)};
            }
            relax(points,edges,pins,radii);
            double score=attempts>1?quality(points,edges,radii):0;
            if(score<best_score){best_score=score;best=std::move(points);}
        }
        Bounds bounds;
        for(size_t i=0;i<component.size();++i) {
            auto& node=nodes[component[i]];node.x=best[i].x;node.y=best[i].y;bounds.add(best[i]);
            if(pinned){anchored_bounds.add(best[i]);have_anchors=true;}
        }
        if(!pinned)floating.push_back({component,bounds});
    }
    // Pack disconnected components separately so unrelated clusters cannot overlap.
    double area=0;for(const auto& part:floating)area+=(part.bounds.width()+180)*(part.bounds.height()+180);
    double row_width=std::sqrt(area)*1.3,origin_x=have_anchors?anchored_bounds.right+180:0;
    double x=origin_x,y=have_anchors?anchored_bounds.top:0,row_height=0;
    for(const auto& part:floating) {
        double w=part.bounds.width()+180,h=part.bounds.height()+180;
        if(x>origin_x && x-origin_x+w>row_width){x=origin_x;y+=row_height;row_height=0;}
        for(int id:part.nodes){nodes[id].x+=x-part.bounds.left;nodes[id].y+=y-part.bounds.top;}
        x+=w;row_height=std::max(row_height,h);
    }
    if(!have_anchors) {
        Bounds bounds;for(const auto& node:nodes)bounds.add({node.x,node.y});
        Point center{(bounds.left+bounds.right)/2,(bounds.top+bounds.bottom)/2};
        for(auto& node:nodes){node.x-=center.x;node.y-=center.y;}
    }
}
}
