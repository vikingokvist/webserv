#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string& filename) {

    ParserVariables vars;

    parseFile(filename, vars);
    parseConfigFile(vars);

}

ConfigParser::ConfigParser(const ConfigParser& src) {*this = src;}

ConfigParser& ConfigParser::operator=(const ConfigParser& src) {if (this != &src) {this->_servers = src._servers;} return (*this);}

ConfigParser::~ConfigParser() {}

void    ConfigParser::parseFile(const std::string& filename, ParserVariables& vars) {

    std::ifstream   config_file(filename.c_str());
    std::string     line;
    std::string     buffer;

    if (!config_file.is_open())
        throw (FileOpenErrorException(filename.c_str()));
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

    vars.cur_server_index = -1;
    vars.cur_loc_index = -1;
    vars.in_location = false;
    vars.in_server = false;
    for (vars.it = vars.config_array.begin(); vars.it != vars.config_array.end() ; ++vars.it) {

        vars.token = *vars.it;
        handleOpenBracket(vars);
        if (vars.in_server == true  && vars.in_location == false) {

            if (!isMisconfiguredServer(vars))
                throw (UnknownVariableException(vars.token, "server"));
            else if (vars.token.find("listen") != std::string::npos)
                listenToken(vars);
            else if (vars.token.find("server_name") != std::string::npos)
                serverNameToken(vars);
            else if (vars.token.find("client_max_body_size") != std::string::npos)
                clientMaxBodySizeToken(vars);
            else if (vars.token.find("error_page") != std::string::npos)
                errorPageToken(vars);
            else if (vars.token.find("root") != std::string::npos)
                defaultServerRoot(vars);
            else if (vars.token.find("index") != std::string::npos && vars.token.find("autoindex") == std::string::npos)
                defaultServerIndex(vars);
        }
        else if (vars.in_location == true) {

            if (!isMisconfiguredLocation(vars))
                throw (UnknownVariableException(vars.token, "location " + vars.cur_loc.path));
            else if (vars.token.find("root") != std::string::npos)
                rootToken(vars);
            else if (vars.token.find("index") != std::string::npos && vars.token.find("autoindex") == std::string::npos)
                indexToken(vars);
            else if (vars.token.find("methods") != std::string::npos)
                methodsToken(vars);
            else if (vars.token.find("autoindex") != std::string::npos)
                autoIndexToken(vars);
            else if (vars.token.find("return") != std::string::npos)
                redirectToken(vars);
            else if (vars.token.find("cgi_ext") != std::string::npos)
                cgiExtensionToken(vars);
            else if (vars.token.find("upload_store") != std::string::npos)
                uploadStoreToken(vars);
        }
        handleClosedBracket(vars);

    }
    if (vars.in_server || vars.in_location || vars.cur_server.bracket_state != 0 || vars.cur_loc.bracket_state != 0)
        throw MissingClosingBracketException("server");
}

void    ConfigParser::handleOpenBracket(ParserVariables& vars) {

    if (vars.token == "{")
        throw (ExtraOpenBracketException("server")); 
    if (vars.token == "server") {

        if (vars.in_server == true)
            throw (MisconfigurationException("server"));
        vars.token = *(++vars.it);
        if (vars.token == "{") {
            vars.cur_server_index++;
            vars.cur_server = ServerConfig();
            vars.cur_server.bracket_state = 1;
            vars.in_server = true;
            vars.cur_loc_index = -1;
            vars.token = *(++vars.it);
        }
    }
    else if (vars.token != "server_name" && vars.token.find("server") != std::string::npos)
        throw (MisconfigurationException(vars.token));
    else if (vars.token == "location") {

        std::string temp = *(++vars.it);

        if (temp == vars.cur_loc.path)
            throw (DuplicateLocationException(temp, "location " + vars.cur_loc.path));
        if (vars.in_location == true || temp == "{")
            throw (MisconfigurationException("location " + vars.cur_loc.path));
        vars.token = *(++vars.it);
        if (vars.token == "{") {
            vars.cur_loc = LocationConfig();
            vars.cur_loc_index++;
            vars.in_location = true;
            vars.cur_loc.path = temp;
            vars.cur_loc.bracket_state  = 1;
            vars.cur_loc.has_redirect = false;
            vars.token = *(++vars.it);
        }
    }
    else if (vars.token.find("location") != std::string::npos && vars.in_location == false)
        throw (MisconfigurationException(vars.token));
}

