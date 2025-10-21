#ifndef CONFIG_PARSER_HPP
# define CONFIG_PARSER_HPP

#include "./webserv.hpp"

const uint64_t MAX_SIZE = 4294967296ULL;

struct LocationConfig
{
    std::string                 path;
    std::string                 root;
    std::vector<std::string>    indices;
    std::set<std::string>       methods;
    bool                        auto_index;
    std::string                 auto_index_str;
    bool                        has_redirect;
    std::string                 redirect_url;
    size_t                      redirect_code;
    std::set<std::string>       cgi_extensions;
    std::string                 upload_store;
    int                         bracket_state;
};

struct ServerConfig
{
    std::vector<std::pair<std::string, int> >               ips_and_ports;
    std::vector<std::string>                                server_names;
    std::map<int, std::pair<std::string, std::string> >     error_pages;
    uint64_t                                                client_max_body_size;
    std::string                                             has_client_max_body_size;
    std::string                                             default_root;
    std::vector<std::string>                                default_indices;
    std::vector<LocationConfig>                             locations;
    int                                                     bracket_state;
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
    size_t                              cur_server_index;
    LocationConfig                      cur_loc;
    int                                 cur_loc_index;
};

class ConfigParser
{
    private:
        std::vector<ServerConfig>   _servers;
        std::vector<std::string>    cp_split(const std::string& str, char delimiter);
        std::string                 trim(const std::string& str);
        uint64_t                    str_to_unsigned_long(const std::string& s);
        void                        parseFile(const std::string& filename, ParserVariables& vars);
        void                        parseConfigFile(ParserVariables& vars);
        void                        handleOpenBracket(ParserVariables& vars);
        void                        handleClosedBracket(ParserVariables& vars);
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
        void                        defaultServerRoot(ParserVariables& vars);
        void                        defaultServerIndex(ParserVariables& vars);
        bool                        isMisconfiguredLocation(ParserVariables& vars);
        bool                        isMisconfiguredServer(ParserVariables& vars);

    public:
        ConfigParser();
        ConfigParser(const std::string& filename);
        ConfigParser(const ConfigParser& src);
        ConfigParser& operator=(const ConfigParser& src);
        ~ConfigParser();

        const std::vector<ServerConfig>& getServers() const;


        class DuplicateLocationException : public std::exception {
            std::string _msg;
        public:
            DuplicateLocationException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Duplicate location.\033[0m";}
            virtual ~DuplicateLocationException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };

        class DuplicateVariablesException : public std::exception {
            std::string _msg;
        public:
            DuplicateVariablesException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Duplicate variable.\033[0m";}
            virtual ~DuplicateVariablesException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class MissingValueException : public std::exception {
            std::string _msg;
        public:
            MissingValueException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Missing value.\033[0m";}
            virtual ~MissingValueException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class MissingPortException : public std::exception {
            std::string _msg;
        public:
            MissingPortException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Missing port.\033[0m";}
            virtual ~MissingPortException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class UnknownVariableException : public std::exception {
            std::string _msg;
        public:
            UnknownVariableException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Unknown variable.\033[0m";}
            virtual ~UnknownVariableException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        

        class ClientMaxBodySizeValueException : public std::exception {
            std::string _msg;
        public:
            ClientMaxBodySizeValueException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m client_max_body_size value should be a number followed by [m/M/g/G/k/K].\033[0m";}
            virtual ~ClientMaxBodySizeValueException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        class ExtraVariablesException : public std::exception {
            std::string _msg;
        public:
            ExtraVariablesException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Extra variables not supported.\033[0m";}
            virtual ~ExtraVariablesException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class ClientMaxBodySizeException : public std::exception {
        	std::string _msg;
        public:
        	ClientMaxBodySizeException(const std::string& token, const std::string& context) throw() {
        		_msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m client_max_body_size exceeds 4 GiB limit.\033[0m";
        	}
        	virtual ~ClientMaxBodySizeException() throw() {}
        	const char* what() const throw() { return _msg.c_str(); }
        };

        class UnknownVariableValueException : public std::exception {
            std::string _msg;
        public:
            UnknownVariableValueException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Unknown value.\033[0m";}
            virtual ~UnknownVariableValueException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
   
        class ExtraOpenBracketException : public std::exception {
            std::string _msg;
        public:
            ExtraOpenBracketException(const std::string& context) throw() {
                _msg = "[" + context + " ] <= \033[1;31m Extra open bracket.\033[0m";}
            virtual ~ExtraOpenBracketException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };   

        class ExtraClosingBracketException : public std::exception {
            std::string _msg;
        public:
            ExtraClosingBracketException(const std::string& context) throw() {
                _msg = "[" + context + " ] <= \033[1;31m Extra closing bracket.\033[0m";}
            virtual ~ExtraClosingBracketException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };   

        class MissingClosingBracketException : public std::exception {
            std::string _msg;
        public:
            MissingClosingBracketException(const std::string& context) throw() {
                _msg = "[" + context + " ] <= \033[1;31m Missing closing bracket.\033[0m";}
            virtual ~MissingClosingBracketException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class MissingClosingSemicolonException : public std::exception {
            std::string _msg;
        public:
            MissingClosingSemicolonException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Missing semicolon.\033[0m";}
            virtual ~MissingClosingSemicolonException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class MisconfigurationException : public std::exception {
            std::string _msg;
        public:
            MisconfigurationException(const std::string& context) throw() {
                _msg = "[" + context + " ] <= \033[1;31m Misconfigured config file.\033[0m";}
            virtual ~MisconfigurationException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class ErrorCodeMisconfiguration : public std::exception {
            std::string _msg;
        public:
            ErrorCodeMisconfiguration(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Misconfigured error code.\033[0m";}
            virtual ~ErrorCodeMisconfiguration() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class MissingRedirectCodeException : public std::exception {
            std::string _msg;
        public:
            MissingRedirectCodeException(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Missing redirection code.\033[0m";}
            virtual ~MissingRedirectCodeException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };

        class MissingErrorCodePage : public std::exception {
            std::string _msg;
        public:
            MissingErrorCodePage(const std::string& token, const std::string& context) throw() {
                _msg = "[" + context + " ] => \"" + token + "\" <= \033[1;31m Missing error_page path/file.\033[0m";}
            virtual ~MissingErrorCodePage() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
        
        class FileOpenErrorException : public std::exception {
            std::string _msg;
        public:
            FileOpenErrorException(const std::string& file_path) throw() {
                _msg = "[server] => \"" + file_path + "\" <= \033[1;31m Failed opening config file.\033[0m";}
            virtual ~FileOpenErrorException() throw() {}
            const char* what() const throw() { return _msg.c_str(); }
        };
};


#endif
