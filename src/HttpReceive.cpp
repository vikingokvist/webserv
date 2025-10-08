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
	this->_is_redirect = false;
	this->body_state = B_INCOMPLETE;
	this->header_state = H_INCOMPLETE;
	this->body_type = UNSET;
	this->_total_bytes = 0;
}

HttpReceive::~HttpReceive() {}


RecvStatus	HttpReceive::receiveRequest() {

    char	buffer[8192];
    int		bytes_received;
	
    while (true) {

        bytes_received = recv(getFd(), buffer, sizeof(buffer), 0);

        if (bytes_received > 0) {

			_request_parse.append(buffer, bytes_received);

			size_t header_end_pos = _request_parse.find("\r\n\r\n");
			if (header_state == H_INCOMPLETE && header_end_pos != std::string::npos) {
				
				if (!parseHeader(_request_parse.substr(0, header_end_pos)))
					return (RECV_ERROR);
				header_state = H_COMPLETE;
				_request_parse = _request_parse.substr(header_end_pos + 4);
				if (_headers.find("Transfer-Encoding") != _headers.end() && _headers["Transfer-Encoding"] == "chunked")
					body_type = CHUNKED;
			}

			if (header_state == H_COMPLETE && body_state == B_INCOMPLETE && body_type == CHUNKED) {
				
				if (!parseChunkedBody(_request_parse))
					return (RECV_PAYLOAD_TOO_LARGE_ERROR);
			}

        }
		else if (bytes_received == 0) {

            return (RECV_CLOSED);
        }
		else {

			break ;
        }
    }
	if (header_state == H_COMPLETE &&  body_state == B_INCOMPLETE && body_type == CHUNKED)
		return (RECV_INCOMPLETE);
	if (body_type != CHUNKED && !_request_parse.empty()) {
		
		_body_complete = _request_parse;
	}
	if (_headers.find("Content-Type") != _headers.end() && _headers["Content-Type"] == "multipart/form-data") {
		body_type = MULTIPART;
		parseMultipart(_body_complete, _headers["Boundary"]);
	}
	if (_headers.find("Content-Type") != _headers.end() && _headers["Content-Type"] == "plain/txt") {
		body_type = PLAIN;
	}
    return (RECV_COMPLETE);
}

bool			HttpReceive::parseHeader(std::string header_complete) {
	
	std::string			request(header_complete);
	std::istringstream	iss(request);
	std::string			line;



	if (!std::getline(iss, line) || line.empty())
		return (sendError(400));

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
	return (true);
}

bool			HttpReceive::prepareRequest() {
	
	std::string				 req_path   = this->_headers["Path"];
	ServerWrapper&			 server	    = this->_server;
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
	if (!_location.upload_store.empty())
		this->_headers["Upload Store"] = _location.upload_store;

	if (index_files.size() != 0 && (req_path == server.getLocationPath(getBestMatch()) || req_path == server.getLocationPath(getBestMatch()) + "/")) {
		for (size_t i = 0; i < index_files.size(); i++) {
			std::string found_path = root + index_files[i];
			if (fileExistsAndReadable(found_path.c_str(), 0)) {
				this->_full_path = found_path;
				break ;
			}
		}
	}
	else {
		relative_path = req_path.substr(server.getLocationPath(getBestMatch()).size());
		if (!relative_path.empty() && relative_path[0] == '/')
			relative_path.erase(0, 1);
		this->_full_path = root + relative_path;
	}
	printParserHeader();
	return (true);
}

