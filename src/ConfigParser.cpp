#include "../includes/ConfigParser.hpp"

ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string& filename) {

    ParserVariables vars;

    parseFile(filename, vars);
    parseConfigFile(vars);
    
    // descomentar para imprimar datos
    // printParsedConfig(_servers);
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

    vars.cur_server_index = -1;
    vars.in_location = false;
    vars.in_server = false;
    for (vars.it = vars.config_array.begin(); vars.it != vars.config_array.end() ; ++vars.it) {

        vars.token = *vars.it;
        handleBracketStack(vars);
        if (vars.in_server == true  && vars.in_location == false) {

            if (!isMisconfiguredServer(vars)) {
                std::cout << "[server] => " << "\"" << vars.token << "\"" << " <= ";
                throw (UnknownVariableException());
            }
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

            if (!isMisconfiguredLocation(vars)) {
                std::cout << "[location " << vars.cur_loc.path << "] => \"" << vars.token << "\"" << " <= ";
                throw (UnknownVariableException());
            }
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
        throw (MissingClosingBracketException());
    }
}

void    ConfigParser::handleBracketStack(ParserVariables& vars) {

    if (vars.token.find("server") != std::string::npos && vars.token.find("server_name") == std::string::npos) {

        if (vars.in_server == true) {
            std::cout << "Unknown directive inside server[" << vars.cur_server_index << "] => \"server\" <=";
            throw (MisconfigurationException());
        }
        vars.token = *(++vars.it);
        if (vars.token.find("{") != std::string::npos) {
            vars.cur_server_index++;
            vars.cur_server = ServerConfig();
            vars.in_server = true;
            vars.token = *(++vars.it);
        }
    }
    else if (vars.token.find("location") != std::string::npos) {

        std::string temp = *(++vars.it);

        if (vars.in_location == true) {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << vars.token << "\"" << " <= ";
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

void    ConfigParser::listenToken(ParserVariables& vars) {

    size_t  colon_pos;

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos)
            vars.token.erase(vars.token.size() - 1);
        else {
            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingClosingBracketException());   
        }
        colon_pos = vars.token.find(':');
        if (colon_pos != std::string::npos && vars.token[colon_pos + 1] && isdigit(vars.token[colon_pos + 1]))
            vars.cur_server.ips_and_ports.push_back(std::make_pair(vars.token.substr(0, colon_pos), std::atoi(vars.token.substr(colon_pos + 1).c_str())));
        else {
            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingPortException());
        }
    }
}

void    ConfigParser::serverNameToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        if (isMisconfiguredServer(vars)) {
            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());
        }
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
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException()); 
    }
    vars.token = *vars.it;
    size_t pos = vars.token.find(';');
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos)
        vars.token.erase(vars.token.size() - 1);
    else {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingSemicolonException()); 
    }
    size_t len = vars.token.size();
    char c = vars.token[len - 1];
    unsigned long multiplier = 1;
    std::string number_part = vars.token;

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
    else {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (UnknownVariableValueException());
    }
    vars.cur_server.client_max_body_size = str_to_unsigned_long(number_part) * multiplier;
}

void    ConfigParser::errorPageToken(ParserVariables& vars) {
    
    int     error_code;
    size_t  pos;

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());
    }
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        for (size_t i = 0; i < vars.token.size(); i++) {
            if (!isdigit(vars.token[i])) {
                if (vars.token[i] == ';') {
                    std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
                    throw (MissingErrorCodePage());
                }
                std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
                throw (ErrorCodeMisconfiguration());
            }
        }
        error_code = std::atoi(vars.token.c_str());
        vars.it++;
        if (vars.it == vars.config_array.end() || (*vars.it)[0] == ';') {
            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingErrorCodePage());
        }
        vars.token = *vars.it;
        pos = vars.token.find(';');
        if (!vars.token.empty() && pos != std::string::npos)
            vars.token.erase(vars.token.size() - 1);
        else {
            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException()); 
        }
        pos = vars.token.find_last_of('/');
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
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());
    }
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_server.default_root = vars.token;
        }
        else {
            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());   
        }
    }
}

void    ConfigParser::defaultServerIndex(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        size_t found_semicolon = vars.token.find(';');

        if (isMisconfiguredServer(vars) == true) {

            std::cout << "[server] => " << "\"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());   
        }
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
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {

            vars.token.erase(vars.token.size() - 1);
            vars.cur_loc.root = vars.token;
        }
        else {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            if (isMisconfiguredLocation(vars) == false) {
                throw (ExtraVariablesException());   
            }
            throw (MissingClosingSemicolonException());   
        }
    }
}

void    ConfigParser::indexToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        size_t found_semicolon = vars.token.find(';');

        if (isMisconfiguredLocation(vars) == true && found_semicolon == std::string::npos) {

            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());   
        }
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
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        found = false;
        pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {
            vars.token.erase(vars.token.size() - 1);
            found = true;
        }
        if (vars.token != "POST" && vars.token != "GET" && vars.token != "DELETE") {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            throw (UnknownVariableValueException());
            break ;
        }
        if (isMisconfiguredLocation(vars) == true) {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());  
        }
        vars.cur_loc.methods.insert(vars.token);
        if (found == true)
            break ;
    }
}

