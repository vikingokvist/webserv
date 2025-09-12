#ifndef WEBSERV_HPP
#define WEBSERV_HPP

#define MAX_CLIENTS 100
#define BUFFER_SIZE 1024

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
    std::vector<LocationConfig>                             locations;
};


std::string		getContentType(const std::string& path);
bool			fileExistsAndReadable(const char* path);
bool			isDirectory(const char* path);
bool			isValidHttpVersion(const std::string& version);

#endif
