/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/24 15:28:25 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Connection.hpp"

Connection::Connection(ServerWrapper& _server) : _server(_server) {
	
	this->_fd = this->_server.getFD();
	this->_is_cgi_script = false;
}

Connection::~Connection() {}


bool			Connection::setConnection(ServerWrapper& _server, int listening_fd) {
	this->_fd = accept(listening_fd, (struct sockaddr*)_server.getSockAddr(), (socklen_t*)_server.getSockAddr());
	if (this->_fd < 0) {
		
		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
	}
	if (!recieveRequest()) // Recibimos la Request
		return (false);
	if (!saveRequest()) // Guardamos la Request
		return (false);
	return (true);
}


bool	Connection::recieveRequest() {
	
	this->_request_complete.clear();
	char buffer[8192];
	int bytes_received;
	int content_length = 0;

	// Leer headers + primer bloque
	bytes_received = recv(getFd(), buffer, sizeof(buffer), 0);
	if (bytes_received <= 0) return false;
	this->_request_complete.append(buffer, bytes_received);
	

	// Buscar Content-Length
	size_t cl_pos = this->_request_complete.find("Content-Length:");
	if (cl_pos != std::string::npos) {
		size_t cl_end = this->_request_complete.find("\r\n", cl_pos);
		std::string cl_str = this->_request_complete.substr(cl_pos + 15, cl_end - (cl_pos + 15));
		content_length = atoi(cl_str.c_str());
		std::cout << "Content-Length: " << content_length << std::endl;
	}

	size_t header_end = this->_request_complete.find("\r\n\r\n");
	size_t body_received = (this->_request_complete.size() > header_end + 4) ? _request_complete.size() - (header_end + 4) : 0;
	
	// Leer todo el body en bloques
	while ((int)body_received < content_length) {
		
		bytes_received = recv(getFd(), buffer, sizeof(buffer), 0);
		if (bytes_received <= 0) break;
		this->_request_complete.append(buffer, bytes_received);
		body_received += bytes_received;
	}
	
	std::cout << "Total received: " << _request_complete.size() << " bytes\n";
	return (true);
}


bool			Connection::saveRequest() {
	
	std::string			request(this->_request_complete);
	size_t				header_end = request.find("\r\n\r\n");
	std::istringstream	iss(request);
	std::string			line;


	if (header_end == std::string::npos)
		return (sendError(400));
	this->_post_body = request.substr(header_end + 4);
	request   = request.substr(0, header_end);
	
	if (!std::getline(iss, line) || line.empty())
		return (send400Response(), false);

	std::istringstream request_line(line);
	std::string method, path, version;
	request_line >> method >> path >> version;

	this->_headers["Method"]  = method;
	this->_headers["Path"]	  = path;
	this->_headers["Version"] = version;

	while (std::getline(iss, line)) {

		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.empty())
			break ;
		
		size_t		colon_pos    = line.find(':');
		if (colon_pos == std::string::npos)
			return (sendError(400));
		
		std::string key			 = line.substr(0, colon_pos);
		std::string value		 = line.substr(colon_pos + 1);
		size_t		boundary_pos = line.find("boundary=");
		
		
		if (boundary_pos == std::string::npos) {
			removeSpaces(key, value);
			if (this->_headers.find(key) != this->_headers.end() || !isValidHeaderName(key) || !isValidHeaderValue(value))
				return (sendError(400));
			this->_headers[key] = value;
		}
		else {
  			size_t semicolon_pos       = line.find(';', colon_pos);
			std::string content_type   = line.substr(colon_pos + 1, semicolon_pos - (colon_pos + 1));
			std::string boundary_value = line.substr(boundary_pos + 9);
			removeSpaces(content_type, content_type);
			if (this->_headers.find(key) != this->_headers.end() || !isValidHeaderName(key) || !isValidHeaderValue(content_type))
				return (sendError(400));
			this->_headers[key] = content_type;
			if (this->_headers.find("Boundary") != this->_headers.end() || !isValidHeaderValue(boundary_value))
				return (sendError(400));
			this->_headers["Boundary"] = boundary_value;
		}
	}
	if (this->_headers["Content-Type"] == "application/json")
		std::cout << "Not implemented yet" << std::endl;
	else if (this->_headers["Content-Type"] == "multipart/form-data")
		this->parts = parseMultipart(this->_post_body, this->_headers["Boundary"]);
	return (true);
}

