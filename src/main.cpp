#include "../includes/webserv.hpp"
#include "../includes/Connection.hpp"
#include "../includes/Servers.hpp"

std::string _previus_full_path = "VARIABLE NO DEFINIDA";

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

// Aqui Configuro A Cada socket individualmente su fd, event y revent 
void	PollConfig(Servers& servers, std::vector<PollData>& poll_data, std::vector<pollfd>& pollfds) {

    for (size_t i = 0; i < servers.size(); i++) {
		const std::vector<int> sockets = servers[i].getSockets();
		for (size_t j = 0; j < sockets.size(); j++) {
			std::cout << "Socket del Server: " << i << " Con Socket: " << sockets[j] << std::endl;
			poll_data.push_back(PollData(sockets[j], i));
		}
    }

	pollfds.resize(poll_data.size());
	for (size_t i = 0; i < poll_data.size(); i++) {
		pollfds[i].fd = poll_data[i].fd;
		pollfds[i].events = POLLIN;
		pollfds[i].revents = 0;
	}

}

int main(int argc, char **argv)
{
	if (argc != 2) {
		std::cout << "Error: Configuration file Missing" << std::endl;
		return (1);
	}
	try
	{
		Servers servers(argv[1]);

		// Setup all servers
		SetupAllServers(servers);

		std::vector<PollData> poll_data;
		std::vector<pollfd> pollfds;
		PollConfig(servers, poll_data, pollfds);

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

					size_t server_idx = poll_data[i].server_index;
					int listening_fd = pollfds[i].fd;
					Connection _connection(servers[server_idx]);
					std::cout << "Servidor: " << server_idx << " Conectado al socket: " << listening_fd << std::endl;

					if (_connection.setConnection(servers[server_idx], listening_fd)) {
						if (_connection.prepareRequest()) {
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
					}
					pollfds[i].revents = 0;
				}
			}
		}
		for (size_t i = 0; i < servers.size(); i++) {
    	    close(servers[i].getSocket());
    	}
	}
	catch(const std::exception& e)
	{
		std::cerr << e.what() << '\n';
	}
	
   
	return (0);
}
