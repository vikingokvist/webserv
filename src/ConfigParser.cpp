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

struct Vector4 {
    std::stack<std::string>             bracket_stack;
    std::string                         found_del;
    std::string                         token;
    ServerConfig                        cur_server;
};

void    ConfigParser::parseConfigFile(std::vector<std::string> config_array) {

    std::vector<std::string>::iterator  it = config_array.begin();
    std::stack<std::string>             bracket_stack;
    std::string                         found_del;
    std::string                         token;
    ServerConfig                        cur_server;

    for (; it != config_array.end(); ++it) {

        token = *it;
        handleBracketStack(bracket_stack, token, found_del, cur_server);
        if (found_del == "server") {

            if (token == "server")
                cur_server = ServerConfig();
            else if (token == "listen")
                ConfigParser::listenToken(token, config_array, it, cur_server);
            else if (token == "server_name")
                ConfigParser::serverNameToken(token, config_array, it, cur_server);
            else if (token == "client_max_body_size")
                ConfigParser::clientMaxBodySizeToken(token, config_array, it, cur_server);
            else if (token == "error_page")
                ConfigParser::errorPageToken(token, config_array, it, cur_server); 
        }
        else if (found_del == "location") {

            LocationConfig cur_loc;
            token = *it++;
            cur_loc.path = token;
            token = *it++;
            if (token == "root")
                ConfigParser::rootToken(token, config_array, it, cur_loc);
            if (token == "index")
                ConfigParser::indexToken(token, config_array, it, cur_loc);
            if (token == "methods")
                ConfigParser::methodsToken(token, config_array, it, cur_loc);
            if (token == "autoindex")
                ConfigParser::autoIndexToken(token, config_array, it, cur_loc);
            if (token == "return")
                ConfigParser::redirectToken(token, config_array, it, cur_loc);
            if (token == "cgi_ext")
                ConfigParser::cgiExtensionToken(token, config_array, it, cur_loc);
            if (token == "upload_store")
                ConfigParser::uploadStoreToken(token, config_array, it, cur_loc);
        }
        else
            _servers.push_back(cur_server);
    }
    if (!bracket_stack.empty())
        throw (ConfigParser::MisconfigurationException());
}

void    ConfigParser::rootToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    it++;
    if (it != config_array.end()) {

        token = *it;
        if (!token.empty() && token.back() == ';') {

            token.pop_back();
            cur_loc.root = token;
            token = *it++;
        }
    }
}

void    ConfigParser::indexToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    it++;
    if (it != config_array.end()) {
        token = *it;
        if (!token.empty() && token.back() == ';') {

            token.pop_back();
            cur_loc.index = token;
            token = *it++;
        }
    }
}

void    ConfigParser::methodsToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    bool    stop = false;
    it++;
    for (; it != config_array.end(); ++it) {

        token = *it;
        if (!token.empty() && token.back() == ';') {

            stop = true;
            token.pop_back();
        }
        cur_loc.methods.insert(token);
        if (stop == true)
            break ;
    }
}

void    ConfigParser::autoIndexToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    it++;
    if (it != config_array.end()) {
        token = *it;
        if (!token.empty() && token.back() == ';')
            token.pop_back();
        if (token == "on")
            cur_loc.auto_index = true;
        else if (token == "off")
            cur_loc.auto_index = false;
    }
}

void    ConfigParser::redirectToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    ++it;
    if (it != config_array.end()) {

        token = *it;
        cur_loc.redirect_code = std::atoi(token.c_str());
        ++it;
        if (it != config_array.end()) {

            token = *it;
            if (!token.empty() && token.back() == ';')
                token.pop_back();
            cur_loc.redirect = token;
        }
    } 
}
void    ConfigParser::cgiExtensionToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    it++;
    for (; it != config_array.end(); ++it) {
        token = *it;
        if (!token.empty()) {
            size_t semicolon_pos = token.find(';');
            if (semicolon_pos != std::string::npos)
                token.pop_back();
            cur_loc.cgi_extensions.insert(token);
            if (semicolon_pos != std::string::npos)
                break ;
        }
    }
}

void    ConfigParser::uploadStoreToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc) {

    it++;
    if (it != config_array.end()) {

        token = *it;
        if (!token.empty() && token.back() == ';') {

            token.pop_back();
            cur_loc.upload_store = token;
        }
    }
}


void    ConfigParser::errorPageToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server) {

    it++;
    if (it != config_array.end()) {

        token = *it;
        std::string temp = token;
        it++;
        token = *it;
        cur_server.error_pages[std::atoi(temp.c_str())] = token;
    }
}

void    ConfigParser::clientMaxBodySizeToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server) {

    size_t      len = token.size();
    char        c = token[len - 1];
    size_t      multiplier = 1;
    std::string number_part = token;
    size_t      numer = 0;

    it++;
    if (it != config_array.end()) {
        token = *it;
        if (!token.empty() && token.back() == ';')
            token.pop_back();
    }
    if (c == 'K' || c == 'k') {

        multiplier = 1024ULL;
        number_part = token.substr(0, len - 1);
    }
    else if (c == 'M' || c == 'm') {

        multiplier = 1024ULL * 1024ULL;
        number_part = token.substr(0, len - 1);
    }
    else if (c == 'G' || c == 'g') {

        multiplier = 1024ULL * 1024ULL * 1024ULL;
        number_part = token.substr(0, len - 1);
    }
    cur_server.client_max_body_size = std::stoul(number_part) * multiplier;
}


void    ConfigParser::serverNameToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server) {

    it++;
    for (; it != config_array.end(); ++it) {
        token = *it;
        if (!token.empty()) {
            size_t semicolon_pos = token.find(';');
            if (semicolon_pos != std::string::npos)
                token.pop_back();
            cur_server.server_names.push_back(token);
            if (semicolon_pos != std::string::npos)
                break ;
        }
    }
}

void    ConfigParser::listenToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server) {

    size_t  colon_pos;

    it++;
    if (it != config_array.end()) {

        token = *it;
        if (!token.empty() && token.back() == ';')
            token.pop_back();
        colon_pos = token.find(':');
        if (colon_pos != std::string::npos)
            cur_server.ips_and_ports.push_back(std::make_pair(token.substr(0, colon_pos), std::atoi(token.substr(colon_pos + 1).c_str())));
    }
}

void    ConfigParser::handleBracketStack(std::stack<std::string>& bracket_stack, std::string token, std::string& found_del, ServerConfig& cur_server) {

    if (token == "location" || token == "server") {

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
