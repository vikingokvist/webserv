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
	ServerWrapper& server = servers[0];

	// BUcle
	server.setServerName(server.getServerNamesList());
	server.setPort(server.getPorts(0));
	server.setHost(inet_addr(server.getIps(0).c_str()));
	server.setMaxClientSize(server.getClientMaxBodySize());
	server.setLocations(server.getLocations());

	server.setupSocket();
	server.setupServerConfig(server.getServerName(), htons(server.getPort())
							,server.getHost(), AF_INET, server.getMaxClientSize());
	server.bindAndListen();


	while (true) 
	{
		Connection _connection(server);

		if (!_connection.setConnection(server))
			continue ;

		// Buscar la location con el prefijo más largo que haga match
		std::string req_path = _connection.getHeader("Path");
		ssize_t best_match = _connection.getBestMatch(server, req_path);
		if (best_match != -1) {

			if (_connection.receiveRequest(best_match)) {

				if (_connection.isMethodAllowed(_connection, _connection.getHeader("Method"))) {

					std::string method = _connection.getHeader("Method");
					if (method == "GET")
						_connection.sendGetResponse();
					else if (method == "POST")
						_connection.sendPostResponse();
				}
				else {

					_connection.send405Response();
				}
			}
		}
		else {

			_connection.send404Response();
		}
	}
	close(server.getSocket());
}
