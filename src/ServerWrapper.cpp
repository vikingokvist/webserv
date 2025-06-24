#include "../includes/ServerWrapper.hpp"

ServerWrapper::ServerWrapper() {}


ServerWrapper::ServerWrapper(const ServerConfig& cfg) : config(&cfg) {}

ServerWrapper::ServerWrapper(const ServerWrapper& src) {*this = src;}

ServerWrapper& ServerWrapper::operator=(const ServerWrapper& src) {
    
    if (this != &src) {

        this->config = src.config;
        return (*this);
    }
    return (*this);
};

ServerWrapper::~ServerWrapper() {}


std::string             ServerWrapper::getIps(size_t ip_index) const {

    if (!config || ip_index >= config->ips_and_ports.size()) return ("");
    return (config->ips_and_ports[ip_index].first);
}


size_t                  ServerWrapper::getIpCount() const {

    return (config->ips_and_ports.size());
}


int                     ServerWrapper::getPorts(size_t port_index) const {

    if (!config || port_index >= config->ips_and_ports.size()) return (-1);
    return (config->ips_and_ports[port_index].second);
}


size_t                  ServerWrapper::getPortCount() const {

    if (!config) return (0);
    return (config->ips_and_ports.size());
}

std::string             ServerWrapper::getServerNames(size_t server_name_index) const {

    if (!config || server_name_index >= config->server_names.size()) return ("");
    return (config->server_names[server_name_index]);
}


size_t                  ServerWrapper::getServerNameCount() const {

    if (!config) return (0);
    return (config->server_names.size());
}



std::map<int, std::string> ServerWrapper::getAllErrorPages() const {
    if (!config)
        return std::map<int, std::string>();
    return (config->error_pages);
}


std::string             ServerWrapper::getErrorPages(int error_page_index) const {

    if (!config) return ("");
    std::map<int, std::string>::const_iterator it = config->error_pages.find(error_page_index);
    if (it != config->error_pages.end())
        return (it->second);
    return ("");
}


size_t                  ServerWrapper::getErrorPageCount() const {

    if (!config) return (0);
    return (config->error_pages.size());
}


size_t                  ServerWrapper::getClientMaxBodySize() const {

    if (!config) return (0);
    return (config->client_max_body_size);
}


const LocationConfig&   ServerWrapper::getLocation(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size())
        throw (std::out_of_range("Invalid location index"));

    return (config->locations[loc_index]);
}


size_t                  ServerWrapper::getLocationCount() const {

    if (!config) return (0);
    return (config->locations.size());
}


std::string             ServerWrapper::getLocationPath(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return ("");
    return (config->locations[loc_index].path);
}



std::string             ServerWrapper::getLocationRoot(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return ("");
    return (config->locations[loc_index].root);
}


std::string             ServerWrapper::getLocationIndex(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return ("");
    return (config->locations[loc_index].index);
}


bool                    ServerWrapper::getAutoIndex(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return (false);
    return (config->locations[loc_index].auto_index);
}

std::string            ServerWrapper::getRedirect(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return (0);
    return (config->locations[loc_index].redirect);
}

size_t             ServerWrapper::getRedirectCode(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return (0);
    return (config->locations[loc_index].redirect_code);
}

std::string ServerWrapper::getMethods(size_t loc_index, size_t method_index) const {
    
    if (!config || loc_index >= config->locations.size()) return "";
    const std::set<std::string>& methods = config->locations[loc_index].methods;

    if (method_index >= methods.size()) return "";
    std::set<std::string>::const_iterator it = methods.begin();
    std::advance(it, method_index);
    return *it;
}


std::string             ServerWrapper::getCgiExtensions(size_t loc_index, size_t cgi_extension_index) const {

    if (!config || loc_index >= config->locations.size()) return ("");
    const std::set<std::string>& extensions = config->locations[loc_index].cgi_extensions;

    if (cgi_extension_index >= extensions.size()) return "";
    std::set<std::string>::const_iterator it = extensions.begin();
    std::advance(it, cgi_extension_index);
    return (*it);
}


size_t                  ServerWrapper::getCgiExtensionCount(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return (0);
    return (config->locations[loc_index].cgi_extensions.size());
}


std::string             ServerWrapper::getUploadStore(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) return ("");
    return (config->locations[loc_index].upload_store);
}   

