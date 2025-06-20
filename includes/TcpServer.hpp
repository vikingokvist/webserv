#ifndef TCP_SERVER_HPP
# define TCP_SERVER_HPP
 
# include "./webserv.hpp"

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
