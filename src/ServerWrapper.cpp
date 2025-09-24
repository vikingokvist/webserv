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


uint16_t                     ServerWrapper::getPorts(size_t port_index) const {

    if (!config || port_index >= config->ips_and_ports.size()) return (-1);
    return (config->ips_and_ports[port_index].second);
}


size_t                  ServerWrapper::getPortCount() const {

    if (!config) return (0);
    return (config->ips_and_ports.size());
}

std::string             ServerWrapper::getServerName(size_t server_name_index) const {

    if (!config || server_name_index >= config->server_names.size()) return ("");
    return (config->server_names[server_name_index]);
}

std::vector<std::string> ServerWrapper::getServerNamesList(void) const {

    return (config->server_names);
}

size_t                  ServerWrapper::getServerNameCount() const {

    if (!config) return (0);
    return (config->server_names.size());
}

uint16_t            ServerWrapper::getCountIpPorts () {
    
    return (config->ips_and_ports.size());
}

std::string             ServerWrapper::getErrorRoot(int error_page_index) const {

    if (!config) return ("");
    std::map<int, std::pair<std::string, std::string> >::const_iterator it;
    it = config->error_pages.find(error_page_index);
    if (it != config->error_pages.end())
        return (it->second.first);
    return ("");
}

std::string             ServerWrapper::getErrorFile(int error_page_index) const {

    if (!config) return ("");
    std::map<int, std::pair<std::string, std::string> >::const_iterator it;
    it = config->error_pages.find(error_page_index);
    if (it != config->error_pages.end())
        return (it->second.second);
    return ("");
}

size_t                  ServerWrapper::getErrorPageCount() const {

    if (!config) return (0);
    return (config->error_pages.size());
}


unsigned long                  ServerWrapper::getClientMaxBodySize() const {

    if (!config) return (0);
    return (config->client_max_body_size);
}


size_t                              ServerWrapper::getDefaultIndexCount() const {

    if (!config || config->default_indices.empty())
        return (0);
    return (config->default_indices.size());
}

std::string                         ServerWrapper::getDefaultIndexFile(size_t index_file) const {

    if (!config || index_file >= config->default_indices.size()) 
        return ("");

    const std::vector<std::string>& indices = config->default_indices;

    return (indices[index_file]);
}

std::string                         ServerWrapper::getDefaultRoot() const {

    if (!config || config->default_root.empty()) return ("");
    return (config->default_root);
}

const std::vector<LocationConfig>& ServerWrapper::getLocations() const {

    if (!config)
        throw std::out_of_range("Invalid location index");
    return (config->locations);
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

    if (!config || loc_index >= config->locations.size() || config->locations[loc_index].root.empty()) return ("");
    return (config->locations[loc_index].root);
}

size_t ServerWrapper::getLocationIndexCount(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size()) 
        return (0);

    return (config->locations[loc_index].indices.size());
}

std::string ServerWrapper::getLocationIndexFile(size_t loc_index, size_t index_file) const {

    if (!config || loc_index >= config->locations.size()) 
        return ("");

    const std::vector<std::string>& indices = config->locations[loc_index].indices;

    if (index_file >= indices.size()) 
        return ("");

    return (indices[index_file]);
}

const std::vector<std::string>&      ServerWrapper::getDefaultIndices() const {
    
    return (config->default_indices);
}

const std::vector<std::string>&     ServerWrapper::getLocationIndices(size_t loc_index) const {

    return (config->locations[loc_index].indices);
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

size_t                ServerWrapper::getMethodsSize(size_t loc_index) const {

    if (!config || loc_index >= config->locations.size() || config->locations[loc_index].methods.empty())
        return (0);
    return (config->locations[loc_index].methods.size());
}

std::set<std::string>  ServerWrapper::getMethods(size_t loc_index) const {

    static std::set<std::string> methods;
    if (!config || loc_index >= config->locations.size())
        return (methods);
    methods = config->locations[loc_index].methods;
    return (methods);
}

std::string             ServerWrapper::getMethod(size_t loc_index, size_t method_index) const {
    
    if (!config || loc_index >= config->locations.size()) return ("");
    const std::set<std::string>& methods = config->locations[loc_index].methods;

    if (method_index >= methods.size()) return ("");
    std::set<std::string>::const_iterator it = methods.begin();
    std::advance(it, method_index);
    return (*it);
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


int                 ServerWrapper::getFD() const {

    return (this->_fd);
}

void                ServerWrapper::setSocket(int _fd) {

	this->_fd = _fd;
}

void                ServerWrapper::setServerName(const std::vector<std::string>& _server_name) {
    
    this->_server_name = _server_name;
}

void                ServerWrapper::setPort(uint16_t _port) {

	this->_port = _port;
}

void                ServerWrapper::setHost(in_addr_t _host) {

	this->_host = _host;
}

void                ServerWrapper::setSinFamily(sa_family_t _sin_family) {

	this->_sin_family = _sin_family;
}

void                ServerWrapper::setMaxClientSize(unsigned long _client_max_body_size) {

	this->_client_max_body_size = _client_max_body_size;
}

void                ServerWrapper::setLocations(const std::vector<LocationConfig>& _locations) {

	this->_locations = _locations;
}


int                 ServerWrapper::getSocket() const {

    return (this->_fd);
}

const std::vector<std::string>& ServerWrapper::getServerName() const {

    return (this->_server_name);
}

uint16_t            ServerWrapper::getPort() const {

    return (this->_port);
}

in_addr_t           ServerWrapper::getHost() const {

    return (this->_host);
}

sa_family_t         ServerWrapper::getSinFamily() const {

    return (this->_sin_family);
}

unsigned long      ServerWrapper::getMaxClientSize() const {
    
    return (this->_client_max_body_size);
}

struct sockaddr_in* ServerWrapper::getSockAddr() {
    
    return(&_server_adress);
}

void                ServerWrapper::setupSocket() {

    this->_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_fd == -1) {

		std::cerr << "Failed to create socket. errno: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
	int opt = 1;
	if (setsockopt(this->_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {

		std::cerr << "setsockopt(SO_REUSEADDR) failed. errno: " << errno << std::endl;
		close(this->_fd);
		exit(EXIT_FAILURE);
	}
}

void            ServerWrapper::setupSockAddr() {

	_server_adress.sin_addr.s_addr = getHost();
	_server_adress.sin_family = getSinFamily();
	_server_adress.sin_port = getPort();
}

void ServerWrapper::bindAndListen() {

	if (bind(this->_fd, (struct sockaddr*)&_server_adress, sizeof(struct sockaddr_in)) < 0) {

		std::cerr << "Failed to bind to port " << ntohs(_server_adress.sin_port)
		          << ". errno: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
	if (listen(this->_fd, this->_client_max_body_size) < 0) {

		std::cerr << "Failed to listen on socket. errno: " << errno << std::endl;
		exit(EXIT_FAILURE);
	}
    else {

		std::cout << "Waiting for connection..." << std::endl;
	}
}

void            ServerWrapper::setupServerConfig(const std::vector<std::string>& _server_name, uint16_t _port, in_addr_t _host,
	sa_family_t _sin_family, unsigned long _client_max_body_size) {

	setServerName(_server_name);
	setPort(_port);
	setHost(_host);
	setSinFamily(_sin_family);
	setMaxClientSize(_client_max_body_size);
	setupSockAddr();
}

