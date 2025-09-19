#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include "./webserv.hpp"


struct ParserVariables
{
    std::string                         buffer;
    std::vector<std::string>            config_array;
    std::vector<std::string>::iterator  it;
    std::string                         token;
    bool                                in_server;
    bool                                in_location;
    ServerConfig                        cur_server;
    size_t                              cur_server_index;
    LocationConfig                      cur_loc;
};


class ConfigParser
{
    private:
        std::vector<ServerConfig>   _servers;
        std::vector<std::string>    cp_split(const std::string& str, char delimiter);
        std::string                 trim(const std::string& str);
        unsigned long               str_to_unsigned_long(const std::string& s);
        void                        parseFile(const std::string& filename, ParserVariables& vars);
        void                        parseConfigFile(ParserVariables& vars);
        void                        handleBracketStack(ParserVariables& vars);
        int                         listenToken(ParserVariables& vars);
        int                         serverNameToken(ParserVariables& vars);
        int                         clientMaxBodySizeToken(ParserVariables& vars);
        int                         errorPageToken(ParserVariables& vars);
        int                         uploadStoreToken(ParserVariables& vars);
        int                         cgiExtensionToken(ParserVariables& vars);
        int                         redirectToken(ParserVariables& vars);
        int                         autoIndexToken(ParserVariables& vars);
        int                         methodsToken(ParserVariables& vars);
        int                         indexToken(ParserVariables& vars);
        int                         rootToken(ParserVariables& vars);
        int                         defaultServerRoot(ParserVariables& vars);
        int                         defaultServerIndex(ParserVariables& vars);
        void                        printParsedConfig(const std::vector<ServerConfig>& servers);
        bool                        isMisconfiguredLocation(ParserVariables& vars);
        bool                        isMisconfiguredServer(ParserVariables& vars);

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
