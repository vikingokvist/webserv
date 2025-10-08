#include "../includes/webserv.hpp"
#include "../includes/HttpReceive.hpp"
#include "../includes/Servers.hpp"
#include "../includes/Connection.hpp"

Servers* g_servers = NULL;
Connection* g_conn = NULL;

void handle_sigint(int sig) {
	std::cout << "\nSeñal SIGINT recibida. Limpiando recursos...\n" << std::endl;

    if (g_conn) {
        g_conn->getFdMap().clear(); // limpiar clientes
    }

    if (g_servers) {
        g_servers->clearServers(); // cerrar sockets y limpiar vectores
    }

	(void)sig;
	std::cout << "Recursos liberados. Saliendo.\n" << std::endl;
    exit(0);
}

int main(int argc, char **argv)
{
	signal(SIGINT, handle_sigint);

	try
	{
		std::string conf_file = (argc < 2) ? DEFAULT_CONF_FILE : argv[1];
    	if (argc > 2)
        	return (std::cout << ERROR_ARGUMENTS << std::endl, 1);

        Servers servers(conf_file);
        g_servers = &servers;

        Connection conn(servers);
        g_conn = &conn;

		while (true) {

			int ready_fds = epoll_wait(conn.getEpollFd(), conn.getEpollEvents(), MAX_EVENTS, TIME_OUT);
			if (ready_fds == -1) {
				std::cerr << "epoll_wait failed: " << strerror(errno) << std::endl;
			}

			time_t now = std::time(0);
			conn.removeTimeoutClients(now);

			for (int i = 0; i < ready_fds; i++) {

    	        int fd = conn.getEpollEvent(i).data.fd;
				if (conn.getFdMap().find(fd) == conn.getFdMap().end())
					continue ;

				PollData &pd = conn.getFdMap()[fd];

    	        if (pd.is_listener && (conn.getEpollEvent(i).events & EPOLLIN)) {
					if (conn.acceptClient(servers, fd, pd) == -1)
						continue ;
				}
    	        else if (!pd.is_listener && (conn.getEpollEvent(i).events & EPOLLIN)) {

					RecvStatus status = pd.client->receiveRequest();

					if (status == RECV_PAYLOAD_TOO_LARGE_ERROR || status == RECV_ERROR || status == RECV_CLOSED) {
						if (status == RECV_PAYLOAD_TOO_LARGE_ERROR) pd.client->sendError(413);
						continue ;
					}
					else if (status == RECV_INCOMPLETE) {
						pd._current_time = std::time(0);
						continue ;
					}
					else if (status == RECV_COMPLETE) {
						
						if (!pd.client->prepareRequest() || !pd.client->checkRequest()) {
    	        	    	if (pd.client->getHeader("Connection") != "keep-alive") {
								conn.removeClient(pd);
							} else {
								pd._current_time = std::time(0);
							}
							continue ;
						}

						std::string method = pd.client->getHeader("Method");

						if (pd.client->isRedirection())
							pd.client->sendRedirectResponse();
						else if (pd.client->isCgiScript())
							pd.client->sendCgiResponse();
						else if (method == "GET")
							pd.client->sendGetResponse();
						else if (method == "POST")
							pd.client->sendPostResponse();
						else if (method == "DELETE")
							pd.client->sendDeleteResponse();

						if (pd.client->getHeader("Connection") != "keep-alive") {
							close(pd.fd);
							conn.removeClient(pd);
						}
						else if (pd.client->getHeader("Connection") == "keep-alive") {
							pd.client->resetForNextRequest();
							pd._current_time = std::time(0);
						}
					}
    	        }
    	    }
    	}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
	return (0);
}

