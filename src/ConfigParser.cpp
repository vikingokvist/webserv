#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string& filename) {

    ParserVariables vars;

    parseFile(filename, vars);
    parseConfigFile(vars);

    // printParsedConfig(this->getServers());
}

ConfigParser::ConfigParser(const ConfigParser& src) {*this = src;}

ConfigParser& ConfigParser::operator=(const ConfigParser& src) {if (this != &src) {this->_servers = src._servers;} return (*this);}

ConfigParser::~ConfigParser() {}


void    ConfigParser::parseFile(const std::string& filename, ParserVariables& vars) {

    std::ifstream   config_file(filename.c_str());
    std::string     line;
    std::string     buffer;

    if (!config_file.is_open())
        throw (FileOpenErrorException());
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
    vars.config_array =  cp_split(buffer, ' ');
}

void    ConfigParser::parseConfigFile(ParserVariables& vars) {

    vars.in_location = false;
    vars.in_server = false;
    for (vars.it = vars.config_array.begin(); vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        handleBracketStack(vars);
        if (vars.in_server == true  && vars.in_location == false) {

            if (vars.token == "listen")
                listenToken(vars);
            else if (vars.token == "server_name")
                serverNameToken(vars);
            else if (vars.token == "client_max_body_size")
                clientMaxBodySizeToken(vars);
            else if (vars.token == "error_page")
                errorPageToken(vars);
        }
        else if (vars.in_location == true) {

            if (vars.token == "root")
                rootToken(vars);
            else if (vars.token == "index")
                indexToken(vars);
            else if (vars.token == "methods")
                methodsToken(vars);
            else if (vars.token == "autoindex")
                autoIndexToken(vars);
            else if (vars.token == "return")
                redirectToken(vars);
            else if (vars.token == "cgi_ext")
                cgiExtensionToken(vars);
            else if (vars.token == "upload_store")
                uploadStoreToken(vars);
            else if (isMisconfiguredLocation(vars.token) == false) {
                std::cout << "\"" << vars.token << "\"" << " <= ";
                throw (MisconfigurationException());
            }
            
        }
        if (vars.token.find("}") != std::string::npos && vars.in_location == true) {

            vars.in_location = false;
            vars.cur_server.locations.push_back(vars.cur_loc);
        }
        else if (vars.token.find("}") != std::string::npos && vars.in_location == false && vars.in_server == true) {

            vars.in_server = false;
            this->_servers.push_back(vars.cur_server);
        }
    }
    if (vars.in_server == true || vars.in_location == true) {

        std::cout << "} <= Missing closing bracket " << std::endl;
        throw (MisconfigurationException());
    }
}

void ConfigParser::handleBracketStack(ParserVariables& vars) {

    if (vars.token.find("server") != std::string::npos && vars.token.find("server_name") == std::string::npos) {

        if (vars.in_server == true) {

            throw (MisconfigurationException());
        }
        vars.token = *(++vars.it);
        if (vars.token.find("{") != std::string::npos) {

            vars.cur_server = ServerConfig();
            vars.in_server = true;
            vars.token = *(++vars.it);
        }
    }
    else if (vars.token.find("location") != std::string::npos) {

        std::string temp = *(++vars.it);

        if (vars.in_location == true) {

            throw (MisconfigurationException());
        }
        vars.token = *(++vars.it);
        if (vars.token.find("{") != std::string::npos) {

            vars.cur_loc = LocationConfig();
            vars.in_location = true;
            vars.cur_loc.path = temp;
            vars.token = *(++vars.it);
        }
    }
}

void    ConfigParser::rootToken(ParserVariables& vars) {

    vars.it++;
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.root = vars.token;
        }
    }
}

void    ConfigParser::indexToken(ParserVariables& vars) {

    vars.it++;
    for (; vars.it != vars.config_array.end(); ) {
        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token.find('}') != std::string::npos)
            break ;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.indices.push_back(vars.token);
            vars.token = *(++vars.it);
        }
        else if (!vars.token.empty() && vars.token[vars.token.size() - 1] != ';') {
            vars.cur_loc.indices.push_back(vars.token);
            vars.token = *(++vars.it);
        }
    }
}