bool			Connection::prepareRequest() {
	
	std::string				 req_path   = this->_headers["Path"];
	ServerWrapper&			 server	   = this->_server;
	ssize_t					 best_match = findBestMatch(server, req_path);
	LocationConfig			 _location;
	std::vector<std::string> index_files;
	std::string				 root;
	std::string				 relative_path;
	
	if (best_match == -1) 
		return (sendError(404));
		
	_location = server.getLocation(best_match);
	setBestMatch(best_match);
	
	root = _location.root.empty() ? server.getDefaultRoot() : _location.root;
	index_files = _location.indices.empty() ? server.getDefaultIndices() : server.getLocationIndices(best_match);
	
	this->_headers["Root"] = root;
	if (req_path == server.getLocationPath(getBestMatch()) || req_path == server.getLocationPath(getBestMatch()) + "/") {
		if (index_files.size() != 0) {
			_previous_full_path = this->_headers["Path"];
			for (size_t i = 0; i < index_files.size(); i++) {
				std::string found_path = root + index_files[i];
				if (fileExistsAndReadable(found_path.c_str(), 0)) {
					this->_full_path = found_path;
					break ;
				}
			}
		}
		else {
			this->_full_path = _previous_full_path;
		}
	}
	else {
		relative_path = req_path.substr(server.getLocationPath(getBestMatch()).size());
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path.erase(0, 1);
		this->_full_path = root + relative_path;
	}
	printParserHeader();
	return (checkRequest(server, root, best_match));
}



bool			Connection::checkRequest(ServerWrapper&	server, std::string root, ssize_t best_match) {

 	if (this->_headers["Method"].empty() || this->_headers["Path"].empty() || this->_headers["Version"].empty() || this->_headers["Host"].empty())
		return (sendError(400));
	if (!this->_headers["Method"].empty() && (this->_headers["Method"] != "GET" && this->_headers["Method"] != "POST" && this->_headers["Method"] != "DELETE"))
		return (sendError(501));
	if (this->_headers["Path"].size() >= MAX_URI_SIZE)
		return (sendError(414));
	if (this->_headers["Path"].find("../") != std::string::npos)
		return (sendError(403));
	if (!isValidHttpVersion(this->_headers["Version"]))
		return (sendError(505));
	if (!this->_headers["Content-Length"].empty() && !isNumber(this->_headers["Content-Length"]))
		return (sendError(400));
	if (this->_headers["Method"] == "POST" && this->_headers["Content-Length"].empty())
		return (sendError(411));
	if (checkContentLength(this->_headers["Content-Length"].c_str(), server.getMaxClientSize()) == -1)
		return (sendError(413));
	if (this->_headers["Method"] == "POST" && this->_headers["Content-Type"].empty())
		return (sendError(415));
	if (this->_headers["Method"] == "GET") {

		if (!isMethodAllowed(server, best_match, this->_headers["Method"]))
			return (sendError(405));
		if (isDirectory(root.c_str())) {

			if (this->_full_path.empty() && server.getAutoIndex(best_match) == true)
				return (sendAutoResponse(root), true);
			else if (this->_full_path.empty() && server.getAutoIndex(best_match) == false)
				return (sendError(404));
			if (!fileExistsAndReadable(this->_full_path.c_str(), 1))
				return (false);
			this->_file.open(this->_full_path.c_str());
			if (!this->_file)
				return (sendError(404)); 
			return (true);
		}
		if (!fileExistsAndReadable(this->_full_path.c_str(), 1))
			return (false);
		this->_file.open(this->_full_path.c_str());
		if (!this->_file) 
			return (sendError(404));
	}
	else if (!this->_headers["Boundary"].empty() && this->_headers["Method"] == "POST") {
		
		if (isMethodAllowed(server, best_match, this->_headers["Method"]) == false)
			return (sendError(405));
		
		for (size_t i = 0; i < this-> parts.size(); ++i) {
			std::string full_path = root + this->parts[i].filename;
			std::ofstream file_post(full_path.c_str());
			if (!file_post)
				return (std::cerr << ERROR_CREATE_FILE << full_path << std::endl, false);
			file_post.write(parts[i].content.data(), parts[i].content.size());
			file_post.close();
		}
	}
	else if (this->_headers["Method"] == "POST") {
		
		if (isMethodAllowed(server, best_match, this->_headers["Method"]) == false)
			return (sendError(405));
		size_t	extension_pos = this->_headers["Path"].find('.');
		if (extension_pos != std::string::npos) {
		
			std::string file_extension = this->_headers["Path"].substr(extension_pos);
			for (size_t i = 0; i < server.getCgiExtensionCount(best_match); i++)
				if (file_extension == server.getCgiExtensions(best_match, i))
					this->_is_cgi_script = true;
		}
	}
	else if (this->_headers["Method"] == "DELETE") {

		if (isMethodAllowed(server, best_match, this->_headers["Method"]) == false)
			return (sendError(405));
		// Intento de Path transversal
		// Intento de eliminar archivos para atras o archivos importantes como .conf
		if (this->_full_path.find("%2e%2e") != std::string::npos) {
				return (sendError(403));
		}

		// Si existe o no 
		if (!fileExistsAndReadable(this->_full_path.c_str(), 0)) {
			return (sendError(404));
		}

		// Eliminar
		if (remove(this->_full_path.c_str()) != 0) {
        	return (sendError(403));
		}
	}
	return (true);
}




