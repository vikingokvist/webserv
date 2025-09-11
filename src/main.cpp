#include "../includes/webserv.hpp"
#include "../includes/Connection.hpp"
#include "../includes/Servers.hpp"


int main(int argc, char **argv)
{
	if (argc != 2) {

		std::cout << "Error: Configuration file Missing" << std::endl;
		return (1);
	}
   	Servers servers(argv[1]);

    // Setup all servers
    for (size_t i = 0; i < servers.size(); i++) {

        servers[i].setPort(servers[i].getPorts(0));
        servers[i].setHost(inet_addr(servers[i].getIps(0).c_str()));
        servers[i].setMaxClientSize(servers[i].getClientMaxBodySize());
        servers[i].setLocations(servers[i].getLocations());
        servers[i].setupSocket();
        servers[i].setServerName(servers[i].getServerNamesList());
        servers[i].setupServerConfig(servers[i].getServerName(), htons(servers[i].getPort()),
                      servers[i].getHost(), AF_INET, servers[i].getMaxClientSize());
        servers[i].bindAndListen();
    }

	std::vector<pollfd> pollfds;
    for (size_t i = 0; i < servers.size(); i++) {
        pollfd pfd;
        pfd.fd = servers[i].getSocket();
        pfd.events = POLLIN;
        pfd.revents = 0;
        pollfds.push_back(pfd);
    }

	while (true) 
	{
		// Wait for events on all server sockets
        int ret = poll(pollfds.data(), pollfds.size(), -1);
        if (ret == -1) {
            std::cerr << "poll() failed: " << strerror(errno) << std::endl;
            continue;
        }
		for (size_t i = 0; i < pollfds.size(); i++) {

			if (pollfds[i].revents & POLLIN) {

				Connection _connection(servers[i]);
				std::cout << "Server: " << i << std::endl;
				if (_connection.setConnection(servers[i])) {

					std::string req_path = _connection.getHeader("Path");
					ssize_t best_match = _connection.getBestMatch(servers[i], req_path);
					if (best_match != -1) {
						if (_connection.receiveRequest(best_match)) {
							if (_connection.isMethodAllowed(_connection, _connection.getHeader("Method"))) {

								std::string method = _connection.getHeader("Method");
								if (method == "GET")
									_connection.sendGetResponse();
								else if (method == "POST")
									_connection.sendPostResponse();
								else
									_connection.send405Response();
							} else {
								_connection.send405Response();
							}
						}
					} else {
						_connection.send404Response();
					}
				}
				pollfds[i].revents = 0;
			}
		}
	}
	for (size_t i = 0; i < servers.size(); i++) {
        close(servers[i].getSocket());
    }
	return (0);
}
