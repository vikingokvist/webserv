#ifndef WEBSERV_HPP
# define WEBSERV_HPP
 
# include <iostream>
# include <exception>
# include <cerrno>
# include <cstring>
# include <csignal>
# include <unistd.h>
# include <sys/types.h>  
# include <sys/socket.h>
# include <netinet/in.h> 
# include <netdb.h>      
# include <sys/select.h> 
# include <poll.h>       
# include <sys/epoll.h>  
# include <sys/stat.h>   
# include <fcntl.h>      
# include <sys/wait.h>   
# include <dirent.h> 
# include <vector>  
# include <map>

# define MAX_CLIENTS 100
# define BUFFER_SIZE 1024
      
class TcpServer
{
    private:
        int                         server_fd;
        int                         port;
        std::string                 ip;
        struct sockaddr_in          server_addr;
        socklen_t                   addr_len;
        std::vector<struct pollfd>  poll_fds;
        std::map<int, std::string>  client_buffers;

        bool                        setupServerSocket();
        void                        acceptNewClient();
        void                        handleClientData(int client_fd);
        void                        removeClient(int index);

    public:

        TcpServer(std::string& ip_address, int port_address);
        ~TcpServer();

        void run();
};


#endif
