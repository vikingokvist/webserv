/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpReceive.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/30 15:31:58 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/HttpReceive.hpp"

HttpReceive::HttpReceive(ServerWrapper& _server) : _server(_server) {
	
	this->_fd = this->_server.getFD();
	this->_is_cgi_script = false;
}

HttpReceive::~HttpReceive() {}


bool	HttpReceive::receiveRequest() {

    char buffer[1];
    int bytes_received;

    while (true) {
        bytes_received = recv(getFd(), buffer, sizeof(buffer), 0);

        if (bytes_received > 0) {
            _request_complete.append(buffer, bytes_received);
        } else if (bytes_received == 0) {
            // Cliente cerró la conexión
            return false;
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No hay más datos por ahora (Edge-triggered)
                break;
            } else {
                // Error real
                perror("recv");
                return false;
            }
        }
    }
	int _content_length = 0;
    // Solo buscar Content-Length si aún no lo hiciste
    if (_content_length == -1) {
        size_t cl_pos = _request_complete.find("Content-Length:");
        if (cl_pos != std::string::npos) {
            size_t cl_end = _request_complete.find("\r\n", cl_pos);
            std::string cl_str = _request_complete.substr(cl_pos + 15, cl_end - (cl_pos + 15));
            _content_length = atoi(cl_str.c_str());
            std::cout << "Content-Length: " << _content_length << std::endl;
        }
    }

    // Verificar si recibimos todo el body
    size_t header_end = _request_complete.find("\r\n\r\n");
    if (header_end == std::string::npos) return false; // headers incompletos

    size_t body_received = _request_complete.size() - (header_end + 4);
    if ((int)body_received < _content_length) {
        // No hemos recibido todo el body aún
        return false;
    }
	std::cout << "Header received: " << _request_complete.size() - body_received << " bytes\n";
	std::cout << "body_received: " << body_received << " bytes\n";
    std::cout << "Total received: " << _request_complete.size() << " bytes\n";
    return true;
}