void    ConfigParser::methodsToken(ParserVariables& vars) {

    bool    stop = false;

    vars.it++;
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {

            stop = true;
            vars.token.erase(vars.token.size() - 1);
        }
        vars.cur_loc.methods.insert(vars.token);
        if (stop == true)
            break ;
    }
}

void    ConfigParser::autoIndexToken(ParserVariables& vars) {

    vars.it++;
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';')
            vars.token.erase(vars.token.size() - 1);
        if (vars.token == "on")
            vars.cur_loc.auto_index = true;
        else if (vars.token == "off")
            vars.cur_loc.auto_index = false;
    }
}

void    ConfigParser::redirectToken(ParserVariables& vars) {

    ++vars.it;
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        vars.cur_loc.redirect_code = std::atoi(vars.token.c_str());
        ++vars.it;
        if (vars.it != vars.config_array.end()) {

            vars.token = *vars.it;
            if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';')
                vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.redirect = vars.token;
        }
    } 
}
void    ConfigParser::cgiExtensionToken(ParserVariables& vars) {

    vars.it++;
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        if (!vars.token.empty()) {
            size_t semicolon_pos = vars.token.find(';');
            if (semicolon_pos != std::string::npos)
                vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.cgi_extensions.insert(vars.token);
            if (semicolon_pos != std::string::npos)
                break ;
        }
    }
    vars.token = *(++vars.it);
}

void    ConfigParser::uploadStoreToken(ParserVariables& vars) {

    vars.it++;
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.upload_store = vars.token;
            vars.token = *(++vars.it);
        }
    }
}


void    ConfigParser::errorPageToken(ParserVariables& vars) {

    vars.it++;
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        std::string temp = vars.token;
        vars.token = *(++vars.it);
        vars.cur_server.error_pages[std::atoi(temp.c_str())] = vars.token;
    }
}
void ConfigParser::clientMaxBodySizeToken(ParserVariables& vars) {

    vars.it++;
    if (vars.it == vars.config_array.end())
        return;
    vars.token = *vars.it;
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';')
        vars.token.erase(vars.token.size() - 1);
    size_t len = vars.token.size();
    char c = vars.token[len - 1];
    size_t multiplier = 1;
    std::string number_part = vars.token;

    if (c == 'K' || c == 'k') {
        multiplier = 1024ULL;
        number_part = vars.token.substr(0, len - 1);
    }
    else if (c == 'M' || c == 'm') {
        multiplier = 1024ULL * 1024ULL;
        number_part = vars.token.substr(0, len - 1);
    }
    else if (c == 'G' || c == 'g') {
        multiplier = 1024ULL * 1024ULL * 1024ULL;
        number_part = vars.token.substr(0, len - 1);
    }
    vars.cur_server.client_max_body_size = str_to_size_t(number_part) * multiplier;
}


void    ConfigParser::serverNameToken(ParserVariables& vars) {

    vars.it++;
    for (; vars.it != vars.config_array.end(); ++vars.it) {
        vars.token = *vars.it;
        if (!vars.token.empty()) {
            size_t semicolon_pos = vars.token.find(';');
            if (semicolon_pos != std::string::npos) {
                vars.token.erase(vars.token.size() - 1);
            }
            vars.cur_server.server_names.push_back(vars.token);
            if (semicolon_pos != std::string::npos)
                break ;
        }
    }
}

void    ConfigParser::listenToken(ParserVariables& vars) {

    size_t  colon_pos;

    vars.it++;
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';')
            vars.token.erase(vars.token.size() - 1);
        colon_pos = vars.token.find(':');
        if (colon_pos != std::string::npos)
            vars.cur_server.ips_and_ports.push_back(std::make_pair(vars.token.substr(0, colon_pos), std::atoi(vars.token.substr(colon_pos + 1).c_str())));
    }
}



std::string ConfigParser::trim(const std::string& str) {

    size_t start = str.find_first_not_of(" \t\n\r");
    size_t end = str.find_last_not_of(" \t\n\r");

    if (start == std::string::npos)
        return ("");
    return (str.substr(start, end - start + 1));
}