void    ConfigParser::autoIndexToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');

        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {
            vars.token.erase(vars.token.size() - 1);
            if (vars.token == "on")
                vars.cur_loc.auto_index = true;
            else if (vars.token == "off")
                vars.cur_loc.auto_index = false;
            else {
                std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
                throw (UnknownVariableValueException());
            }
            return ;
        }
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        if (isMisconfiguredLocation(vars) == false && vars.token[0] == ';')
            throw (MissingClosingSemicolonException());
        throw (ExtraVariablesException());
    }
}

void    ConfigParser::redirectToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    vars.token = *vars.it;
    if (std::isdigit(vars.token[0])) {

        vars.cur_loc.redirect_code = std::atoi(vars.token.c_str());
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';')
            return ;
        ++vars.it;
        if (vars.it == vars.config_array.end()) {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            throw (MissingClosingBracketException()); 
        }
        vars.token = *vars.it;
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {
            vars.token.erase(vars.token.size() - 1);
            if (!vars.token.empty())
                vars.cur_loc.redirect = vars.token;
            return ;
        }
        return ;
    }
    vars.cur_loc.redirect_code = 302;
    if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';') {
        vars.token.erase(vars.token.size() - 1);
        vars.cur_loc.redirect = vars.token;
        return ;
    }
    std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
    throw (MissingClosingSemicolonException()); 
}

void    ConfigParser::cgiExtensionToken(ParserVariables& vars) {

    std::string temp_var = *vars.it;
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    for (; vars.it != vars.config_array.end(); ++vars.it) {

        vars.token = *vars.it;
        if (isMisconfiguredLocation(vars) == true) {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());
        }
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
    if (temp_var.find(';') != std::string::npos) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingValueException());        
    }
    vars.it++;
    if (vars.it == vars.config_array.end()) {
        std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
        throw (MissingClosingBracketException());   
    }
    if (vars.it != vars.config_array.end()) {

        vars.token = *vars.it;
        size_t pos = vars.token.find(';');
        if (!vars.token.empty() && vars.token[vars.token.size() - 1] == ';' && pos != std::string::npos) {
                vars.token.erase(vars.token.size() - 1);
                vars.cur_loc.upload_store = vars.token;
                vars.token = *(++vars.it);
        }
        else {
            std::cout << "[location " << vars.cur_loc.path << "] => \"" << temp_var << "\"" << " <= ";
            throw (MissingClosingSemicolonException());
        }
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


unsigned long      ConfigParser::str_to_unsigned_long(const std::string& s) {

    std::stringstream iss(s);
    unsigned long result;

    iss >> result;
    return (result);
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

const char* ConfigParser::MissingValueException::what() const throw() {

    return ("\033[1;31m Missing variable value.\033[0m");
}

const char* ConfigParser::MissingPortException::what() const throw() {

    return ("\033[1;31m Missing port.\033[0m");
}

const char* ConfigParser::UnknownVariableException::what() const throw() {

    return ("\033[1;31m Unknown variable.\033[0m");
}

const char* ConfigParser::ExtraVariablesException::what() const throw() {

    return ("\033[1;31m Extra variables not supported.\033[0m");
}

const char* ConfigParser::UnknownVariableValueException::what() const throw() {

    return ("\033[1;31m Unknown variable value.\033[0m");
}

const char* ConfigParser::MissingClosingBracketException::what() const throw() {

    return ("\033[1;31m Missing closing bracket.\033[0m");
}

const char* ConfigParser::MissingClosingSemicolonException::what() const throw() {

    return ("\033[1;31m Missing semicolon.\033[0m");
}

const char* ConfigParser::MisconfigurationException::what() const throw() {

    return ("\033[1;31m Misconfigured config file.\033[0m");
}

const char* ConfigParser::ErrorCodeMisconfiguration::what() const throw() {

    return ("\033[1;31m Misconfigured error code.\033[0m");
}

const char* ConfigParser::MissingErrorCodePage::what() const throw() {

    return ("\033[1;31m Missing error_page path/file.\033[0m");
}

const char* ConfigParser::FileOpenErrorException::what() const throw() {

    return ("\033[1;31m Failed opening config file\033[0m");
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
        for (std::map<int, std::pair<std::string, std::string> >::const_iterator it = server.error_pages.begin(); it != server.error_pages.end(); ++it)
            std::cout << "    " << it->first
            << " => (" << it->second.first
            << ", " << it->second.second << ")\n";
    
        std::cout << "Server Default Indices: ";
        if (!server.default_indices.empty()) {
            for (size_t l = 0; l < server.default_indices.size(); l++) {
                std::cout << "      Index[" << l << "]: " << server.default_indices[l] << std::endl;
            }
        }

        std::cout << "Server Default Root: ";
        if (!server.default_root.empty())
            std::cout << "      Default Root: " << server.default_root << "\n";

        
        std::cout << "  Locations:\n";
        for (size_t k = 0; k < server.locations.size(); ++k) {
            const LocationConfig& loc = server.locations[k];
            std::cout << "-----------------------Location #" << k + 1 << ":\n";
            if (!loc.path.empty())
                std::cout << "      Path: " << loc.path << "\n";
            if (!loc.root.empty())
                std::cout << "      Root: " << loc.root << "\n";
            if (!loc.indices.empty()) {
                for (size_t l = 0; l < loc.indices.size(); l++) {
                    std::cout << "      Index[" << l << "]: " << loc.indices[l] << std::endl;
                }
            }
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
            std::cout << std::endl;
        }
    
        std::cout << "----------------------\n";
    }
}