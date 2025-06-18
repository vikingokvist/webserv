#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include <iostream>
#include <exception>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stack>
#include <map>
#include <set>


struct LocationConfig
{
    std::string             path;
    std::string             root;
    std::string             index;
    std::set<std::string>   methods;
    bool                    auto_index;
    std::string             redirect;
    size_t                  redirect_code;
    std::set<std::string>   cgi_extensions;
    std::string             upload_store;
};

struct ServerConfig
{
    std::vector<std::pair<std::string, int> >   ips_and_ports;
    std::vector<std::string>                    server_names;
    std::map<int, std::string>                  error_pages;
    size_t                                      client_max_body_size;
    std::vector<LocationConfig>                 locations;
};

class ConfigParser
{
    private:
        std::vector<ServerConfig>   _servers;
        std::vector<std::string>    split(const std::string& str, char delimiter);
        std::string                 parseFile(const std::string& filename);
        std::string                 trim(const std::string& str);
        void                        parseConfigFile(std::vector<std::string> config_array);
        void                        handleBracketStack(std::stack<std::string>& bracket_stack, std::string token, std::string& found_del, ServerConfig& cur_server);
        void                        listenToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server);
        void                        serverNameToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server);
        void                        clientMaxBodySizeToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server);
        void                        errorPageToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, ServerConfig& cur_server);
        void                        uploadStoreToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);
        void                        cgiExtensionToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);
        void                        redirectToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);
        void                        autoIndexToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);
        void                        methodsToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);
        void                        indexToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);
        void                        rootToken(std::string& token, std::vector<std::string>& config_array, std::vector<std::string>::iterator& it, LocationConfig& cur_loc);


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