std::vector<std::string> ConfigParser::cp_split(const std::string& str, char delimiter) {

    std::vector<std::string>    tokens;
    std::string                 token;

    for (size_t i = 0; i < str.length(); ++i) {

        char ch = str[i];
        if (ch == delimiter || ch == '{') {

            if (!token.empty()) {

                tokens.push_back(token);
                token.clear();
            }
            if (ch == '{') {

                tokens.push_back("{"); 
            }
        }
        else {

            token += ch;
        }
    }
    if (!token.empty())
        tokens.push_back(token);
    return (tokens);
}


size_t      ConfigParser::str_to_size_t(const std::string& s) {

    std::istringstream iss(s);
    size_t result;

    iss >> result;
    return (result);
}

bool ConfigParser::isMisconfiguredLocation(std::string token) {

    if (token.find("upload_store") != std::string::npos)
        return (true);
    if (token.find("cgi_ext") != std::string::npos)
        return (true);
    if (token.find("return") != std::string::npos)
        return (true);
    if (token.find("autoindex") != std::string::npos)
        return (true);
    if (token.find("methods") != std::string::npos)
        return (true);
    if (token.find("root") != std::string::npos)
        return (true);
    if (token.find("index") != std::string::npos)
        return (true);
    return (false);
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {

    return (_servers);
}


const char* ConfigParser::MisconfigurationException::what() const throw() {

    return ("Misconfigured config file");
}

const char* ConfigParser::FileOpenErrorException::what() const throw() {

    return ("Failed opening config file");
}



void        ConfigParser::printParsedConfig(const std::vector<ServerConfig>& servers) {

    for (size_t i = 0; i < servers.size(); ++i) {
        const ServerConfig& server = servers[i];
        std::cout << "-----------------------------------------" << "Server #" << i + 1 << "\n";
    
        std::cout << "Server Names: ";
        for (size_t j = 0; j < server.server_names.size(); ++j)
            std::cout << server.server_names[j] << " ";
    
        std::cout << std::endl;
        for (size_t j = 0; j < server.ips_and_ports.size(); ++j)
            std::cout << "  IP:Port = " << server.ips_and_ports[j].first << ":" << server.ips_and_ports[j].second << "\n";
    
        std::cout << "  Client Max Body Size: " << server.client_max_body_size << "\n";
    
        std::cout << "  Error Pages:\n";
        for (std::map<int, std::string>::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
            std::cout << "    " << it->first << " => " << it->second << "\n";
    
        std::cout << "  Locations:\n";
        for (size_t k = 0; k < server.locations.size(); ++k) {
            const LocationConfig& loc = server.locations[k];
            std::cout << "    Location #" << k + 1 << ":\n";
            if (!loc.path.empty())
                std::cout << "      Path: " << loc.path << "\n";
            if (!loc.root.empty())
                std::cout << "      Root: " << loc.root << "\n";
            if (!loc.methods.empty()) {
                std::cout << "      Methods: ";
                for (std::set<std::string>::const_iterator it = loc.methods.begin(); it != loc.methods.end(); ++it)
                    std::cout << *it << " ";
                std::cout << "\n";
            }
    
            if (loc.auto_index == true || loc.auto_index == false)
                std::cout << "      Autoindex: " << (loc.auto_index ? "on" : "off") << "\n";
    
            if (!loc.redirect.empty())
                std::cout << "      Redirect: " << loc.redirect << "\n";
            if (loc.redirect_code != 0)  
                std::cout << "      Redirect Code: " << loc.redirect_code << "\n";
    
            if (!loc.cgi_extensions.empty()) {
                std::cout << "      CGI Extensions: ";
                for (std::set<std::string>::const_iterator it = loc.cgi_extensions.begin(); it != loc.cgi_extensions.end(); ++it)
                    std::cout << *it << " ";
                std::cout << "\n";
            }   
            if (!loc.upload_store.empty())
                std::cout << "      Upload Store: " << loc.upload_store << "\n";
        }
    
        std::cout << "----------------------\n";
    }
}