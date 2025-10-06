#include "../includes/webserv.hpp"
#include "../includes/HttpReceive.hpp"
#include "../includes/Servers.hpp"
#include "../includes/Connection.hpp"

int main(int argc, char **argv)
{
    std::string conf_file = (argc < 2) ? DEFAULT_CONF_FILE : argv[1];
    if (argc > 2)
        return (std::cout << ERROR_ARGUMENTS << std::endl, 1);

    Servers servers(conf_file);
    Connection conn(servers);

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

				if (status == RECV_PAYLOAD_TOO_LARGE_ERROR) {
					pd.client->sendError(413);
					conn.removeClient(pd);
					continue ;
				}
				else if (status == RECV_ERROR || status == RECV_CLOSED) {
					conn.removeClient(pd);
					continue ;
				}
				else if (status == RECV_INCOMPLETE) {
					pd._current_time = std::time(0);
					continue ;
				}
				else if (status == RECV_HEADER_COMPLETE && !pd.client->saveRequest()) {
					conn.removeClient(pd);
					continue ;
				}
				else if (status == RECV_COMPLETE) {

					if (!pd.client->prepareRequest() || !pd.client->checkRequest()) {
            	    	conn.removeClient(pd);
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
					conn.removeClient(pd);
				}

            }
        }
    }
	return (0);
}

