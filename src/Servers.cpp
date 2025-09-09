#include "../includes/Servers.hpp"

Servers::Servers() {}

Servers::Servers(const std::string& filename) : parser(filename) {

    const std::vector<ServerConfig>& configs = parser.getServers();
    
    for (size_t i = 0; i < configs.size(); ++i) {

        servers.push_back(ServerWrapper(configs[i]));
    }
}

Servers::Servers(const Servers& src) {*this = src;}

Servers& Servers::operator=(const Servers& src) {(void)src; return (*this);};

Servers::~Servers() {

}


ServerWrapper& Servers::operator[](size_t index) {
    
    if (index >= servers.size()) {
        throw std::out_of_range("Invalid server index");
    }
    return servers[index];
}


const ServerWrapper& Servers::operator[](size_t index) const {

    if (index >= servers.size()) {
        throw std::out_of_range("Invalid server index");
    }
    return servers[index];
}


size_t  Servers::size() const {

    return (servers.size());
}