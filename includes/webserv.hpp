#ifndef WEBSERV_HPP
#define WEBSERV_HPP


#define DEFAULT_CONF_FILE "www/default.conf"
#define ERROR_ARGUMENTS "Error: Wrong amount of arguments."

#include <iostream>
#include <exception>
#include <cerrno>
#include <cstring>
#include <csignal>
#include <unistd.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h> 
#include <netdb.h>      
#include <sys/select.h> 
#include <poll.h>       
#include <sys/epoll.h>  
#include <sys/stat.h>   
#include <fcntl.h>      
#include <sys/wait.h>   
#include <dirent.h>
#include <string>
#include <arpa/inet.h>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <cctype>
#include <cstdarg>
#include <filesystem>
#include <map>
#include <set>
#include <vector>
#include <utility>
#include <climits>

class Connection;

struct LocationConfig
{
    std::string                 path;
    std::string                 root;
    std::vector<std::string>    indices;
    std::set<std::string>       methods;
    bool                        auto_index;
    std::string                 auto_index_str;
    std::string                 redirect;
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
    unsigned long                                           client_max_body_size;
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

struct Part
{
    std::string headers;
    std::string content;
    std::string filename;
	std::string	content_type;
};





std::map<std::string, std::string>	parseUrlEncoded(const std::string& body);
std::vector<Part>                   parseMultipart(const std::string& body, const std::string& boundary);
void		                        removeSpaces(std::string& str1, std::string& str2);
bool			                    isDirectory(const char* path);
bool			                    isValidHttpVersion(const std::string& version);
bool                                isNumber(const std::string &s);
int                                 checkContentLength(const char *num_str, unsigned long max_size);
bool                                isValidHeaderName(std::string header_name);
bool                                isValidHeaderValue(std::string header_value);

#endif
