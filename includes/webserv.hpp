#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#define MAX_CLIENTS 100
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


struct LocationConfig
{
    std::string                 path;
    std::string                 root;
    std::vector<std::string>    indices;
    std::set<std::string>       methods;
    bool                        auto_index;
    std::string                 redirect;
    size_t                      redirect_code;
    std::set<std::string>       cgi_extensions;
    std::string                 upload_store;
};

struct ServerConfig
{
    std::vector<std::pair<std::string, int> >               ips_and_ports;
    std::vector<std::string>                                server_names;
    std::map<int, std::pair<std::string, std::string> >     error_pages;
    unsigned long                                           client_max_body_size;
    std::string                                             default_root;
    std::vector<std::string>                                default_indices;
    std::vector<LocationConfig>                             locations;
};


bool			isDirectory(const char* path);
bool			isValidHttpVersion(const std::string& version);
bool            isNumber(const std::string &s);
int             checkContentLength(const char *num_str, unsigned long max_size);
bool            isValidHeaderName(std::string header_name);
bool            isValidHeaderValue(std::string header_value);

#endif
