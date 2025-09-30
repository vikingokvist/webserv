#include "../includes/ServerSetUp.hpp"

ServerSetUp::ServerSetUp(Servers& servers) : events()  {

    SetupAllServers(servers);
    createEpollInstance();
    populateSockets(servers);
}

ServerSetUp::ServerSetUp(const ServerSetUp& src) {*this = src;}

ServerSetUp& ServerSetUp::operator=(const ServerSetUp& src) {(void)src;return (*this);};

ServerSetUp::~ServerSetUp() {}


void            ServerSetUp::SetupAllServers(Servers& servers) {

	for (size_t i = 0; i < servers.size(); i++) {

		for (int j = 0; j < servers[i].getCountIpPorts(); j++) {

			std::string ip = servers[i].getIps(j).c_str();
			uint16_t port = servers[i].getPorts(j);
			servers[i].setHost(inet_addr(ip.c_str()));
			servers[i].setPort(port);
			servers[i].setMaxClientSize(servers[i].getClientMaxBodySize());
			servers[i].setupSocket();
			servers[i].setServerName(servers[i].getServerNamesList());
       		servers[i].setupServerConfig(servers[i].getServerName(), htons(servers[i].getPort()),
                      servers[i].getHost(), AF_INET, servers[i].getMaxClientSize());
			servers[i].bindAndListen();
			servers[i].addSocket(servers[i].getTheSocket());
			std::cout << "✓ Server listening on " << ip << ":" << port << std::endl;
		}
    }
}

void              ServerSetUp::createEpollInstance() {

    setEpollFd(epoll_create(1));
    if (this->epoll_fd == -1) {

        perror("epoll_create1");
        throw (EpollInstanceException());
    }
}

void              ServerSetUp::populateSockets(Servers& servers) {

    for (size_t i = 0; i < servers.size(); i++) {

        for (size_t j = 0; j < servers[i].getSocketsSize(); j++) {

            setListenFd(servers[i].getSocket(j));
            setNonBlocking(getListenFd());
            addEpollEvent();
            populatePollData(i);
            std::cout << "✓ Server listening on fd " << getListenFd() << std::endl;
        }
    }
}

void                ServerSetUp::setNonBlocking(int fd) {

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void                ServerSetUp::populatePollData(int server_index) {

    PollData pd;
    pd.is_listener = true;
    pd.server_index = server_index;
    pd._conn = NULL;
    this->fd_map[getListenFd()] = pd;
}

void                ServerSetUp::addEpollEvent() {

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = getListenFd();
    if (epoll_ctl(getEpollFd(), EPOLL_CTL_ADD, getListenFd(), &ev) == -1) {

        throw (AddEpollInstanceException());
    }
}

void              ServerSetUp::print_epoll_event(const epoll_event &ev) {
    
    std::cout << "fd: " << ev.data.fd << std::endl;
    std::cout << "events: ";

    if (ev.events & EPOLLIN)  std::cout << "EPOLLIN ";
    if (ev.events & EPOLLOUT) std::cout << "EPOLLOUT ";
    if (ev.events & EPOLLERR) std::cout << "EPOLLERR ";
    if (ev.events & EPOLLHUP) std::cout << "EPOLLHUP ";
    if (ev.events & EPOLLET) std::cout << "EPOLLET ";
    if (ev.events & EPOLLONESHOT) std::cout << "EPOLLONESHOT ";

    std::cout << std::endl;
}

void                ServerSetUp::setEpollFd(int fd) {this->epoll_fd = fd;}

void                ServerSetUp::setListenFd(int fd) {this->listen_fd = fd;}

int                 ServerSetUp::getEpollFd() const {return (this->epoll_fd);}

int                 ServerSetUp::getListenFd() const {return (this->listen_fd);}

epoll_event*        ServerSetUp::getEpollEvents() {return (this->events);}

epoll_event&        ServerSetUp::getEpollEvent(int index) {

    if (index < 0 || index >= MAX_EVENTS) {
        throw std::out_of_range("Invalid epoll_event index");
    }
    return (this->events[index]);
}

int                 ServerSetUp::getMaxEvents() const {return (MAX_EVENTS);}

int                 ServerSetUp::getTimeOut() const {return (TIME_OUT);}

std::map<int, PollData>     ServerSetUp::getFdMap() const {return (this->fd_map);}

const char*         ServerSetUp::EpollInstanceException::what() const throw() {return ("Error creating epoll instance.");}

const char*         ServerSetUp::AddEpollInstanceException::what() const throw() {return ("Error adding event to epoll instance.");}