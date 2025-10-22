#include "../includes/webserv.hpp"
#include "../includes/HttpReceive.hpp"
#include "../includes/Servers.hpp"
#include "../includes/Connection.hpp"
#include "../includes/Logger.hpp"

Servers *servers = NULL;
Connection *conn = NULL;

volatile sig_atomic_t g_stop = 0;

void handle_sigint(int sig)
{
	g_stop = 1;
	(void)sig;
}

int free_all()
{
	if (conn)
	{
		std::vector<int> fds_to_remove;
		std::map<int, PollData>::iterator it = conn->getFdMap().begin();

		for (; it != conn->getFdMap().end(); ++it)
		{
			fds_to_remove.push_back(it->first);
		}
		for (size_t i = 0; i < fds_to_remove.size(); i++)
		{
			int fd = fds_to_remove[i];
			std::map<int, PollData>::iterator it = conn->getFdMap().find(fd);
			if (it == conn->getFdMap().end())
				continue;

			PollData &pd = it->second;
			if (pd.is_listener)
			{
				epoll_ctl(conn->getEpollFd(), EPOLL_CTL_DEL, fd, NULL);
				close(fd);
			}
			else
				conn->removeClient(pd);
		}
		close(conn->getEpollFd());
		delete conn;
		conn = NULL;
	}
	if (servers)
	{
		Logger::printSignalReceived(*servers);
		delete servers;
		servers = NULL;
	}
	return (0);
}

int main(int argc, char **argv)
{

	try
	{
		std::string conf_file = (argc < 2) ? DEFAULT_CONF_FILE : argv[1];
		if (argc > 2)
			return (std::cout << ERROR_ARGUMENTS << std::endl, 1);

		servers = new Servers(conf_file);
		conn = new Connection(*servers);
		
		signal(SIGINT, handle_sigint);
		while (!g_stop)
		{

			int ready_fds = epoll_wait(conn->getEpollFd(), conn->getEpollEvents(), MAX_EVENTS, EPOLL_TIME_OUT);
			if (ready_fds == -1)
				break;
			conn->removeTimeoutClients(std::time(0));
			for (int i = 0; i < ready_fds; i++)
			{
				int fd = conn->getEpollEvent(i).data.fd;
				if (conn->getFdMap().find(fd) == conn->getFdMap().end())
					continue;

				PollData &pd = conn->getFdMap()[fd];
				if (pd.is_listener && (conn->getEpollEvent(i).events & EPOLLIN))
				{
					if (conn->acceptClient(*servers, fd, pd) == -1)
						continue;
				}
				else if (!pd.is_listener && (conn->getEpollEvent(i).events & EPOLLIN))
				{
					RecvStatus status = pd.client->receiveRequest();

					if (status == RECV_PAYLOAD_TOO_LARGE_ERROR || status == RECV_ERROR || status == RECV_CLOSED)
					{
						if (status == RECV_PAYLOAD_TOO_LARGE_ERROR)
							pd.client->sendError(413);
						conn->removeClient(pd);
						continue;
					}
					else if (status == RECV_INCOMPLETE)
					{
						pd._current_time = std::time(0);
						continue;
					}
					else if (status == RECV_COMPLETE)
					{
						if (!pd.client->prepareRequest() || !pd.client->checkRequest())
						{
							pd.client->sendError(pd.error_code);
							continue;
						}
						pd._current_time = std::time(0);
						conn->modifyEpollEvent(fd, EPOLLOUT);
					}
				}
				else if (!pd.is_listener && (conn->getEpollEvent(i).events & EPOLLOUT))
				{
					std::string method = pd.client->getHeader("Method");
					bool send_error = true;

					if (!pd.has_error) {
						if (pd.client->getIsAutoIndex())
						{
							send_error = pd.client->sendAutoResponse(pd.client->getAutoIndex());
							if (!send_error)
								pd.client->sendOutErr(403);
						}
						else if (pd.client->isRedirection())
						{
							send_error = pd.client->sendRedirectResponse();
							if (!send_error)
								pd.client->sendOutErr(404);
						}
						else if (pd.client->isCgiScript())
						{
							send_error = pd.client->sendCgiResponse();
							if (!send_error)
								pd.client->sendOutErr(500);
						}			
						else if (method == "GET")
							send_error = pd.client->sendGetResponse();
						else if (method == "POST")
							send_error = pd.client->sendPostResponse();
						else if (method == "DELETE")
							send_error = pd.client->sendDeleteResponse();
						else if (method == "HEAD")
							send_error = pd.client->sendHeadResponse();

						if (pd.client->getHeader("Connection") != "keep-alive" || send_error == false)
							conn->removeClient(pd);
						else
						{
							pd.client->resetForNextRequest();
							pd._current_time = std::time(0);
							conn->modifyEpollEvent(fd, EPOLLIN | EPOLLET);
						}
					} else {
						pd.client->sendOutErr(pd.error_code);
						conn->removeClient(pd);
					}
				}
			}
		}
	}
	catch (const std::exception &e)
	{
		handle_sigint(1);
		std::cerr << e.what() << '\n';
	}
	return (free_all());
}
