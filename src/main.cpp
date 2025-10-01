#include "../includes/webserv.hpp"
#include "../includes/HttpReceive.hpp"
#include "../includes/Servers.hpp"
#include "../includes/Connection.hpp"


std::string _previous_full_path = "";


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

		for (int i = 0; i < ready_fds; i++) {

            int fd = conn.getEpollEvent(i).data.fd;
			if (conn.getFdMap().find(fd) == conn.getFdMap().end())
				continue ;
			
			PollData &pd = conn.getFdMap()[fd];
            if (conn.getEpollEvent(i).events & EPOLLIN) {

                if (pd.is_listener) {

                    if (conn.acceptClient(servers, fd, pd) == -1)
                        continue ;
                }
                else {

					pd.client->setFd(fd);
					if (!pd.client->receiveRequest() || !pd.client->saveRequest() || !pd.client->prepareRequest()) {
                        conn.removeClient(pd);
						continue ;
					}

					std::string method = pd.client->getHeader("Method");

					if (pd.client->isCgiScript())
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
