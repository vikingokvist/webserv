/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/11 11:45:29 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Connection.hpp"
#include "../includes/ErrorResponse.hpp"


Connection::Connection(ServerWrapper& _server) : _server(_server) {
	
	this->_fd = this->_server.getFD();
}


void			Connection::sendGetResponse() {
	
	std::ostringstream body_stream;
	body_stream << getFile().rdbuf();
	std::string body = body_stream.str();

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
	oss << "Content-Type: " << getContentType(_full_path) << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n\r\n";
	oss << body;

	std::string response = oss.str();
	send(getFd(), response.c_str(), response.size(), 0);
	close(getFd());
}

void			Connection::sendPostResponse() {
	
	// Procesar datos del formulario si es necesario
	std::string nombre = getHeader("nombre");
	std::string email = getHeader("email");
	std::string mensaje = getHeader("mensaje");

	// Redirigir al cliente a index.html
	std::ostringstream oss;
	oss << "HTTP/1.1 303 See Other\r\n";
	oss << "Location: "<< getHeader("Path") << "\r\n";
	oss << "Content-Length: 0\r\n";
	oss << "Connection: close\r\n\r\n";

	std::string response = oss.str();
	send(getFd(), response.c_str(), response.size(), 0);
	close(getFd());
}

ssize_t			Connection::getBestMatch(ServerWrapper& server, std::string req_path) {
	
	size_t max_match_len = 0;
	ssize_t best_match = -1;
	
	for (size_t j = 0; j < server.getLocations().size(); ++j) {	
		
		const std::string& loc_path = server.getLocationPath(j);
		if (req_path.rfind(loc_path, 0) == 0 && loc_path.size() > max_match_len) {

			max_match_len = loc_path.size();
			best_match = j;
		}
	}
	return (best_match);
}

bool	Connection::isMethodAllowed(Connection& connection, const std::string& method) {
	
	const std::vector<LocationConfig>& locations = connection.getServer().getLocations();
	std::string path = connection.getHeader("Path");
	
	for (size_t j = 0; j < locations.size(); ++j) {
		
		if (path.find(locations[j].path) == 0) {
			
			if (locations[j].methods.empty())
				return (true);
			if (locations[j].methods.count(method))
				return (true);
			else
				return (false);
		}
	}
	return (false);
}

std::string		Connection::getContentType(const std::string& path) {
	
	if (path.find(".css") != std::string::npos)
		return ("text/css");
	if (path.find(".html") != std::string::npos)
		return ("text/html");
	if (path.find(".js") != std::string::npos)
		return ("application/javascript");
	if (path.find(".png") != std::string::npos)
		return ("image/png");
	return ("text/plain");
}

int				Connection::getFd() {return (this->_fd);}

char*			Connection::getRequest() {return (this->_request);}

std::string		Connection::getHeader(std::string index) {return (this->_headers[index]);}

std::ifstream&	Connection::getFile() {return (this->_file);}

std::string		Connection::getFullPath() {return (this->_full_path);}

ServerWrapper&	Connection::getServer() {return (this->_server);}

void			Connection::setFd(int fd) {this->_fd = fd;}

void			Connection::setHeader(std::string index,std::string value) {this->_headers[index] = value;}

void			Connection::setFullPath(const std::string& full_path) {this->_full_path = full_path;}

void			Connection::send400Response() {ErrorResponse::send400(getFd());}

void			Connection::send403Response() {ErrorResponse::send403(getFd());}

void			Connection::send404Response() {ErrorResponse::send404(getFd());}

void			Connection::send405Response() {ErrorResponse::send405(getFd());}

void			Connection::send505Response() {ErrorResponse::send505(getFd());}

/* void Connection::sendDeleteResponse() {
	std::ostringstream body_stream;
	body_stream << getFile().rdbuf();
	std::string body = body_stream.str();

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
	oss << "Content-Type: " << getContentType(getHeader("Path")) << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n\r\n";
	oss << body;

	std::string response = oss.str();
	send(getFd(), response.c_str(), response.size(), 0);
	close(getFd());
} */