bool			HttpReceive::checkRequest() {

	ServerWrapper	&server = this->_server;
	size_t			best_match = getBestMatch();
	std::string		root = this->_headers["Root"];

	if (this->_headers.find("Method") == this->_headers.end() || this->_headers.find("Path") == this->_headers.end() || this->_headers.find("Host") == this->_headers.end())
		return (sendError(400));
	if (this->_headers.find("Method") != this->_headers.end() && (this->_headers["Method"] != "GET" && this->_headers["Method"] != "POST" && this->_headers["Method"] != "DELETE"))
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
	if (this->_headers["Method"] == "POST" && this->_headers.find("Content-Type") == this->_headers.end())
		return (sendError(415));
	if (server.getRedirect(best_match)) {

		this->_is_redirect = true;
		return (true);
	}
	else if (this->_headers["Method"] == "GET") {

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
	else if (this->_headers.find("Boundary") != this->_headers.end() && this->_headers["Method"] == "POST") {
		
		if (isMethodAllowed(server, best_match, this->_headers["Method"]) == false)
			return (sendError(405));
		if (this->_headers.find("Upload Store") == this->_headers.end())
			return (sendError(500));
		for (size_t i = 0; i < this->parts.size(); ++i) {
			std::string full_path = this->_headers["Upload Store"] + this->parts[i].filename;
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
		size_t	extension_pos = this->_headers["Path"].find(".py");
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
		if (this->_full_path.find("%2e%2e") != std::string::npos) {
				return (sendError(403));
		}
		if (!fileExistsAndReadable(this->_full_path.c_str(), 0)) {
			return (sendError(404));
		}
		if (remove(this->_full_path.c_str()) != 0) {
        	return (sendError(403));
		}
	}
	return (true);
}

void	HttpReceive::parseMultipart(const std::string& body, const std::string& boundary) {
	
	std::vector<Part> parts;
	std::string delimiter = "--" + boundary;
	std::string endDelimiter = delimiter + "--";

	size_t start = 0;
	while (true) {
		size_t pos = body.find(delimiter, start);
		if (pos == std::string::npos) break;
		pos += delimiter.size();

		if (body.substr(pos, 2) == "\r\n") pos += 2;
		if (body.compare(pos, endDelimiter.size(), endDelimiter) == 0)
			break;

		size_t headerEnd = body.find("\r\n\r\n", pos);
		if (headerEnd == std::string::npos)
			break;

			
		std::string headers = body.substr(pos, headerEnd - pos);

		std::string content_type;
		size_t ct_pos = headers.find("Content-Type:");
		if (ct_pos != std::string::npos) {
			ct_pos += 14;
			size_t ct_end = headers.find("\r\n", ct_pos);
			if (ct_end == std::string::npos) ct_end = headers.size();
				content_type = headers.substr(ct_pos, ct_end - ct_pos);

		}
		pos = headerEnd + 4;

		size_t next = body.find(delimiter, pos);
		if (next == std::string::npos) break;

		std::string content = body.substr(pos, next - pos);
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
	this->parts = parts;
}

bool	HttpReceive::parseChunkedBody(std::string& _body_recv) {

    std::string new_body;

	while (true) {

		size_t crlf_pos = _body_recv.find("\r\n");
		if (crlf_pos == std::string::npos)
		    return (true);

		std::string hex_str = _body_recv.substr(0, crlf_pos);
		unsigned long chunk_size = strtoul(hex_str.c_str(), NULL, 16);
		if (chunk_size == 0) {
    	    if (_body_recv.size() < crlf_pos + 4)
				return (true);

            if (_body_recv.substr(crlf_pos, 4) == "0\r\n\r\n" || _body_recv.substr(crlf_pos + 2, 2) == "\r\n") {
    	        this->body_state = B_COMPLETE;
				std::ostringstream oss;
				oss << this->_total_bytes;
    	        this->_headers["Content-Length"] = oss.str();
				_body_recv.erase(0, crlf_pos + 4);
    	        return (true);
    	    }
    	    return (true);
    	}
		if (_body_recv.size() < crlf_pos + 2 + chunk_size + 2)
    	    return (true);

		size_t start = crlf_pos + 2;
		this->_body_complete.append(_body_recv, start, chunk_size);
		this->_total_bytes += chunk_size;
		_body_recv.erase(0, start + chunk_size + 2);
		if (this->_total_bytes > this->_server.getClientMaxBodySize())
    		return (false);
	}
	return (true);
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

bool			HttpReceive::isRedirection() {return (this->_is_redirect);}

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

size_t			HttpReceive::getPostBodySize() {return (this->_body_complete.size());}

std::string		HttpReceive::getPostBody() {return (this->_body_complete);}

void			HttpReceive::sendAutoResponse(const std::string &direction_path) {HttpSend::sendAutoResponse(getFd(), *this, direction_path); }
		
void			HttpReceive::sendDeleteResponse() {HttpSend::sendDeleteResponse(getFd(), *this); }

void			HttpReceive::sendGetResponse() {HttpSend::sendGetResponse(getFd(), *this); }

void			HttpReceive::sendPostResponse() {HttpSend::sendPostResponse(getFd(), *this); }

void			HttpReceive::sendCgiResponse() {HttpSend::sendCgiResponse(getFd(), *this); }

void			HttpReceive::sendRedirectResponse() {HttpSend::sendRedirectResponse(getFd(), *this, getBestMatch()); }

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


