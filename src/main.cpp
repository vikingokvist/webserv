#include "../includes/webserv.hpp"
#include "../includes/Connection.hpp"
#include "../includes/Servers.hpp"

#include <algorithm>

std::string _previous_full_path = "";


void setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) flags = 0;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void SetupAllServers(Servers& servers) {
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
			servers[i].addSocket(servers[i].getSocket());
			std::cout << "✓ Server listening on " << ip << ":" << port << std::endl;
		}
    }
}

void print_epoll_event(const epoll_event &ev) {
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

int main(int argc, char **argv)
{
    std::string conf_file = (argc < 2) ? DEFAULT_CONF_FILE : argv[1];
    if (argc > 2)
        return (std::cout << ERROR_ARGUMENTS << std::endl, 1);

    Servers servers(conf_file);
	std::map<int, PollData> fd_map;
    SetupAllServers(servers);

    // Crear epoll
    int epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        return 1;
    }

    // Registrar sockets de escucha en epoll
    for (size_t i = 0; i < servers.size(); i++) {
        const std::vector<int>& sockets = servers[i].getSockets();
        for (size_t j = 0; j < sockets.size(); j++) {
            int listen_fd = sockets[j];
            setNonBlocking(listen_fd);

            struct epoll_event ev;
            memset(&ev, 0, sizeof(ev));
            ev.events = EPOLLIN;
            ev.data.fd = listen_fd;

            if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
                perror("epoll_ctl: listen_fd");
                return 1;
            }

            // Guardar info en map
            PollData pd;
            pd.is_listener = true;
            pd.server_index = i;
            pd._conn = NULL;
            fd_map[listen_fd] = pd;

            std::cout << "✓ Server listening on fd " << listen_fd << std::endl;
        }
    }

    // Bucle principal
    const int MAX_EVENTS = 1024;
    epoll_event events[MAX_EVENTS];


	while (true) {
		int n = epoll_wait(epoll_fd, events, MAX_EVENTS, 1000);
		if (n == -1) {
			std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
		}

		for (int i = 0; i < n; i++) {
            int fd = events[i].data.fd;
			if (fd_map.find(fd) == fd_map.end())
				continue;
			
			PollData &pd = fd_map[fd];

            if (events[i].events & EPOLLIN) {
                if (pd.is_listener) {
                    // Listener -> aceptar cliente
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(fd, (sockaddr*)&client_addr, &client_len);
                    if (client_fd == -1) {
                        perror("accept");
                        continue;
                    }
                    setNonBlocking(client_fd);

                    struct epoll_event client_ev;
                    memset(&client_ev, 0, sizeof(client_ev));
                    client_ev.events = EPOLLIN | EPOLLET;
                    client_ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &client_ev);

                    // Guardar info de cliente en map
                    PollData client_pd;
                    client_pd.is_listener = false;
                    client_pd.server_index = pd.server_index;
                    client_pd._conn = new Connection(servers[pd.server_index]);
                    fd_map[client_fd] = client_pd;

                    std::cout << "✓ New client " << client_fd << " accepted on server fd " << fd << std::endl;
                } else {
					Connection* conn = pd._conn;
					conn->setFd(fd);
					// Recibir y preparar request
					if (!conn->recieveRequest()
						|| !conn->saveRequest()
						|| !conn->prepareRequest()) {
						close(fd);
						epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
						delete conn;
						fd_map.erase(fd);
						continue;
					}
					std::string method = conn->getHeader("Method");


					if (conn->isCgiScript())
						conn->sendCgiResponse();
					if (method == "GET") {
						conn->sendGetResponse();
					} else if (method == "POST") {
						conn->sendPostResponse();
					} else if (method == "DELETE") {
						conn->sendDeleteResponse();
					}

					// Cerrar fd y limpiar map
					close(fd);
					epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, 0);
					delete conn;
					fd_map.erase(fd);

                }
            }
        }
	}
   
	return (0);
}