void    ConfigParser::handleClosedBracket(ParserVariables& vars) {

    if (vars.token == "}") {

        if (vars.in_location == true && vars.cur_loc.bracket_state == 1) {
            vars.in_location = false;
            vars.cur_loc.bracket_state  = 0;
            vars.cur_server.locations.push_back(vars.cur_loc);
        }
        else if (vars.in_server == true && vars.cur_server.bracket_state == 1) {
            vars.in_server = false;
            vars.cur_server.bracket_state  = 0;
            this->_servers.push_back(vars.cur_server);
        }
        else
            throw (ExtraClosingBracketException("server"));
    }
}

void    ConfigParser::listenToken(ParserVariables& vars) {

    size_t  colon_pos;

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "server"));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("server"));   
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos)
            vars.token.erase(vars.token.size() - 1);
        else
            throw (MissingClosingBracketException("server"));   
        colon_pos = vars.token.find(':');
        std::string ip = vars.token.substr(0, colon_pos);
        std::string port_str = vars.token.substr(colon_pos + 1);
        int port = std::atoi(port_str.c_str());
        for (size_t i = 0; i < vars.cur_server.ips_and_ports.size(); i++)
            if (vars.cur_server.ips_and_ports[i].first == ip && vars.cur_server.ips_and_ports[i].second == port)
                throw (DuplicateVariablesException(ip + ":" + port_str, "server " + temp_var));
        if (colon_pos != std::string::npos && vars.token[colon_pos + 1] && isdigit(vars.token[colon_pos + 1]))
            vars.cur_server.ips_and_ports.push_back(std::make_pair(ip, port));
        else
            throw (MissingPortException(temp_var, "server"));
    }
}

void    ConfigParser::serverNameToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_server.server_names.empty())
        throw (DuplicateVariablesException(temp_var, "server"));
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "server"));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("server"));   
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        if (isMisconfiguredServer(vars))
            throw (MissingClosingSemicolonException(temp_var, "server"));
        if (!vars.token.empty()) {

            size_t semicolon_pos = vars.token.find(';');
            if (semicolon_pos != std::string::npos)
                vars.token.erase(vars.token.size() - 1);
            vars.cur_server.server_names.push_back(vars.token);
            if (semicolon_pos != std::string::npos)
                break ;
        }
    }
}

void     ConfigParser::clientMaxBodySizeToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    
    if (!vars.cur_server.has_client_max_body_size.empty())
        throw (DuplicateVariablesException(temp_var, "server"));
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "server"));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("server")); 
    vars.token = *vars.it;
    size_t pos = vars.token.find(';');
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos)
        vars.token.erase(vars.token.size() - 1);
    else
        throw (MissingClosingSemicolonException(temp_var, "server")); 
    size_t len = vars.token.size();
    char c = vars.token[len - 1];
    std::string number_part = vars.token;
    uint64_t multiplier = 1ULL;

    if ((c == 'K' || c == 'k') && isdigit(vars.token[len - 2])) {
        multiplier = 1024ULL;
        number_part = vars.token.substr(0, len - 1);
    }
    else if ((c == 'M' || c == 'm') && isdigit(vars.token[len - 2])) {
        multiplier = 1024ULL * 1024ULL;
        number_part = vars.token.substr(0, len - 1);
    }
    else if ((c == 'G' || c == 'g') && isdigit(vars.token[len - 2])) {
        multiplier = 1024ULL * 1024ULL * 1024ULL;
        number_part = vars.token.substr(0, len - 1);
    }
    else
        throw (ClientMaxBodySizeValueException(temp_var, "server"));
    vars.cur_server.client_max_body_size = str_to_unsigned_long(number_part) * multiplier;
    if (vars.cur_server.client_max_body_size > MAX_SIZE)
        throw (ClientMaxBodySizeException(temp_var, "server"));
    vars.cur_server.has_client_max_body_size = "true";
}

