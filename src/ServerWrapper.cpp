#include "../includes/ServerWrapper.hpp"

ServerWrapper::ServerWrapper() {}


ServerWrapper::ServerWrapper(const ServerConfig* cfg) : config(cfg) {}

ServerWrapper::ServerWrapper(const ServerWrapper& src) {*this = src;}

ServerWrapper& ServerWrapper::operator=(const ServerWrapper& src) {(void)src; return (*this);};

ServerWrapper::~ServerWrapper() {}


std::string             ServerWrapper::getIps(size_t ip_index) const {

    if (!config || ip_index >= config->ips_and_ports.size()) return ("");
    return (config->ips_and_ports[ip_index].first);
}


size_t                  ServerWrapper::getIpSize() const {

    return (config->ips_and_ports.size());
}


int                     ServerWrapper::getPorts(size_t port_index) const {

    if (!config || port_index >= config->ips_and_ports.size()) return (-1);
    return (config->ips_and_ports[port_index].second);
}


size_t                  ServerWrapper::getPortSize() const {

    if (!config) return (0);
    return (config->ips_and_ports.size());
}



// struct ServerConfig
// {
//     std::vector<std::pair<std::string, int> >   ips_and_ports;
//     std::vector<std::string>                    server_names;
//     std::map<int, std::string>                  error_pages;
//     size_t                                      client_max_body_size;
//     std::vector<LocationConfig>                 locations;
// };
std::string             ServerWrapper::getServerNames(size_t server_name_index) const {

    if (!config || server_name_index >= config->server_names.size()) return ("");
    return (config->server_names[server_name_index]);
}


size_t                  ServerWrapper::getServerNameSize() const {

    if (!config) return (0);
    return (config->server_names.size());
}


std::string             ServerWrapper::getErrorPages(int error_page_index) const {

    if ()
}


int                     ServerWrapper::getErrorPageCode(size_t error_page_index) const {


}


size_t                  ServerWrapper::getErrorPageSize() const {


}


size_t                  ServerWrapper::getClientMaxBodySize() const {


}


const LocationConfig&   ServerWrapper::getLocation(size_t index) const {


}


size_t                  ServerWrapper::getLocationCount() const {


}


std::string             ServerWrapper::getLocationPath() const {


}


std::string             ServerWrapper::getLocationRoot() const {


}


std::string             ServerWrapper::getLocationIndex() const {


}


bool                    ServerWrapper::getIfRedirec() const {


}


std::string             ServerWrapper::getRedirectCode() const {


}


std::string             ServerWrapper::getCgiExtensions(size_t index) const {


}


size_t                  ServerWrapper::getCgiExtensionSize() const {


}


std::string             ServerWrapper::getUploadStore() const {


}   

