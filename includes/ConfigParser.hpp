#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include <iostream>
#include <exception>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <sstream>

enum BracketState 
{
    B_DEFAULT,
    B_OPEN,
    B_CLOSED
};

struct LocationConfig
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

class ConfigParser
{
    private:
        std::vector<ServerConfig>   _servers;
        void                        parseFile(const std::string& filename);
        std::string                 trim(const std::string& str);
        std::vector<std::string>    split(const std::string& str, char delim);
        size_t                      convertValueToBytes(std::string line);

    public:
        ConfigParser();
        ConfigParser(const std::string& filename);
        ConfigParser(const ConfigParser& src);
        ConfigParser& operator=(const ConfigParser& src);
        ~ConfigParser();


        const std::vector<ServerConfig>& getServers() const;
        class MisconfigurationException: public std::exception {public: const char* what() const throw();};
        class FileOpenErrorException: public std::exception {public: const char* what() const throw();};


};

#endif