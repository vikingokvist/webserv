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
#include <ctime>
#include <iomanip>

struct Session
{
	std::string     session_id;
    int             _current_time;
};


std::map<std::string, std::string>	parseUrlEncoded(const std::string& body);
void		                        removeSpaces(std::string& str1, std::string& str2);
bool			                    isDirectory(const char* path);
bool			                    isValidHttpVersion(const std::string& version);
bool                                isNumber(const std::string &s);
int                                 checkContentLength(const char *num_str, unsigned long max_size);
bool                                isValidHeaderName(std::string header_name);
bool                                isValidHeaderValue(std::string header_value);
bool                                isMissingRequiredHeaders(std::map<std::string, std::string> &headers);
bool                                isUnsupportedMethod(std::map<std::string, std::string> &headers);
bool                                isPathTraversal(std::map<std::string, std::string> &headers);
bool                                isUriTooLong(std::map<std::string, std::string> &headers);
bool                                isInvalidHttpVersion(std::map<std::string, std::string> &headers);
bool                                isInvalidContentLength(std::map<std::string, std::string> &headers); 
bool                                isMissingContentLengthForPost(std::map<std::string, std::string> &headers);
bool                                isContentLengthTooLarge(std::map<std::string, std::string> &headers, size_t max_client_size);
bool                                isMissingContentTypeForPost(std::map<std::string, std::string> &headers);
bool                                clientHasCookiesEnabled(std::map<std::string, std::string> &headers);
void                                addSession(std::map<std::string, Session>& session, const std::string& session_id);
double                              getSessionDuration(std::map<std::string, Session>& session, const std::string& session_id);    
std::string                         parseSessionId(const std::string &cookie_header);
std::string                         generateSessionId();
std::string                         ensureSession(std::map<std::string, Session>& session, std::string session_id, bool& is_new_session);

#endif