bool			Connection::fileExistsAndReadable(const char* path, int mode) {

	if (path == NULL)
		return (false);
	struct stat st;
	if (stat(path, &st) != 0) {
		if (mode == 1) {
			sendError(404);
		}
		return (false);
	}
	if (!S_ISREG(st.st_mode)) {
		if (mode == 1) {
			sendError(404);
		}
		return (false);
	}
	if (access(path, R_OK) != 0) {

		if (mode == 1) {
			sendError(403);
		}
		return (false);
	}
	return (true);
}


bool	Connection::isMethodAllowed(ServerWrapper& server, ssize_t best_match, std::string& method) {
	
	if (server.getMethods(best_match).empty())
		return (true);
	for (size_t j = 0; j < server.getMethodsSize(best_match); j++) {
		
		if (method == server.getMethod(best_match, j))
			return (true);
	}
	return (false);
}

ssize_t			Connection::findBestMatch(ServerWrapper& server, std::string req_path) {
	
	size_t max_match_len = 0;
	ssize_t best_match = -1;

	for (size_t j = 0; j < server.getLocations().size(); j++) {	
		
		const std::string& loc_path = server.getLocationPath(j);
		if (req_path.rfind(loc_path, 0) == 0 && loc_path.size() > max_match_len) {
			
			max_match_len = loc_path.size();
			best_match = j;
		}
	}
	return (best_match);
}


void Connection::printParserHeader(void) {
	
	std::cout << "\033[32m -----------[REQUEST]-----------\033[0m" << std::endl << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = this->_headers.begin(); it != this->_headers.end(); ++it) {
		std::cout << "\033[32m[" << it->first << "] = " << it->second << "\033[0m" << std::endl;
	}
	std::cout << "\033[32m--------------------------------\033[0m" << std::endl;
}


