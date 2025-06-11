#ifndef WEBSERV_HPP
# define WEBSERV_HPP

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
      
int     main(void);


#endif
