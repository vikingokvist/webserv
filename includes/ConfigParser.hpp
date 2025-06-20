#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include "./webserv.hpp"

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

struct ParserVariables
{
    std::string                         buffer;
    std::vector<std::string>            config_array;
    std::vector<std::string>::iterator  it;
    std::string                         token;
    bool                                in_server;
    bool                                in_location;
    ServerConfig                        cur_server;
    LocationConfig                      cur_loc;
};


class ConfigParser
{
    private:
        std::vector<ServerConfig>   _servers;
        std::vector<std::string>    cp_split(const std::string& str, char delimiter);
        std::string                 trim(const std::string& str);
        size_t                      str_to_size_t(const std::string& s);
        void                        parseFile(const std::string& filename, ParserVariables& vars);
        void                        parseConfigFile(ParserVariables& vars);
        void                        handleBracketStack(ParserVariables& vars);
        void                        listenToken(ParserVariables& vars);
        void                        serverNameToken(ParserVariables& vars);
        void                        clientMaxBodySizeToken(ParserVariables& vars);
        void                        errorPageToken(ParserVariables& vars);
        void                        uploadStoreToken(ParserVariables& vars);
        void                        cgiExtensionToken(ParserVariables& vars);
        void                        redirectToken(ParserVariables& vars);
        void                        autoIndexToken(ParserVariables& vars);
        void                        methodsToken(ParserVariables& vars);
        void                        indexToken(ParserVariables& vars);
        void                        rootToken(ParserVariables& vars);
        void                        printParsedConfig(const std::vector<ServerConfig>& servers);
        bool                        isMisconfiguredLocation(std::string token);


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
