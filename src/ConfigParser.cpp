#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string& filename) {

    std::string buffer = ConfigParser::parseFile(filename);
    ConfigParser::parseConfigFile(ConfigParser::split(buffer, ' '));
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


std::string    ConfigParser::parseFile(const std::string& filename) {

    std::ifstream   config_file(filename.c_str());
    std::string     line;
    std::string     buffer;

    if (!config_file.is_open())
        throw (ConfigParser::FileOpenErrorException());
    while (std::getline(config_file, line)) {

        line = trim(line);
        if (line.empty())
            continue ;
        size_t pos = line.find("#");
        if (pos != std::string::npos)
            line = line.substr(0, pos);
        line = trim(line);
        if (!line.empty())
            buffer += line + " ";
    }
    config_file.close();
    return (buffer);
}

/*struct LocationConfig
{
    std::string path;
    std::string root;
    std::string index;
    std::set<std::string> methods;
    bool auto_index = false;
    std::string redirect;
    std::string cgi_extension;
    std::string upload_store;
};

struct ServerConfig
{
    std::string host;
    int         port;
    std::vector<std::string> server_names;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size = 1 * 1024 * 1024;
    std::vector<LocationConfig> locations;
};
*/

// server {
//     listen 127.0.0.1:8080;                          # Bind to this IP and port
//     server_name mysite.com www.mysite.com;         # Accept requests for these hostnames

//     client_max_body_size 2M;                        # Limit on request body size (e.g. file uploads)
//     error_page 404 /errors/404.html;               # Custom error page for 404
//     error_page 500 /errors/500.html;        

void    ConfigParser::parseConfigFile(std::vector<std::string> config_array) {

    std::vector<std::string>::iterator  it = config_array.begin();
    std::stack<std::string>             bracket_stack;
    std::string                         found_del;
    std::string                         token;
    ServerConfig                        cur_server;

    for (; it != config_array.end(); ++it) {

        token = *it;
        handleBracketStack(bracket_stack, token, found_del, cur_server);
        if (token == "listen" && found_del == "server") {
            it++;
            if (it != config_array.end()) {
                token = *it;
                size_t  colon_pos = token.find(':');
                if (colon_pos != std::string::npos) {
                    cur_server.ips_and_ports.push_back(std::make_pair(token.substr(0, colon_pos), std::atoi(token.substr(colon_pos + 1).c_str())));
                }
            }
        }
        if (token == "server_name" && found_del == "server") {
            it++;
            for (; it != config_array.end(); ++it) {
                token = *it;
                if (!token.empty()) {
                    size_t semicolon_pos = token.find(';');
                    if (semicolon_pos != std::string::npos)
                        token = token.substr(0, semicolon_pos);
                    cur_server.server_names.push_back(token);
                    if (semicolon_pos != std::string::npos)
                        break ;
                }
            }
        }
        


    }
    if (!bracket_stack.empty())
        throw (ConfigParser::MisconfigurationException());
}

void    ConfigParser::handleBracketStack(std::stack<std::string>& bracket_stack, std::string token, std::string& found_del, ServerConfig& cur_server) {

    if (token == "server") {

        cur_server = ServerConfig();
        found_del = token;
    }
    if (token == "location") {

        found_del = token;
    }
    else if (token == "{" && found_del != "") {

        bracket_stack.push(found_del);
    }
    if (token == "}") {
        
        found_del = bracket_stack.top();
        bracket_stack.pop();
    }
}

std::string ConfigParser::trim(const std::string& str) {

    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos)
        return ("");
    return (str.substr(start, end - start + 1));
}

std::vector<std::string> ConfigParser::split(const std::string& str, char delimiter) {

    std::vector<std::string>    tokens;
    std::istringstream          stream(str);
    std::string                 token;

    while (std::getline(stream, token, delimiter)) {

        if (!token.empty())
            tokens.push_back(token);
    }
    return (tokens);
}


const char* ConfigParser::MisconfigurationException::what() const throw() {

    return ("Misconfigured config file");
}

const char* ConfigParser::FileOpenErrorException::what() const throw() {

    return ("Failed opening config file");
}