void    ConfigParser::errorPageToken(ParserVariables& vars) {
    
    int     error_code;
    size_t  pos;

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "server"));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("server"));
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        for (size_t i = 0; i < vars.token.size(); i++) {
            if (!isdigit(vars.token[i])) {
                if (vars.token[i] == ';')
                    throw (MissingErrorCodePage(vars.token, "server"));
                throw (ErrorCodeMisconfiguration(vars.token, "server"));
            }
        }
        error_code = std::atoi(vars.token.c_str());
        std::string error_code_str = " " + vars.token;
        vars.it++;
        if (vars.it == vars.config_array.end() || (*vars.it)[0] == ';')
            throw (MissingErrorCodePage(temp_var, "server"));
        vars.token = *vars.it;
        pos = vars.token.find(';');
        if (!vars.token.empty() && pos != std::string::npos)
            vars.token.erase(vars.token.size() - 1);
        else
            throw (MissingClosingSemicolonException(temp_var, "server")); 
        pos = vars.token.find_last_of('/');
        if (vars.cur_server.error_pages.find(error_code) != vars.cur_server.error_pages.end())
            throw DuplicateVariablesException(temp_var + error_code_str, "server");
        if (pos != std::string::npos) {
            std::string root = vars.token.substr(0, pos + 1);
            std::string file = vars.token.substr(pos + 1);
            vars.cur_server.error_pages[error_code] = std::make_pair(root, file);
        } else {
            vars.cur_server.error_pages[error_code] = std::make_pair("", vars.token);
        }
    }
}

void    ConfigParser::defaultServerRoot(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_server.default_root.empty())
        throw DuplicateVariablesException(temp_var, "server");
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "server"));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("server"));
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_server.default_root = vars.token;
        }
        else
            throw (MissingClosingSemicolonException(temp_var, "server"));   
    }
}

void    ConfigParser::defaultServerIndex(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_server.default_indices.empty())
        throw DuplicateVariablesException(temp_var, "server");
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "server"));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("server"));   
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        size_t found_semicolon = vars.token.find(';');

        if (isMisconfiguredServer(vars) == true)
            throw (MissingClosingSemicolonException(temp_var, "server"));   
        if (!vars.token.empty() && found_semicolon != std::string::npos) {

            std::string temp = vars.token.substr(0, found_semicolon);
            vars.cur_server.default_indices.push_back(temp);
            break ;
        }
        vars.cur_server.default_indices.push_back(vars.token);
    }
}

void    ConfigParser::rootToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_loc.root.empty())
        throw (DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path));
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.root = vars.token;
        }
        else {
            if (isMisconfiguredLocation(vars) == false)
                throw (ExtraVariablesException(temp_var, "location " + vars.cur_loc.path));   
            throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path));   
        }
    }
}

void    ConfigParser::indexToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_loc.indices.empty())
        throw (DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path));
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        size_t found_semicolon = vars.token.find(';');

        if (isMisconfiguredLocation(vars) == true && found_semicolon == std::string::npos)
            throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path));   
        if (!vars.token.empty() && found_semicolon != std::string::npos) {

            std::string temp = vars.token.substr(0, found_semicolon);
            vars.cur_loc.indices.push_back(temp);
            break ;
        }
        vars.cur_loc.indices.push_back(vars.token);
    }
}

void    ConfigParser::methodsToken(ParserVariables& vars) {

    size_t pos;
    bool found = false;

    std::string temp_var = *vars.it;
    if (!vars.cur_loc.methods.empty())
        throw DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path);
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        found = false;
        pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {
            vars.token.erase(vars.token.size() - 1);
            found = true;
        }
        if (vars.token != "POST" && vars.token != "GET" && vars.token != "DELETE" && vars.token != "HEAD")
            throw (UnknownVariableValueException(temp_var, "location " + vars.cur_loc.path));
        if (isMisconfiguredLocation(vars) == true)
            throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path));  
        vars.cur_loc.methods.insert(vars.token);
        if (found == true)
            break ;
    }
}

void    ConfigParser::autoIndexToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_loc.auto_index_str.empty())
        throw (DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path));
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');

        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {
            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.auto_index_str = "auto_index";
            if (vars.token == "on")
                vars.cur_loc.auto_index = true;
            else if (vars.token == "off")
                vars.cur_loc.auto_index = false;
            else
                throw (UnknownVariableValueException(temp_var, "location " + vars.cur_loc.path));
            return ;
        }
        if (isMisconfiguredLocation(vars) == false && vars.token[0] == ';')
            throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path));
        throw (ExtraVariablesException(temp_var, "location " + vars.cur_loc.path));
    }
}

