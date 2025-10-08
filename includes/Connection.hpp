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

#define MAX_EVENTS 1024
#define TIME_OUT 1000
// seconds
#define CLIENT_REQUEST_TIME_OUT 5
#define CLIENT_TOTAL_TIME_OUT 20

#include "./Servers.hpp"
#include "./HttpReceive.hpp"

struct PollData
{
    int             fd;
    size_t          server_index;
	bool            is_listener;
	HttpReceive     *client;
    int             _start_time;
    int             _current_time;
    bool            client_time_out;

    PollData() : fd(-1), server_index(0), is_listener(false), client(NULL) {}
    PollData(int _fd, size_t _i, bool _l) : fd(_fd), server_index(_i), is_listener(_l), client(NULL) {}
};

class Connection : public Servers
{
    private:
        std::map<int, PollData>         fd_map;
        std::vector<epoll_event>        events;
        int                             epoll_fd;

        void                            SetupAllServers(Servers& servers);
        void                            populateSockets(Servers& servers);
        void                            createEpollInstance();
        int                             setNonBlocking(int fd);
        void                            populateServerPollData(int index, int listen_fd);
        void                            addServerEpollEvent(int listen_fd);
        void                            populateClientPollData(Servers& servers, PollData &pd, int client_fd);
        void                            addClientEpollEvent(int client_fd);
        


    public:
        Connection(Servers& servers);
        Connection(const Connection& src);
        Connection& operator=(const Connection& src);
        ~Connection();

        
        
        int                             acceptClient(Servers& servers, int fd, PollData &pd);
        void                            removeClient(PollData& pd);
        void                            removeTimeoutClients(time_t now);
        void                            setEpollFd(int fd);
        int                             getEpollFd() const;
        epoll_event*                    getEpollEvents();
        epoll_event&                    getEpollEvent(int index);
        std::map<int, PollData>&        getFdMap();
        const std::map<int, PollData>&  getFdMap() const;

        void                            print_epoll_event(const epoll_event &ev);

        class EpollInstanceException: public std::exception {public:const char* what() const throw();};
        class AddEpollInstanceException: public std::exception {public:const char* what() const throw();};

        
};


#endif
