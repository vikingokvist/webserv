#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string& filename) {

    ConfigParser::parseFile(filename);
}

ConfigParser::ConfigParser(const ConfigParser& src) {
    
    *this = src;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& src) {

    if (this != &src) {

        this->_servers = src._servers;
    }
    return (*this);
}

ConfigParser::~ConfigParser() {}
// struct LocationConfig
// {
//     std::string path;
//     std::string root;
//     std::string index;
//     std::set<std::string> methods;
//     bool auto_index = false;
//     std::string redirect;
//     std::string cgi_extension;
//     std::string upload_store;
// };

// struct ServerConfig
// {
//     std::string host;
//     int         port;
//     std::vector<std::string> server_names;
//     std::map<int, std::string> error_pages;
//     size_t client_max_body_size = 1 * 1024 * 1024;
//     std::vector<LocationConfig> locations;
// };


std::string ConfigParser::trim(const std::string& str) {

    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    return (start == std::string::npos) ? "" : str.substr(start, end - start + 1);
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delim) {

    std::stringstream ss(str);
    std::string token;
    std::vector<std::string> tokens;
    while (std::getline(ss, token, delim)) {

        tokens.push_back(trim(token));
    }
    return tokens;
}

size_t      ConfigParser::convertValueToBytes(std::string line) {

    size_t multiplier = 1;
    char suffix = line.back();

    if (suffix == 'K' || suffix == 'k') {
        multiplier = 1024;
        line.pop_back();
    }
    else if (suffix == 'M' || suffix == 'm') {
        multiplier = 1024 * 1024;
        line.pop_back();
    }
    return (std::stoul(line) * multiplier);
}


void    ConfigParser::parseFile(const std::string& filename) {

    ServerConfig    server;
    LocationConfig  loc;
    BracketState    server_bracket = B_DEFAULT;
    std::ifstream   config_file(filename.c_str());
    std::string     line;
    size_t          pos;

    if (!config_file.is_open())
        throw (ConfigParser::FileOpenErrorException());


    while (std::getline(config_file, line)) {

        line = trim(line);
        if (line.empty() || line[0] == '#')
            continue ;
        if (line.find("server") != std::string::npos || server_bracket == B_OPEN) {
            while (line.find("{") != std::string::npos && std::getline(config_file, line))
                continue ;
            server_bracket = B_OPEN;
        }
        if (line.find("listen") == 0) {
            line = trim(line);
            line = line.substr(6);
            if (line.back() == ';')
                line.pop_back();
            std::vector<std::string> parts = split(line, ':');
            if (parts.size() == 2) {
                server.host = parts[0];
                server.port = std::stoi(parts[1]);
            }
            else if (parts.size() == 1) {
                server.host = "0.0.0.0";
                server.port = std::stoi(parts[0]);
            }
        }
        else if (line.find("server_name") == 0) {
            line = trim(line);
            line = line.substr(11);
            if (line.back() == ';')
                line.pop_back();

            std::stringstream ss(line);
            std::string name;
            while (ss >> name) {
                server.server_names.push_back(name);
            }
        }
        else if (line.find("client_max_body_size") == 0) {
            line = trim(line);
            line = line.substr(std::string("client_max_body_size").length());
            if (!line.empty() && line.back() == ';')
                line.pop_back();
            server.client_max_body_size = convertValueToBytes(line);
        }
        
    }

}
    



const char* ConfigParser::MisconfigurationException::what() const throw() {

    return ("Misconfigured config file");
}

const char* ConfigParser::FileOpenErrorException::what() const throw() {

    return ("Failed opening config file");
}
