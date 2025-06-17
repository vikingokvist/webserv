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

void    ConfigParser::parseFile(const std::string& filename) {

    std::ifstream config_file(filename.c_str());
    std::string line;


    if (!config_file.is_open())
        throw (ConfigParser::FileOpenErrorException());


    while (std::getline(config_file, line) && line.empty())
        continue ;
    
    if (line.find("server") != std::string::npos)
        throw (ConfigParser::MisconfigurationException());

    
    


}

const char* ConfigParser::MisconfigurationException::what() const throw() {

    return ("Misconfigured config file");
}

const char* ConfigParser::FileOpenErrorException::what() const throw() {

    return ("Failed opening config file");
}
