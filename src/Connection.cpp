#include "../includes/Connection.hpp"
#include "../includes/Logger.hpp"

Connection::Connection(Servers& servers) : events(MAX_EVENTS)  {

    SetupAllServers(servers);
    createEpollInstance();
    populateSockets(servers);
}

Connection::Connection(const Connection& src) {*this = src;}

Connection& Connection::operator=(const Connection& src) {(void)src;return (*this);};

Connection::~Connection() {if (epoll_fd >= 0) close(epoll_fd);}


void            Connection::SetupAllServers(Servers& servers) {

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
		}
    }
}

void              Connection::createEpollInstance() {

    setEpollFd(epoll_create(1));
    if (this->epoll_fd == -1) {

        perror("epoll_create1");
        throw (EpollInstanceException());
    }
}

void              Connection::populateSockets(Servers& servers) {

    for (size_t i = 0; i < servers.size(); i++) {

        for (size_t j = 0; j < servers[i].getSocketsSize(); j++) {

            int listen_fd = servers[i].getSocket(j);
            setNonBlocking(listen_fd);
            addServerEpollEvent(listen_fd);
            populateServerPollData(i, listen_fd, servers[i], j);
        }
    }
}

void                Connection::addServerEpollEvent(int listen_fd) {

    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = EPOLLIN;
    ev.data.fd = listen_fd;
    if (epoll_ctl(getEpollFd(), EPOLL_CTL_ADD, listen_fd, &ev) == -1) {

        throw (AddEpollInstanceException());
    }
}

void                Connection::populateServerPollData(int server_index, int listen_fd, ServerWrapper& server, int j) {

    PollData pd;
    std::ostringstream  oss;

    oss << server.getPorts(j);
    pd.fd = listen_fd;
    pd.ip_port = "[" + server.getIps(j) + ":" + oss.str() + "] - ";
    pd.server_index = server_index;
    pd.is_listener = true;
    pd.client = NULL;
    pd._start_time = 0;
    pd._current_time = 0;
    pd.client_time_out = false;
    pd.client_allocated = false;
    this->fd_map[listen_fd] = pd;
    logger(listen_fd, SERVER_CONNECT, 0);
}

int            Connection::acceptClient(Servers& servers, int fd, PollData &pd) {

    sockaddr_in     client_addr;
    socklen_t       client_len = sizeof(client_addr);
    int             client_fd = accept(fd, (sockaddr*)&client_addr, &client_len);

    if (client_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return (0);
        } else if (errno == ECONNABORTED) {
            std::cerr << "accept() failed: ECONNABORTED" << std::endl;
            return (-1);
        } else if (errno == EMFILE || errno == ENFILE) {
            std::cerr << "accept() failed: too many open files" << std::endl;
            return -1; 
        }
    }

    if (setNonBlocking(client_fd) == -1) {
        close(client_fd);
        return (-1);
    }
    addClientEpollEvent(client_fd);
    populateClientPollData(servers, pd, client_fd);
    return (0);
}

void                Connection::populateClientPollData(Servers& servers, PollData &pd, int client_fd) {

    PollData client_pd;

    client_pd.fd = client_fd;
    client_pd.ip_port = pd.ip_port;
    client_pd.server_index = pd.server_index;
    client_pd.is_listener = false;
    client_pd.client = new HttpReceive(servers[pd.server_index]);
    client_pd._start_time = std::time(0);
    client_pd._current_time = client_pd._start_time;
    client_pd.client_time_out = false;
    client_pd.client_allocated = true;
    client_pd.client->setFd(client_fd); 
    this->fd_map[client_fd] = client_pd;
    logger(client_fd, CLIENT_CONNECT, 0);
}

void                Connection::addClientEpollEvent(int client_fd) {

    struct epoll_event client_ev;

    memset(&client_ev, 0, sizeof(client_ev));
    client_ev.events = EPOLLIN | EPOLLET;
    client_ev.data.fd = client_fd;
    epoll_ctl(getEpollFd(), EPOLL_CTL_ADD, client_fd, &client_ev);
}

int                 Connection::setNonBlocking(int fd) {

    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "fcntl(F_GETFL) error: " << strerror(errno) << std::endl;
        return (-1);
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "fcntl(F_SETFL) error: " << strerror(errno) << std::endl;
        return (-1);
    }
    return (0);
}


void            Connection::removeClient(PollData& pd) {

    if (pd.client == NULL)
        return ;
    
    int client_fd = pd.client->getFd();
    logger(client_fd, CLIENT_DISCONNECT, 0);
    if (client_fd != -1) {
        epoll_ctl(getEpollFd(), EPOLL_CTL_DEL, client_fd, NULL);
        close(client_fd);
    }
    if (pd.client_allocated)
        delete pd.client;
    pd.client = NULL;
    pd.client_allocated = false;
    this->fd_map.erase(client_fd);
}


void        Connection::removeTimeoutClients(time_t now) {

	std::vector<int> fds_to_remove;
    std::map<int, PollData>::iterator it = getFdMap().begin();

	for ( ; it != getFdMap().end(); ++it) {

		PollData &pd = it->second;
		if (!pd.is_listener) {
            int time_elapsed = now - pd._current_time;
            int time_left = CLIENT_REQUEST_TIME_OUT - time_elapsed;
            if (time_left < 0)
                time_left = 0;
            logger(it->second.fd, CLIENT_TIME_OUT, time_left);
        }
		if (!pd.is_listener && (now - pd._current_time >= CLIENT_REQUEST_TIME_OUT)) {
			fds_to_remove.push_back(it->first);
		}
	}
		
	for (size_t i = 0; i < fds_to_remove.size(); i++) {
		std::map<int, PollData>::iterator it = getFdMap().find(fds_to_remove[i]);
		if (it != getFdMap().end()) {
			removeClient(it->second);
		}
	}
}


void            Connection::logger(int target_fd, int flag, int time_left) { Logger::logger(this->fd_map[target_fd], flag, time_left); };

void              Connection::print_epoll_event(const epoll_event &ev) {
    
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

void                Connection::setEpollFd(int fd) {this->epoll_fd = fd;}

int                 Connection::getEpollFd() const {return (this->epoll_fd);}

epoll_event*        Connection::getEpollEvents() {return (&this->events[0]);}

epoll_event&        Connection::getEpollEvent(int index) {

    if (index < 0 || index >= MAX_EVENTS) {
        throw std::out_of_range("Invalid epoll_event index");
    }
    return (this->events[index]);
}

std::map<int, PollData>&     Connection::getFdMap() {return (this->fd_map);}

const std::map<int, PollData>&     Connection::getFdMap() const {return (this->fd_map);}

const char*         Connection::EpollInstanceException::what() const throw() {return ("Error creating epoll instance.");}

const char*         Connection::AddEpollInstanceException::what() const throw() {return ("Error adding event to epoll instance.");}