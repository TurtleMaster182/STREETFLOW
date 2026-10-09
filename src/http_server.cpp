#include "http_server.hpp"
#include <algorithm>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

namespace sf {
namespace {
std::string trim(std::string s) {
    auto first=s.find_first_not_of(" \t\r\n");
    if(first==std::string::npos) return {};
    return s.substr(first,s.find_last_not_of(" \t\r\n")-first+1);
}
void respond(int socket,int code,const std::string& type,const std::string& body) {
    std::string reason=code==200?"OK":code==400?"Bad Request":code==403?"Forbidden":code==404?"Not Found":"Method Not Allowed";
    std::string data="HTTP/1.1 "+std::to_string(code)+" "+reason+"\r\nContent-Type: "+type+
        "\r\nContent-Length: "+std::to_string(body.size())+
        "\r\nCache-Control: no-store\r\nX-Content-Type-Options: nosniff\r\nConnection: close\r\n\r\n"+body;
    size_t sent=0;
    while(sent<data.size()) {
        auto n=::send(socket,data.data()+sent,data.size()-sent,MSG_NOSIGNAL);
        if(n<0 && errno==EINTR) continue;
        if(n<=0) return;
        sent+=static_cast<size_t>(n);
    }
}
bool valid_command(const std::string& command) {
    if(command=="pause" || command=="resume" || command=="reset" || command=="clear-route") return true;
    if(command.starts_with("route=")) {
        auto comma=command.find(',');
        if(comma==std::string::npos) return false;
        auto valid_id=[](const std::string& s) {
            if(s.empty() || s.size()>10 || !std::all_of(s.begin(),s.end(),[](unsigned char c){return std::isdigit(c);})) return false;
            try {return std::stoll(s)<=INT32_MAX;}catch(...) {return false;}
        };
        return valid_id(command.substr(6,comma-6)) && valid_id(command.substr(comma+1));
    }
    auto equals=command.find('='); if(equals==std::string::npos) return false;
    auto key=command.substr(0,equals);
    try {
        size_t used; auto value=command.substr(equals+1); double n=std::stod(value,&used);
        if(used!=value.size() || !std::isfinite(n)) return false;
        return (key=="speed" && n>=0.25 && n<=20) || (key=="spawn" && n>=0 && n<=2000);
    } catch(...) { return false; }
}
}
HttpServer::HttpServer(int port,std::string html,std::string city,SharedState& state)
    : port_(port), html_(std::move(html)), city_(std::move(city)), state_(state) {
    socket_=::socket(AF_INET,SOCK_STREAM,0);
    if(socket_<0) throw std::runtime_error("Cannot create local server socket: "+std::string(std::strerror(errno)));
    int reuse=1; setsockopt(socket_,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));
    sockaddr_in address{}; address.sin_family=AF_INET; address.sin_port=htons(static_cast<uint16_t>(port));
    address.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(::bind(socket_,reinterpret_cast<sockaddr*>(&address),sizeof(address))<0 || ::listen(socket_,16)<0) {
        auto error=std::string(std::strerror(errno)); ::close(socket_); socket_=-1;
        throw std::runtime_error("Cannot listen on 127.0.0.1:"+std::to_string(port)+": "+error+". Try --port 8081.");
    }
    thread_=std::thread([this]{serve();});
}
HttpServer::~HttpServer() { stopping_=true; if(thread_.joinable()) thread_.join(); if(socket_>=0) ::close(socket_); }
void HttpServer::serve() {
    while(!stopping_) {
        pollfd ready{socket_,POLLIN,0};
        if(::poll(&ready,1,100)<=0) continue;
        int client=::accept(socket_,nullptr,nullptr); if(client<0) continue;
        timeval timeout{0,250000};
        setsockopt(client,SOL_SOCKET,SO_RCVTIMEO,&timeout,sizeof(timeout));
        setsockopt(client,SOL_SOCKET,SO_SNDTIMEO,&timeout,sizeof(timeout));
        try { handle(client); } catch(...) { respond(client,400,"text/plain","Invalid request"); }
        ::close(client);
    }
}
void HttpServer::handle(int client) {
    std::string request; char buffer[4096]; size_t header_end=std::string::npos;
    while((header_end=request.find("\r\n\r\n"))==std::string::npos) {
        auto n=recv(client,buffer,sizeof(buffer),0); if(n<=0) return;
        request.append(buffer,static_cast<size_t>(n));
        if(request.size()>16384) { respond(client,400,"text/plain","Request too large"); return; }
    }
    std::istringstream headers(request.substr(0,header_end));
    std::string method,path,version,line; headers >> method >> path >> version; std::getline(headers,line);
    std::map<std::string,std::string> fields;
    while(std::getline(headers,line)) {
        auto colon=line.find(':'); if(colon==std::string::npos) continue;
        auto key=trim(line.substr(0,colon));
        std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
        fields[key]=trim(line.substr(colon+1));
    }
    std::string local="127.0.0.1:"+std::to_string(port_), hostname="localhost:"+std::to_string(port_);
    if(fields["host"]!=local && fields["host"]!=hostname) { respond(client,403,"text/plain","Local access only"); return; }
    if(fields.contains("origin") && fields["origin"]!="http://"+local && fields["origin"]!="http://"+hostname) {
        respond(client,403,"text/plain","Origin not allowed"); return;
    }
    if(method=="GET") {
        if(path=="/" || path=="/index.html") respond(client,200,"text/html; charset=utf-8",html_);
        else if(path=="/api/city") respond(client,200,"application/json",city_);
        else if(path=="/api/state") {
            std::string snapshot; { std::lock_guard lock(state_.mutex); snapshot=state_.snapshot; }
            respond(client,200,"application/json",snapshot);
        } else respond(client,404,"text/plain","Not found");
        return;
    }
    if(method!="POST" || path!="/api/control") { respond(client,405,"text/plain","Method not allowed"); return; }
    size_t length=0;
    if(fields.contains("content-length")) {
        size_t used; length=std::stoul(fields["content-length"],&used);
        if(used!=fields["content-length"].size()) throw std::runtime_error("Bad length");
    }
    if(length>128 || fields.contains("transfer-encoding")) { respond(client,400,"text/plain","Invalid body"); return; }
    size_t body_start=header_end+4;
    while(request.size()<body_start+length) {
        auto n=recv(client,buffer,sizeof(buffer),0); if(n<=0) return;
        request.append(buffer,static_cast<size_t>(n));
    }
    auto command=trim(request.substr(body_start,length));
    if(!valid_command(command)) { respond(client,400,"text/plain","Unknown command or value outside allowed range"); return; }
    { std::lock_guard lock(state_.mutex); if(state_.commands.size()<64) state_.commands.push_back(command); }
    respond(client,200,"application/json","{\"ok\":true}");
}
}
