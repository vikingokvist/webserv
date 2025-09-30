#ifndef SERVER_SET_UP_HPP
# define SERVER_SET_UP_HPP

#include <cstddef>
#include <cstdint>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <map>
#include <exception>

#define MAX_EVENTS 1024
#define TIME_OUT 1000

#include "./Servers.hpp"

struct PollData
{
    int         fd;
    size_t      server_index;
	bool        is_listener;
	Connection  *_conn;

    PollData() : fd(-1), server_index(0), is_listener(false), _conn(0) {}
    PollData(int _fd, size_t _i, bool _l) : fd(_fd), server_index(_i), is_listener(_l), _conn(0) {}
};

class ServerSetUp : public Servers
{
    private:
        std::map<int, PollData> fd_map;
        epoll_event             events[MAX_EVENTS];
        int                     epoll_fd;
        int                     listen_fd;

        void                  SetupAllServers(Servers& servers);
        void                  populateSockets(Servers& servers);
        void                  createEpollInstance();
        void                  setNonBlocking(int fd);
        void                  populatePollData(int index);
        void                  addEpollEvent();


    public:
        ServerSetUp(Servers& servers);
        ServerSetUp(const ServerSetUp& src);
        ServerSetUp& operator=(const ServerSetUp& src);
        ~ServerSetUp();

        
        
        void                  setEpollFd(int fd);
        void                  setListenFd(int fd);
        int                   getEpollFd() const;
        int                   getListenFd() const;
        epoll_event*          getEpollEvents();
        epoll_event&          getEpollEvent(int index);
        int                   getMaxEvents() const;
        int                   getTimeOut() const;
        std::map<int, PollData> getFdMap() const;
        

        void                  print_epoll_event(const epoll_event &ev);

        class EpollInstanceException: public std::exception {public:const char* what() const throw();};
        class AddEpollInstanceException: public std::exception {public:const char* what() const throw();};
};


#endif