void    ConfigParser::redirectToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    vars.cur_loc.has_redirect = true;
    if (!vars.cur_loc.redirect_url.empty())
        throw DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path);
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    vars.token = *vars.it;
    size_t n = (vars.token[vars.token.size() - 1] == ';') ? 1 : 0;
    for (size_t i = 0; i < vars.token.size() - n; i++) {
        if (!std::isdigit(vars.token[i]))
            throw (MissingRedirectCodeException(temp_var, "location " + vars.cur_loc.path));
    }
    vars.cur_loc.redirect_code = std::atoi(vars.token.c_str());
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';')
        return ;
    ++vars.it;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path)); 
    vars.token = *vars.it;
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {
        vars.token.erase(vars.token.size() - 1);
        if (!vars.token.empty())
            vars.cur_loc.redirect_url = vars.token;
        return ;
    }
    throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path)); 
}

void    ConfigParser::cgiExtensionToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_loc.cgi_extensions.empty())
        throw DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path);
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        if (isMisconfiguredLocation(vars) == true)
            throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path));
        size_t semicolon_pos = vars.token.find(';');
        if (semicolon_pos != std::string::npos) {
            vars.token.erase(vars.token.size() - 1);
            if (!vars.token.empty())
                vars.cur_loc.cgi_extensions.insert(vars.token);
            break ;
        }
        vars.cur_loc.cgi_extensions.insert(vars.token);
    }
}

void    ConfigParser::uploadStoreToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;

    if (!vars.cur_loc.upload_store.empty())
        throw DuplicateVariablesException(temp_var, "location " + vars.cur_loc.path);
    if (temp_var.find(';') != std::string::npos)
        throw (MissingValueException(temp_var, "location " + vars.cur_loc.path));        
    vars.it++;
    if (vars.it == vars.config_array.end())
        throw (MissingClosingBracketException("location " + vars.cur_loc.path));   
    vars.token = *vars.it;
    size_t pos = vars.token.find(';');
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {
        
            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.upload_store = vars.token;
    }
    else
        throw (MissingClosingSemicolonException(temp_var, "location " + vars.cur_loc.path));
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


uint64_t    ConfigParser::str_to_unsigned_long(const std::string &s) {

    char *endptr = NULL;
    errno = 0;

    uint64_t val = strtoull(s.c_str(), &endptr, 10);

    if (errno == ERANGE)
        throw std::out_of_range("value too large");
    if (*endptr != '\0')
        throw std::invalid_argument("non-numeric characters in input");

    return (val);
}

bool ConfigParser::isMisconfiguredLocation(ParserVariables& vars) {

    std::vector<std::string>::iterator next_it = vars.it;
    
    if (next_it == vars.config_array.end())
        return (true);

    std::string token = *next_it;

    if (token == "}" || token == "{")
        return (true);
    if (token == "upload_store" || token == "upload_store;")
        return (true);
    if (token == "cgi_ext" || token == "cgi_ext;")
        return (true);
    if (token == "return" || token == "return;")
        return (true);
    if (token == "autoindex" || token == "autoindex;")
        return (true);
    if (token == "methods" || token == "methods;")
        return (true);
    if (token == "root" || token == "root;")
        return (true);
    if (token == "index" || token == "index;")
        return (true);

    return (false); 
}

bool ConfigParser::isMisconfiguredServer(ParserVariables& vars) {

    std::vector<std::string>::iterator next_it = vars.it;

    if (next_it == vars.config_array.end())
        return (true);

    std::string token = *next_it;

    if (token == "}" || token == "{")
        return (true);
    if (token == "server_name" || token == "server_name;")
        return (true);
    if (token == "error_page" || token == "error_page;")
        return (true);
    if (token == "client_max_body_size" || token == "client_max_body_size;")
        return (true);
    if (token == "listen" || token == "listen;")
        return (true);
    if (token == "root" || token == "root;")
        return (true);
    if (token == "index" || token == "index;")
        return (true);

    return (false);
}

const std::vector<ServerConfig>& ConfigParser::getServers() const {

    return (_servers);
}