bool		Connection::sendError(size_t error_code) {

	static Handler handlers[506] = {0};

	if (handlers[0] == 0) {
		handlers[201] = &Connection::send201Response;
		handlers[204] = &Connection::send204Response;
		handlers[301] = &Connection::send301Response;
		handlers[302] = &Connection::send302Response;
		handlers[400] = &Connection::send400Response;
		handlers[401] = &Connection::send401Response;
		handlers[403] = &Connection::send403Response;
		handlers[404] = &Connection::send404Response;
		handlers[405] = &Connection::send405Response;
		handlers[411] = &Connection::send411Response;
		handlers[413] = &Connection::send413Response;
		handlers[414] = &Connection::send414Response;
		handlers[500] = &Connection::send500Response;
		handlers[501] = &Connection::send501Response;
		handlers[502] = &Connection::send502Response;
		handlers[503] = &Connection::send503Response;
		handlers[504] = &Connection::send504Response;
		handlers[505] = &Connection::send505Response;
	}
	if (error_code < 506 && handlers[error_code])
		(this->*handlers[error_code])();
	return (false);
}

bool 			Connection::isCgiScript() {return (this->_is_cgi_script);}

ssize_t			Connection::getBestMatch() {return (_best_match);}

int				Connection::getFd() {return (this->_fd);}

char*			Connection::getRequest() {return (this->_request);}

std::string		Connection::getHeader(std::string index) {return (this->_headers[index]);}

std::ifstream&	Connection::getFile() {return (this->_file);}

std::string		Connection::getFullPath() {return (this->_full_path);}

ServerWrapper&	Connection::getServer() {return (this->_server);}

void			Connection::setBestMatch(ssize_t _best_match) {this->_best_match = _best_match;}

void			Connection::setFd(int fd) {this->_fd = fd;}

void			Connection::setHeader(std::string index,std::string value) {this->_headers[index] = value;}

void			Connection::setFullPath(const std::string& full_path) {this->_full_path = full_path;}

size_t			Connection::getPostBodySize() {return (this->_post_body.size());}

std::string		Connection::getPostBody() {return (this->_post_body);}

void			Connection::sendAutoResponse(const std::string &direction_path) {SendResponse::sendAutoResponse(getFd(), *this, direction_path); }
		
void			Connection::sendDeleteResponse() {SendResponse::sendDeleteResponse(getFd(), *this); }

void			Connection::sendGetResponse() {SendResponse::sendGetResponse(getFd(), *this); }

void			Connection::sendPostResponse() {SendResponse::sendPostResponse(getFd(), *this, _previous_full_path); }

void			Connection::sendCgiResponse() {SendResponse::sendCgiResponse(getFd(), *this); }

void			Connection::send200Response() { SendResponse::send200(getFd(), *this); }

void			Connection::send201Response() { SendResponse::send201(getFd(), *this); }

void			Connection::send204Response() { SendResponse::send204(getFd(), *this); }

void			Connection::send301Response() { SendResponse::send301(getFd(), *this); }

void			Connection::send302Response() { SendResponse::send302(getFd(), *this); }

void			Connection::send400Response() { SendResponse::send400(getFd(), *this); }

void			Connection::send401Response() { SendResponse::send401(getFd(), *this); }

void			Connection::send403Response() { SendResponse::send403(getFd(), *this); }

void			Connection::send404Response() { SendResponse::send404(getFd(), *this); }

void			Connection::send405Response() { SendResponse::send405(getFd(), *this); }

void			Connection::send411Response() { SendResponse::send411(getFd(), *this); }

void			Connection::send413Response() { SendResponse::send413(getFd(), *this); }

void			Connection::send414Response() { SendResponse::send414(getFd(), *this); }

void			Connection::send415Response() { SendResponse::send415(getFd(), *this); }

void			Connection::send500Response() { SendResponse::send500(getFd(), *this); }

void			Connection::send501Response() { SendResponse::send501(getFd(), *this); }

void			Connection::send502Response() { SendResponse::send502(getFd(), *this); }

void			Connection::send503Response() { SendResponse::send503(getFd(), *this); }

void			Connection::send504Response() { SendResponse::send504(getFd(), *this); }

void			Connection::send505Response() { SendResponse::send505(getFd(), *this); }


