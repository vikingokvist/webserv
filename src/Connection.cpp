/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/16 13:28:36 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Connection.hpp"

Connection::Connection(ServerWrapper& _server) : _server(_server) {this->_fd = this->_server.getFD();}

Connection::~Connection() {}


bool			Connection::setConnection(ServerWrapper& _server, int listening_fd) {
	this->_fd = accept(listening_fd, (struct sockaddr*)_server.getSockAddr(), (socklen_t*)_server.getSockAddr());
	if (this->_fd < 0) {
		
		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
	}
	if (!recieveRequest()) // Recibimos la Request
		return (false);
	if (!saveRequest(getRequest())) // Guardamos la Request
		return (false);
	return (true);
}


bool			Connection::recieveRequest() {
	
	int bytes_received = recv(getFd(), this->_request, sizeof(this->_request) - 1, 0);
	if (bytes_received < 0) {
		std::cerr << "Error in recv()" << std::endl;
		return (false);
	}
	this->_request[bytes_received] = '\0';
	return (true);
}


bool			Connection::saveRequest(char *_request) {
	
	std::string			request(_request);
	size_t				header_end = request.find("\r\n\r\n");
	std::istringstream	iss(request);
	std::string			line;
	std::string			post_body;

	if (header_end == std::string::npos)
		return (send400Response(), false);
		
	post_body = request.substr(header_end + 4);
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
			continue ;
		
		std::string key			 = line.substr(0, colon_pos);
		std::string value		 = line.substr(colon_pos + 1);
		size_t		boundary_pos = line.find("boundary=");
		
		if (boundary_pos == std::string::npos) {
			
			removeSpaces(key, value);
			this->_headers[key] = value;
		}
		else {
			
  			size_t semicolon_pos       = line.find(';', colon_pos);
    		std::string content_type   = line.substr(colon_pos + 1, semicolon_pos - (colon_pos + 1));
			removeSpaces(content_type, content_type);
    		this->_headers[key] = content_type;
    		this->_headers["Boundary"] = line.substr(boundary_pos + 9);
		}
	}
	printParserHeader();
	if (!post_body.empty())
		return (savePostBodyFile(post_body));
	return (true);
}



bool	Connection::savePostBodyFile(std::string post_body) {

	std::string boundary_marker = "--" + this->_headers["Boundary"];
	std::size_t filename_pos = post_body.find("filename=\"");
	
	if (filename_pos == std::string::npos) {
	    std::cerr << "Error: No filename found" << std::endl;
	    return (false);
	}
	
	filename_pos += 10;
	std::size_t filename_end = post_body.find("\"", filename_pos);
	std::string filename = post_body.substr(filename_pos, filename_end - filename_pos);
	std::size_t content_start = post_body.find("\r\n\r\n", filename_end);
	
	if (content_start == std::string::npos) {
	    std::cerr << "Error: Could not find start of content" << std::endl;
	    return (false);
	}

	content_start += 4;
	std::size_t content_end = post_body.find(boundary_marker, content_start);
	
	if (content_end == std::string::npos)
	    content_end = post_body.length();
		
	std::string file_content = post_body.substr(content_start, content_end - content_start);
	
	this->_post_body = file_content;
	this->_post_body_file_name = filename;
	return (true);
}

void Connection::printParserHeader(void) {
	
    std::cout << "\033[32m -----------[REQUEST]-----------\033[0m" << std::endl << std::endl;
    std::map<std::string, std::string>::const_iterator it;
    for (it = this->_headers.begin(); it != this->_headers.end(); ++it) {
        std::cout << "\033[32m[" << it->first << "] = " << it->second << "\033[0m" << std::endl;
    }
    std::cout << "\033[32m--------------------------------\033[0m" << std::endl;
}


bool			Connection::prepareRequest() {
	
    std::string req_path = this->_headers["Path"]; // Ej: "/index/estilos.css"
	ServerWrapper& server = this->_server;
	ssize_t best_match = getBestMatch(server, req_path);
	LocationConfig _location = server.getLocation(best_match);
    std::string root = _location.root;       // "www/"

	setBestMatch(best_match);
	if (getBestMatch() == -1) {
		std::cout << "Error Best Match" << std::endl;
		return (false);
	}
	
    // Si la ruta es exactamente igual a la location o termina con '/'
    if (req_path == server.getLocationPath(getBestMatch()) || req_path == server.getLocationPath(getBestMatch()) + "/") {
		if (_location.indices.size() != 0) {
			_previus_full_path = _headers["Path"];
			this->_full_path = root + _location.indices[0];
		}
		else {
			this->_full_path = _previus_full_path;
		}

    }
	
	
	
    // Verificar archivo

/* 	if (isDirectory(root.c_str()) && _headers["Method"] != "POST") {

		bool found_index = false;
		for (size_t i = 0; i < server.getLocationIndexCount(getBestMatch()); i++) {

			std::string index_path = root + server.getLocationIndexFile(getBestMatch(), i);
			if (_previus_full_path == "test1/upload.html")
				index_path = _previus_full_path;
			if (fileExistsAndReadable(index_path.c_str())) {
				found_index = true;
				break ;
			}
		}
		if (!found_index && server.getAutoIndex(getBestMatch()) == true)
			return (SendAutoResponse(root), true);	
		else if (!found_index && server.getAutoIndex(getBestMatch()) == false)
			return (send403Response(), false);
	} */
/*     else if (!fileExistsAndReadable(_full_path.c_str()) || !checkRequest()) {
        send404Response();
        return (false);
    } */ 

	_file.open(_full_path.c_str());

	// if (!_file || !checkRequest()) 
	// 	return (send404Response(), false); 
    return (true);
}


