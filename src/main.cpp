#include "../includes/webserv.hpp"
#include "../includes/Connection.hpp"
#include "../includes/Servers.hpp"



bool isMethodAllowed(Connection& connection, const std::string& method) {
	const std::vector<LocationConfig>& locations = connection.getServer()->getLocations();
	std::string path = connection.getHeader("Path");
	for (size_t j = 0; j < locations.size(); ++j) {
		if (path.find(locations[j].path) == 0) {
			const std::set<std::string>& methods = locations[j].methods;
			if (methods.empty())
				return true;
			if (methods.count(method))
				return true;
			else
				return false;
		}
	}
	return false;
}

int main(int argc, char **argv)
{
	(void)argv;
	if (argc != 2) {

		std::cout << "falta el .conf" << std::endl;
		return (1);
	}

	// const std::string& name = "webserv.conf";
	Servers servers("webserv.conf");

	// ServerWrapper server = servers[0];


	// server.setServerName(server.getServerNamesList());
	// server.setPort(server.getPorts(0));
	// server.setHost(inet_addr(server.getIps(0).c_str()));
	// server.setMaxClientSize(server.getClientMaxBodySize());
	// // _server.setLocations(server.getLocations());

	// server.setupSocket();
	// server.setupServerConfig(server.getServerName(), htons(server.getPort())
	// 	,server.getHost(), AF_INET, server.getMaxClientSize());
	// server.bindAndListen();

	// while (true) 
	// {
	// 	Connection _connection;
	// 	_connection.setServer(&server);
	// 	if (!_connection.setConnection(server)) {
	// 		continue;
	// 	}
	// 	std::vector<LocationConfig> locations = _connection.getServer()->getLocations();
	// 	std::string req_path = _connection.getHeader("Path");

	// 	// Buscar la location con el prefijo más largo que haga match
	// 	const LocationConfig* best_match = 0;
	// 	size_t max_match_len = 0;
	
	// 	for (size_t j = 0; j < locations.size(); ++j) {
	// 		const std::string& loc_path = locations[j].path;
	// 		if (req_path.find(loc_path) == 0) {
	// 			best_match = &locations[j];
	// 			max_match_len = loc_path.size();
	// 		}
	// 	}
		
	// 	if (best_match) {
	// 		if (_connection.receiveRequest(*best_match)) {
	// 			if (isMethodAllowed(_connection, _connection.getHeader("Method"))) {
	// 				std::string method = _connection.getHeader("Method");
	// 				if (method == "GET")
	// 					_connection.sendGetResponse();
	// 				else if (method == "POST")
	// 					_connection.sendPostResponse();
	// 			} else {
	// 				_connection.send405Response();
	// 			}
	// 		}
	// 	} else {
	// 		_connection.send404Response();
	// 	}
	// }
	
	// close(server.getSocket());
}
