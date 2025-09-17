/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/17 15:56:06 by ctommasi         ###   ########.fr       */
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
		return (sendError(400));
		
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



bool			Connection::prepareRequest() {
	
    std::string		req_path   = this->_headers["Path"];
	ServerWrapper&	server	   = this->_server;
	ssize_t			best_match = getBestMatch(server, req_path);
	LocationConfig	_location  = server.getLocation(best_match);
    std::string		root       = _location.root;
	std::string		relative_path;

	setBestMatch(best_match);
	if (getBestMatch() == -1) {
		std::cout << "Error Best Match" << std::endl;
		return (false);
	}
	
    if (req_path == server.getLocationPath(getBestMatch()) || req_path == server.getLocationPath(getBestMatch()) + "/") {
		if (_location.indices.size() != 0) {
			_previous_full_path = _headers["Path"];
			this->_full_path = root + _location.indices[0];
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
	return (checkRequest(server, root, best_match));
}

bool			Connection::fileExistsAndReadable(const char* path, int mode) {
	
    struct stat st;
	if (stat(path, &st) != 0) {
		if (mode)
        	sendError(404);
        return (false);
    }
	if (!S_ISREG(st.st_mode)) {
		if (mode)
        	sendError(404);
        return (false);
    }
	if (access(path, R_OK) != 0) {
		if (mode)
        	sendError(403);
        return (false);
    }
    return (true);
}

bool			Connection::checkRequest(ServerWrapper&	server, std::string root, ssize_t best_match) {
	
	if (this->_headers["Method"].empty() || this->_headers["Path"].empty() || this->_headers["Version"].empty() || this->_headers["Host"].empty())
		return (sendError(400));
	if (!this->_headers["Method"].empty() && (this->_headers["Method"] != "GET" && this->_headers["Method"] != "POST" && this->_headers["Method"] != "DELETE"))
		return (sendError(501));
	if (!isMethodAllowed(server, best_match, this->_headers["Method"]))
		return (sendError(405));
	if (this->_headers["Path"].size() >= MAX_URI_SIZE)
		return (sendError(414));
	if (this->_headers["Path"].find("../") != std::string::npos)
		return (sendError(403));
	if (!isValidHttpVersion(_headers["Version"]))
		return (sendError(505));
	if (!this->_headers["Content-Length"].empty() && !isNumber(this->_headers["Content-Length"]))
		return (sendError(400));
	if (this->_headers["Method"] == "POST" && this->_headers["Content-Length"].empty())
		return (sendError(411));
	if (checkContentLength(this->_headers["Content-Length"].c_str(), server.getMaxClientSize()) == -1)
		return (sendError(413));
	if (this->_headers["Method"] == "POST" && (!this->_headers["Content-Type"].empty() || this->_headers["Content-Type"] != "multipart/form-data"))
		return (sendError(415));
	if (isDirectory(root.c_str()) && this->_headers["Method"] != "POST") {

		bool found_index = false;
		for (size_t i = 0; i < server.getLocationIndexCount(best_match); i++) {

			std::string found_path = root + server.getLocationIndexFile(best_match, i);
			if (fileExistsAndReadable(found_path.c_str(), 0)) {
				found_index = true;
				break ;
			}
		}

		if (!found_index && server.getAutoIndex(best_match) == true)
			return (SendAutoResponse(root), true);
		else if (!found_index && server.getAutoIndex(best_match) == false)
			return (sendError(404));
    	if (!fileExistsAndReadable(this->_full_path.c_str(), 1))
			return (false);
		
		_file.open(this->_full_path.c_str());
		if (!_file)
			return (sendError(404)); 

		return (true);
	}
    if (!fileExistsAndReadable(this->_full_path.c_str(), 1))
		return (false);
	
	_file.open(this->_full_path.c_str());
	if (!_file) 
		return (sendError(404));
	return (true);
}

void			Connection::sendGetResponse() {
	
	std::ostringstream body_stream;
	body_stream << getFile().rdbuf();
	std::string body = body_stream.str();

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
	oss << "Content-Type: " << getContentType(this->_full_path) << "\r\n";
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
	oss << "Location: "<< _previous_full_path  << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n\r\n";

	std::string response = oss.str();
	send(getFd(), response.c_str(), response.size(), 0);
	close(getFd());
}


void			Connection::SendAutoResponse(const std::string &direction_path) {

	DIR * dir = opendir(direction_path.c_str());
	if (!dir) {
		sendError(403);
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


bool	Connection::isMethodAllowed(ServerWrapper& server, ssize_t best_match, std::string& method) {
	
	if (method.empty())
		return (false);
	for (size_t j = 0; j < server.getMethodsSize(best_match); ++j) {
		
		if (method == server.getMethod(best_match, j))
			return (true);
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

void Connection::printParserHeader(void) {
	
    std::cout << "\033[32m -----------[REQUEST]-----------\033[0m" << std::endl << std::endl;
    std::map<std::string, std::string>::const_iterator it;
    for (it = this->_headers.begin(); it != this->_headers.end(); ++it) {
        std::cout << "\033[32m[" << it->first << "] = " << it->second << "\033[0m" << std::endl;
    }
    std::cout << "\033[32m--------------------------------\033[0m" << std::endl;
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

void			Connection::send411Response() { ErrorResponse::send411(getFd(), *this); }

void			Connection::send413Response() { ErrorResponse::send413(getFd(), *this); }

void			Connection::send414Response() { ErrorResponse::send414(getFd(), *this); }

void			Connection::send415Response() { ErrorResponse::send415(getFd(), *this); }

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