bool			Connection::checkRequest() {
	
	if (_headers["Host"].empty()) {
		
		send400Response();
	}
	if (_headers["Method"] != "GET" && _headers["Method"] != "POST" && _headers["Method"] != "DELETE" ) {
		
		send405Response();
		return (false);
	}
	if (!isValidHttpVersion(_headers["Version"])) {
		
		std::cerr << "Unsupported HTTP version: " << _headers["Version"] << std::endl;
		send505Response();
		return (false);
	}
	return (true);
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

	std::ostringstream body_stream;
	body_stream << getFile().rdbuf();
	std::string body = body_stream.str();
	

	// Redirigir al cliente a index.html
	std::ostringstream oss;
	oss << "HTTP/1.1 303 See Other\r\n";
	oss << "Location: "<< _previus_full_path  << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n\r\n";

	std::string response = oss.str();
	send(getFd(), response.c_str(), response.size(), 0);
	close(getFd());
}


void			Connection::SendAutoResponse(const std::string &direction_path) {

	DIR * dir = opendir(direction_path.c_str());
	if (!dir) {
		send403Response();
		return ;
	}
	std::ostringstream body;
    body << "<html><head><title>Index of " << _headers["Path"] << "</title></head><body>";
    body << "<h1>Index of " << _headers["Path"] << "</h1><ul>";

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        std::string name = entry->d_name;

        // Skip "." and ".."
        if (name == "." || name == "..")
            continue;

        std::string href = _headers["Path"];
        if (href.empty() || href[href.size()-1] != '/')
            href += "/";
        href += name;

        // Check if directory to add trailing slash
        struct stat st;
        std::string fullPath = direction_path + "/" + name;
        if (stat(fullPath.c_str(), &st) == 0 && S_ISDIR(st.st_mode)) {
            name += "/";
            href += "/";
        }

        body << "<li><a href=\"" << href << "\">" << name << "</a></li>\n";
    }

    body << "</ul></body></html>";
    closedir(dir);

    std::string bodyStr = body.str();
    std::ostringstream response;
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: text/html\r\n";
    response << "Content-Length: " << bodyStr.size() << "\r\n";
    response << "Connection: close\r\n\r\n";
    response << bodyStr;

    send(getFd(), response.str().c_str(), response.str().size(), 0);
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

void		Connection::removeSpaces(std::string& str1, std::string& str2) {

	while (!str1.empty() && isspace(str1[str1.size() - 1]))
		str1.erase(str1.size() - 1);

	while (!str2.empty() && isspace(str2[0]))
		str2.erase(0, 1);

}

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

void			Connection::send200Response() { ErrorResponse::send200(getFd(), *this); }

void			Connection::send201Response() { ErrorResponse::send201(getFd(), *this); }

void			Connection::send204Response() { ErrorResponse::send204(getFd(), *this); }

void			Connection::send301Response() { ErrorResponse::send301(getFd(), *this); }

void			Connection::send302Response() { ErrorResponse::send302(getFd(), *this); }

void			Connection::send400Response() { ErrorResponse::send400(getFd(), *this); }

void			Connection::send401Response() { ErrorResponse::send401(getFd(), *this); }

void			Connection::send403Response() { ErrorResponse::send403(getFd(), *this); }

void			Connection::send404Response() { ErrorResponse::send404(getFd(), *this); }

void			Connection::send405Response() { ErrorResponse::send405(getFd(), *this); }

void			Connection::send413Response() { ErrorResponse::send413(getFd(), *this); }

void			Connection::send414Response() { ErrorResponse::send414(getFd(), *this); }

void			Connection::send500Response() { ErrorResponse::send500(getFd(), *this); }

void			Connection::send501Response() { ErrorResponse::send501(getFd(), *this); }

void			Connection::send502Response() { ErrorResponse::send502(getFd(), *this); }

void			Connection::send503Response() { ErrorResponse::send503(getFd(), *this); }

void			Connection::send504Response() { ErrorResponse::send504(getFd(), *this); }

void			Connection::send505Response() { ErrorResponse::send505(getFd(), *this); }


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