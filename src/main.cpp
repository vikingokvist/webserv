#include "../includes/webserv.hpp"
#include "../includes/Connection.hpp"
#include "../includes/Servers.hpp"
#include "../includes/ServerSetUp.hpp"

#include <algorithm>

std::string _previous_full_path = "";


int main(int argc, char **argv)
{
    std::string conf_file = (argc < 2) ? DEFAULT_CONF_FILE : argv[1];
    if (argc > 2)
        return (std::cout << ERROR_ARGUMENTS << std::endl, 1);

    Servers servers(conf_file);
    ServerSetUp s_conf(servers);


	while (true) {

		int ready_fds = epoll_wait(s_conf.getEpollFd(), s_conf.getEpollEvents(), MAX_EVENTS, TIME_OUT);
		if (ready_fds == -1) {
			std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
		}

		for (int i = 0; i < ready_fds; i++) {

            int fd = s_conf.getEpollEvent(i).data.fd;
			if (s_conf.getFdMap().find(fd) == s_conf.getFdMap().end())
				continue ;
			
			PollData &pd = s_conf.getFdMap()[fd];

            if (s_conf.getEpollEvent(i).events & EPOLLIN) {

                if (pd.is_listener) {
                    // Listener -> aceptar cliente
                    s_conf.acceptClient(); // ME QUEDE AQUI
                    sockaddr_in client_addr;
                    socklen_t client_len = sizeof(client_addr);
                    int client_fd = accept(fd, (sockaddr*)&client_addr, &client_len);
                    if (client_fd == -1) {
                        perror("accept");
                        continue ;
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
