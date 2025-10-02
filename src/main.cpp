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
		std::vector<int> fds_to_remove;

		for (std::map<int, PollData>::iterator it = conn.getFdMap().begin(); it != conn.getFdMap().end(); ++it) {
			PollData &pd = it->second;
			if (!pd.is_listener)
				std::cout << "fd: "<< pd.client->getFd() << " -> " << now - pd._start_time << " ms" << std::endl;
			if (!pd.is_listener && (now - pd._start_time >= CLIENT_TIME_OUT / 100)) {
				fds_to_remove.push_back(it->first);
			}
		}
		
		for (size_t i = 0; i < fds_to_remove.size(); i++) {
			std::map<int, PollData>::iterator it = conn.getFdMap().find(fds_to_remove[i]);
			if (it != conn.getFdMap().end()) {
				std::cout << "Cliente con fd: " << it->first << " ha sido eliminado " << std::endl;
				conn.removeClient(it->second);
			}
		}

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

				if (!pd.client->receiveRequest() || !pd.client->saveRequest() || !pd.client->prepareRequest()) {
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
	return (0);
}