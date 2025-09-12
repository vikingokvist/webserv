/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/12 12:33:42 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Connection.hpp"

Connection::Connection(ServerWrapper& _server) : _server(_server) {
	
	this->_fd = this->_server.getFD();
}

Connection::~Connection() {}

bool			Connection::setConnection(ServerWrapper& _server) {
	
	this->_fd = accept(_server.getSocket(), (struct sockaddr*)_server.getSockAddr(), (socklen_t*)_server.getSockAddr());
	if ( this->_fd < 0) {
		std::cerr << "accept() failed: " << strerror(errno) << std::endl;
		return (false);
	}
	if (!setRequest()) { 	 // Recibimos la Request
		return (false);
	}
	if (!saveRequest(getRequest())) {	// Guardamos la Request
		return (false);
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
    this->_full_path = root + relative_path;  // ej: "www/estilos.css" o "www/index.html"
    // Verificar archivo
    if (this->_file.is_open())
        this->_file.close();
    this->_file.clear();
	
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
	this->_file.open(_full_path.c_str());
	if (!this->_file || !checkRequest()) 
		return (send404Response(), false);
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

bool			Connection::saveRequest(char *request) {
	
	
	std::cout << "REQUEST" << std::endl;
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

	if (method == "POST") {
		std::string body_part = full_request.substr(header_end + 4);
		std::string boundary = "------geckoformboundaryc324ad9b46a325a07382e0f763b09062";

		// 1. Buscar inicio del filename
		size_t start_filename = body_part.find("filename=\"");
		start_filename += 10; // saltar 'filename="'

		// 2. Extraer nombre del archivo
		size_t end_filename = body_part.find("\"", start_filename);
		std::string filename = body_part.substr(start_filename, end_filename - start_filename);

		// 3. Buscar inicio del contenido
		size_t content_start = body_part.find("\r\n\r\n", end_filename);
		content_start += 4; // saltar los \r\n\r\n

		size_t content_end = body_part.find(boundary, content_start); // buscar boundary final
		if (content_end != std::string::npos) {
			// Quitar los \r\n justo antes del boundary
			while (content_end > content_start && 
				(body_part[content_end - 1] == '\n' || body_part[content_end - 1] == '\r')) {
				--content_end;
			}
		}

		std::string file_content = body_part.substr(content_start, content_end - content_start);
		
		// 6. Guardar archivo en uploads/
		std::string filepath = "uploads/" + filename;
		
		std::ofstream ofs(filepath.c_str(), std::ios::out | std::ios::binary);
		if (ofs.is_open()) {
			ofs << file_content;
			ofs.close(); // se crea y cierra inmediatamente
			std::cout << "Archivo creado: " << filepath << std::endl;
		} else {
			std::cerr << "Error al crear archivo: " << filepath << std::endl;
		}
	}
	else {
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

int				Connection::getFd() {return (this->_fd);}

char*			Connection::getRequest() {return (this->_request);}

std::string		Connection::getHeader(std::string index) {return (this->_headers[index]);}

std::ifstream&	Connection::getFile() {return (this->_file);}

std::string		Connection::getFullPath() {return (this->_full_path);}

ServerWrapper&	Connection::getServer() {return (this->_server);}

void			Connection::setFd(int fd) {this->_fd = fd;}

void			Connection::setHeader(std::string index,std::string value) {this->_headers[index] = value;}

void			Connection::setFullPath(const std::string& full_path) {this->_full_path = full_path;}

void			Connection::send400Response() {ErrorResponse::send400(getFd(), *this);}

void			Connection::send403Response() {ErrorResponse::send403(getFd(), *this);}

void			Connection::send404Response() {ErrorResponse::send404(getFd(), *this);}

void			Connection::send405Response() {ErrorResponse::send405(getFd(), *this);}

void			Connection::send505Response() {ErrorResponse::send505(getFd(), *this);}

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