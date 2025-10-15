#ifndef CONNECTION_HPP
# define CONNECTION_HPP

#include <cstddef>
#include <stdint.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <map>
#include <exception>

#include "./Servers.hpp"
#include "./HttpReceive.hpp"
#include "./webserv.hpp"

#define MAX_EVENTS 1024
#define EPOLL_TIME_OUT 1000
#define CLIENT_REQUEST_TIME_OUT 5


struct ServerSocket
{
    int	                                _fd; 
	std::vector<std::string>            _server_name; 
	uint16_t                            _port; 
	in_addr_t                           _host;
	sa_family_t                         _sin_family;
	unsigned long		                _client_max_body_size;
	struct sockaddr_in	                _server_adress;
    std::map<std::string, Session>      _session;
};

struct PollData
{
    int             fd;
    std::string     ip_port;
    size_t          server_index;
	bool            is_listener;
	HttpReceive     *client;
    int             _start_time;
    int             _current_time;
    bool            client_time_out;
    bool            client_allocated;
    

    PollData() : fd(-1), server_index(0), is_listener(false), client(NULL) {}
    PollData(int _fd, size_t _i, bool _l) : fd(_fd), server_index(_i), is_listener(_l), client(NULL) {}
};


class Connection {

    private:

        std::vector<ServerSocket>       server_sockets;
        std::map<int, PollData>         fd_map;
        std::vector<epoll_event>        events;
        int                             epoll_fd;

    public:

        Connection(Servers& servers);
        Connection(const Connection& src);
        Connection& operator=(const Connection& src);
        ~Connection();

        void                            modifyEpollEvent(int fd, uint32_t events);
        int                             acceptClient(Servers& servers, int fd, PollData &pd);
        void                            removeClient(PollData& pd);
        void                            removeTimeoutClients(time_t now);
        int                             getEpollFd() const;
        epoll_event*                    getEpollEvents();
        epoll_event&                    getEpollEvent(int index);
        std::map<int, PollData>&        getFdMap();
        const std::map<int, PollData>&  getFdMap() const;
        void                            logger(int target_fd, int flag, int time_left);
        class EpollInstanceException: public std::exception {public:const char* what() const throw();};
        class AddEpollInstanceException: public std::exception {public:const char* what() const throw();};


    private:

		void	                        bindAndListen(ServerSocket& s_socket);
		void	                        setupSocket(ServerSocket& s_socket);
        int                             setNonBlocking(int fd);
        void                            populateServerPollData(int server_index, int listen_fd, std::string ip, uint16_t port);
        void                            addServerEpollEvent(int listen_fd);
        void                            populateClientPollData(Servers& servers, PollData &pd, int client_fd);
        void                            addClientEpollEvent(int client_fd);
    
};

#endif