bool			HttpReceive::saveRequest() {
	
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

bool			HttpReceive::prepareRequest() {
	
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



bool			HttpReceive::checkRequest(ServerWrapper&	server, std::string root, ssize_t best_match) {

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

std::vector<Part>	HttpReceive::parseMultipart(const std::string& body, const std::string& boundary) {
	
	std::vector<Part> parts;
	std::string delimiter = "--" + boundary;
	std::string endDelimiter = delimiter + "--";

	size_t start = 0;
	while (true) {
		size_t pos = body.find(delimiter, start);
		if (pos == std::string::npos) break;
		pos += delimiter.size();

		// Saltar CRLF
		if (body.substr(pos, 2) == "\r\n") pos += 2;

		// Si encontramos el delimitador final
		if (body.compare(pos, endDelimiter.size(), endDelimiter) == 0)
			break;

		// Buscar fin de headers
		size_t headerEnd = body.find("\r\n\r\n", pos);
		if (headerEnd == std::string::npos)
			break;

			
		std::string headers = body.substr(pos, headerEnd - pos);

		// Extraer Content-Type
		std::string content_type;
		size_t ct_pos = headers.find("Content-Type:");
		if (ct_pos != std::string::npos) {
			ct_pos += 14; // salto "Content-Type:"
			size_t ct_end = headers.find("\r\n", ct_pos);
			if (ct_end == std::string::npos) ct_end = headers.size();
				content_type = headers.substr(ct_pos, ct_end - ct_pos);

		}
		pos = headerEnd + 4; // saltar \r\n\r\n

		// Buscar siguiente boundary
		size_t next = body.find(delimiter, pos);
		if (next == std::string::npos) break;

		std::string content = body.substr(pos, next - pos);
		// Eliminar CRLF final del contenido si existe
		if (!content.empty() && content.substr(content.size()-2) == "\r\n") {
			content.erase(content.size()-2);
		}
		
		std::string filename;
		size_t find_filename = headers.find("filename=");
		if (find_filename != std::string::npos) {
			size_t startQuote = headers.find('"', find_filename);
			if (startQuote != std::string::npos) {
				++startQuote;
				size_t endQuote = headers.find('"', startQuote);
				if (endQuote != std::string::npos) {
					filename = headers.substr(startQuote, endQuote - startQuote);
				}
			}
		}
		Part p;
		p.headers = headers;
		p.content = content;
		p.filename = filename;
		p.content_type = content_type;
		parts.push_back(p);
		start = next;
	}
	return (parts);
}


bool			HttpReceive::fileExistsAndReadable(const char* path, int mode) {

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


bool	HttpReceive::isMethodAllowed(ServerWrapper& server, ssize_t best_match, std::string& method) {
	
	if (server.getMethods(best_match).empty())
		return (true);
	for (size_t j = 0; j < server.getMethodsSize(best_match); j++) {
		
		if (method == server.getMethod(best_match, j))
			return (true);
	}
	return (false);
}

ssize_t			HttpReceive::findBestMatch(ServerWrapper& server, std::string req_path) {
	
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


void HttpReceive::printParserHeader(void) {
	
	std::cout << "\033[32m -----------[REQUEST]-----------\033[0m" << std::endl << std::endl;
	std::map<std::string, std::string>::const_iterator it;
	for (it = this->_headers.begin(); it != this->_headers.end(); ++it) {
		std::cout << "\033[32m[" << it->first << "] = " << it->second << "\033[0m" << std::endl;
	}
	std::cout << "\033[32m--------------------------------\033[0m" << std::endl;
}


bool		HttpReceive::sendError(size_t error_code) {

	static Handler handlers[506] = {0};

	if (handlers[0] == 0) {
		handlers[201] = &HttpReceive::send201Response;
		handlers[204] = &HttpReceive::send204Response;
		handlers[301] = &HttpReceive::send301Response;
		handlers[302] = &HttpReceive::send302Response;
		handlers[400] = &HttpReceive::send400Response;
		handlers[401] = &HttpReceive::send401Response;
		handlers[403] = &HttpReceive::send403Response;
		handlers[404] = &HttpReceive::send404Response;
		handlers[405] = &HttpReceive::send405Response;
		handlers[411] = &HttpReceive::send411Response;
		handlers[413] = &HttpReceive::send413Response;
		handlers[414] = &HttpReceive::send414Response;
		handlers[500] = &HttpReceive::send500Response;
		handlers[501] = &HttpReceive::send501Response;
		handlers[502] = &HttpReceive::send502Response;
		handlers[503] = &HttpReceive::send503Response;
		handlers[504] = &HttpReceive::send504Response;
		handlers[505] = &HttpReceive::send505Response;
	}
	if (error_code < 506 && handlers[error_code])
		(this->*handlers[error_code])();
	return (false);
}

bool 			HttpReceive::isCgiScript() {return (this->_is_cgi_script);}

ssize_t			HttpReceive::getBestMatch() {return (_best_match);}

int				HttpReceive::getFd() {return (this->_fd);}

char*			HttpReceive::getRequest() {return (this->_request);}

std::string		HttpReceive::getHeader(std::string index) {return (this->_headers[index]);}

std::ifstream&	HttpReceive::getFile() {return (this->_file);}

std::string		HttpReceive::getFullPath() {return (this->_full_path);}

ServerWrapper&	HttpReceive::getServer() {return (this->_server);}

void			HttpReceive::setBestMatch(ssize_t _best_match) {this->_best_match = _best_match;}

void			HttpReceive::setFd(int fd) {this->_fd = fd;}

void			HttpReceive::setHeader(std::string index,std::string value) {this->_headers[index] = value;}

void			HttpReceive::setFullPath(const std::string& full_path) {this->_full_path = full_path;}

size_t			HttpReceive::getPostBodySize() {return (this->_post_body.size());}

std::string		HttpReceive::getPostBody() {return (this->_post_body);}

void			HttpReceive::sendAutoResponse(const std::string &direction_path) {HttpSend::sendAutoResponse(getFd(), *this, direction_path); }
		
void			HttpReceive::sendDeleteResponse() {HttpSend::sendDeleteResponse(getFd(), *this); }

void			HttpReceive::sendGetResponse() {HttpSend::sendGetResponse(getFd(), *this); }

void			HttpReceive::sendPostResponse() {HttpSend::sendPostResponse(getFd(), *this, _previous_full_path); }

void			HttpReceive::sendCgiResponse() {HttpSend::sendCgiResponse(getFd(), *this); }

void			HttpReceive::send200Response() { HttpSend::send200(getFd(), *this); }

void			HttpReceive::send201Response() { HttpSend::send201(getFd(), *this); }

void			HttpReceive::send204Response() { HttpSend::send204(getFd(), *this); }

void			HttpReceive::send301Response() { HttpSend::send301(getFd(), *this); }

void			HttpReceive::send302Response() { HttpSend::send302(getFd(), *this); }

void			HttpReceive::send400Response() { HttpSend::send400(getFd(), *this); }

void			HttpReceive::send401Response() { HttpSend::send401(getFd(), *this); }

void			HttpReceive::send403Response() { HttpSend::send403(getFd(), *this); }

void			HttpReceive::send404Response() { HttpSend::send404(getFd(), *this); }

void			HttpReceive::send405Response() { HttpSend::send405(getFd(), *this); }

void			HttpReceive::send411Response() { HttpSend::send411(getFd(), *this); }

void			HttpReceive::send413Response() { HttpSend::send413(getFd(), *this); }

void			HttpReceive::send414Response() { HttpSend::send414(getFd(), *this); }

void			HttpReceive::send415Response() { HttpSend::send415(getFd(), *this); }

void			HttpReceive::send500Response() { HttpSend::send500(getFd(), *this); }

void			HttpReceive::send501Response() { HttpSend::send501(getFd(), *this); }

void			HttpReceive::send502Response() { HttpSend::send502(getFd(), *this); }

void			HttpReceive::send503Response() { HttpSend::send503(getFd(), *this); }

void			HttpReceive::send504Response() { HttpSend::send504(getFd(), *this); }

void			HttpReceive::send505Response() { HttpSend::send505(getFd(), *this); }


