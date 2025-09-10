/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Request.cpp                                        :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/18 14:19:05 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/10 16:47:26 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/webserv.hpp"
#include "../includes/Connection.hpp"

bool			Connection::setConnection(ServerWrapper& _server) {
	

	this->_fd = accept(_server.getSocket(), (struct sockaddr*)_server.getSockAddr(), (socklen_t*)_server.getSockAddr());
	if ( this->_fd < 0) {
		
		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
	}
	if (!setRequest()) // Recibimos la Request
		return (false);
	if (!saveRequest(getRequest())) // Guardamos la Request
		return (false);
	return (true);
}

bool			Connection::setRequest() {
	
	int bytes_received = recv(getFd(), this->_request, sizeof(this->_request) - 1, 0);
	if (bytes_received < 0) {
		std::cerr << "Error in recv()" << std::endl;
		return (false);
	}
	this->_request[bytes_received] = '\0';
	return (true);
}

bool			Connection::saveRequest(char *request) {
	
	std::cout << "REQUEST\n" << std::endl;
	std::cout << "\033[32m" << request << "\033[0m" << std::endl;

	std::string full_request(request);
	std::size_t header_end = full_request.find("\r\n\r\n");
	if (header_end == std::string::npos) {
		
		send400Response();
		return (false);
	}

	std::string headers_part = full_request.substr(0, header_end);
	std::string body_part = full_request.substr(header_end + 4);
	std::istringstream iss(headers_part);
	std::string line;
	std::getline(iss, line);
	std::istringstream request_line(line);
	std::string method, path, version;
	
	if (!(request_line >> method >> path >> version)) {
		
		send400Response();
		return (false);
	}
	_headers["Method"] = method;
	_headers["Path"] = path;
	_headers["Version"] = version;
	
	while (std::getline(iss, line) && !line.empty()) {
		
		if (!line.empty() && line[line.length() - 1] == '\r') {
			line.erase(line.length() - 1);
		}
		size_t colon = line.find(":");
		
		if (colon != std::string::npos) {
			
			std::string key = line.substr(0, colon);
			std::string value = line.substr(colon + 1);
			value.erase(0, value.find_first_not_of(" \t"));
			_headers[key] = value;
		}
	}
	// Parse body if POST
	if (method == "POST") {
		
		std::istringstream body_iss(body_part);
		std::string pair;
		
		while (std::getline(body_iss, pair, '&')) {
			
			size_t pos = pair.find('=');
			if (pos != std::string::npos) {
				
				std::string key = pair.substr(0, pos);
				std::string value = pair.substr(pos + 1);
				_headers[key] = value;
			}
		}
	}
	return (true);
}



bool			Connection::receiveRequest(ssize_t location_index) {
	
    std::string req_path = _headers["Path"]; // Ej: "/index/estilos.css"
	ServerWrapper& server = this->_server;
	LocationConfig _location = server.getLocation(location_index);
    std::string root = _location.root;       // "www/"
    std::string relative_path;

    // Si la ruta es exactamente igual a la location o termina con '/'
    if (req_path == server.getLocationPath(location_index) || req_path == server.getLocationPath(location_index) + "/") {
		
        relative_path = _location.indices[0];  // "index.html" ARREGLAR LUEGO PARA MULTIPLES INDICES
    }
	else {
        // Obtener la parte después de "/index"
        relative_path = req_path.substr(server.getLocationPath(location_index).size());
        if (!relative_path.empty() && relative_path[0] == '/') {
			
            relative_path.erase(0, 1);  // Quita la barra inicial
		}
    }

    // Construye la ruta completa
    _full_path = root + relative_path;  // ej: "www/estilos.css" o "www/index.html"
    // Verificar archivo
    if (_file.is_open())
        _file.close();
    _file.clear();
	
	if (isDirectory(root.c_str())) {

		bool found_index = false;
		for (size_t i = 0; i < server.getLocationIndexCount(location_index); i++) {

			std::string index_path = root + server.getLocationIndexFile(location_index, i);
			if (fileExistsAndReadable(index_path.c_str())) {
				found_index = true;
				break ;
			}
		}
		if (!found_index && server.getAutoIndex(location_index) == true)
			return (SendAutoResponse(root), true);	
		else if (!found_index && server.getAutoIndex(location_index) == false)
			return (send403Response(), false);
	}
    else if (!fileExistsAndReadable(_full_path.c_str()) || !checkRequest()) {
        send404Response();
        return (false);
    }
	_file.open(_full_path.c_str());
	if (!_file || !checkRequest()) 
		return (send404Response(), false);
    return (true);
}

bool			Connection::fileExistsAndReadable(const char* path) {
	
    struct stat st;
    if (stat(path, &st) != 0) {
        // No existe el archivo
        return (false);
    }
    if (!S_ISREG(st.st_mode)) {
        // No es un archivo regular
        return (false);
    }
    FILE* f = fopen(path, "r");
    if (!f) {
        // No se puede abrir para lectura (sin permiso)
        return (false);
    }
    fclose(f);
    return (true);
}

bool			Connection::isDirectory(const char* path) {

	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

bool			Connection::isValidHttpVersion(const std::string& version) {
	
	return (version == "HTTP/1.0" || version == "HTTP/1.1");
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

void		Connection::SendAutoResponse(const std::string &direction_path) {

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
