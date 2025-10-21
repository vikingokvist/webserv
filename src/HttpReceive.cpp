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
#include "../includes/Logger.hpp"

HttpReceive::HttpReceive(ServerWrapper& _server, std::map<std::string, Session>& session) : _server(_server) {

	this->_session = session;
	this->user_accepts_cookies = false;
	this->_is_cgi_script = false;
	this->_is_redirect = false;
	this->body_state = B_INCOMPLETE;
	this->header_state = H_INCOMPLETE;
	this->body_type = UNSET;
	this->_total_bytes = 0;
}

HttpReceive::~HttpReceive() {}


RecvStatus	HttpReceive::receiveRequest() {

    char	buffer[100];
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
			if (header_state == H_COMPLETE && _headers.find("Content-Length") != _headers.end() && checkContentLength(_headers["Content-Length"].c_str(), _server.getClientMaxBodySize()) > 0)
				return (RECV_PAYLOAD_TOO_LARGE_ERROR);
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
	if (header_state == H_COMPLETE && body_state == B_INCOMPLETE && body_type == CHUNKED) {
		return (RECV_INCOMPLETE);
	}
	if (_headers.find("Content-Length") != _headers.end() && _body_complete.empty()) {
		_body_complete = _request_parse;
	}
	if (_headers["Content-Type"] == "multipart/form-data") {
		parseMultipart(_body_complete, _headers["Boundary"]);
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
	logger(this->_headers, CLIENT_REQUEST);
	return (true);
}

bool			HttpReceive::checkRequest() {

	if (isMissingRequiredHeaders(this->_headers))
		return (sendError(400));
	if (isUnsupportedMethod(this->_headers))
		return (sendError(501));
	if (isPathTraversal(this->_headers))
		return (sendError(403));
	if (isUriTooLong(this->_headers))
		return (sendError(414));
	if (isInvalidHttpVersion(this->_headers))
		return (sendError(505));
	if (isInvalidContentLength(this->_headers))
		return (sendError(400));
	if (isMissingContentLengthForPost(this->_headers))
		return (sendError(411));
	if (isContentLengthTooLarge(this->_headers, this->_server.getClientMaxBodySize()))
		return (sendError(413));
	if (isMissingContentTypeForPost(this->_headers))
		return (sendError(415));
	if (clientHasCookiesEnabled(this->_headers))
			setClientCookie();
	if (this->_server.getRedirect(getBestMatch()))
		return (this->_is_redirect = true);


	if (this->_headers["Method"] == "GET")
		return (methodGET(this->_server, getBestMatch()));
	else if (this->_headers["Method"] == "POST")
		return (methodPOST(this->_server, getBestMatch()));
	else if (this->_headers["Method"] == "DELETE")
		return (methodDELETE(this->_server, getBestMatch()));
	else if (this->_headers["Method"] == "HEAD")
		return (methodHEAD(this->_server, getBestMatch()));

	return (sendError(501));
}

bool	HttpReceive::methodHEAD(ServerWrapper &server, size_t best_match) {

	std::string root = this->_headers["Root"];

	if (!isMethodAllowed(server, best_match, this->_headers["Method"]))
		return (sendError(405));

	if (isDirectory(this->_full_path.c_str())) {
		if (server.getAutoIndex(best_match)) {
			this->_is_autoindex = true;
			this->_autoindex_to =  this->_full_path;
			return (true);
		}
		else
			return (sendError(404));
	}

	if (this->_full_path.empty() && server.getAutoIndex(best_match)) {
		this->_is_autoindex = true;
		this->_autoindex_to = root;
		return (true);
	}
	else if (this->_full_path.empty() && !server.getAutoIndex(best_match))
		return (sendError(404));

	if (!fileExistsAndReadable(this->_full_path.c_str(), 1))
		return (false);

	return (true);
}

bool	HttpReceive::methodGET(ServerWrapper &server, size_t best_match) {

	std::string root = this->_headers["Root"];

	if (!isMethodAllowed(server, best_match, this->_headers["Method"]))
		return (sendError(405));

	if (isDirectory(this->_full_path.c_str())) {
		if (server.getAutoIndex(best_match)) {
			this->_is_autoindex = true;
			this->_autoindex_to = this->_full_path;
			return (true);
		}
		else
			return (sendError(404));
	}

	if (this->_full_path.empty() && server.getAutoIndex(best_match)) {
		this->_is_autoindex = true;
		this->_autoindex_to = root;
		return (true);
	}

	else if (this->_full_path.empty() && !server.getAutoIndex(best_match))
		return (sendError(404));

	if (!fileExistsAndReadable(this->_full_path.c_str(), 1))
		return (false);

	this->_file.open(this->_full_path.c_str());
	if (!this->_file)
		return (sendError(404));

	return (true);
}

bool	HttpReceive::methodPOST(ServerWrapper &server, size_t best_match) {

	if (!isMethodAllowed(server, best_match, this->_headers["Method"]))
		return (sendError(405));
	if (this->_headers.find("Boundary") != this->_headers.end()) {
	
		if (this->_headers.find("Upload Store") == this->_headers.end())
			return (sendError(500));

		for (size_t i = 0; i < this->parts.size(); ++i) {
			std::string full_path = this->_headers["Upload Store"] + this->parts[i].filename;
			std::ofstream file_post(full_path.c_str());
			if (!file_post)
				return (std::cerr << ERROR_CREATE_FILE << full_path << std::endl, false);
			file_post.write(parts[i].content.data(), parts[i].content.size());
			if (file_post.fail() || file_post.bad()) {
				file_post.close();
				return (false);
			}
			file_post.close();
		}
	}
	else {

		size_t extension_pos = this->_headers["Path"].rfind(".");
		if (extension_pos != std::string::npos) {
			std::string file_extension = this->_headers["Path"].substr(extension_pos);
			for (size_t i = 0; i < server.getCgiExtensionCount(best_match); ++i) {
				if (file_extension == server.getCgiExtensions(best_match, i)) {
					this->_is_cgi_script = true;
				}
			}
		}
	}
	return (true);
}


bool	HttpReceive::methodDELETE(ServerWrapper &server, size_t best_match) {

	std::string path_to_delete;
	if (this->_headers.find("Upload Store") != this->_headers.end()) {
		std::string filename = this->_headers["Path"].substr(this->_headers["Path"].find_last_of("/") + 1);
		path_to_delete = this->_headers["Upload Store"] + filename;
	}
	else
		return (sendError(404));

	if (!isMethodAllowed(server, best_match, this->_headers["Method"]))
		return (sendError(405));
	if (!fileExistsAndReadable(path_to_delete.c_str(), 0))
		return (sendError(404));
	if (std::remove(path_to_delete.c_str()) != 0)
		return (sendError(403));
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

void	HttpReceive::resetForNextRequest() {
	
    memset(_request, 0, BUFFER_SIZE);  
    _headers.clear();
    parts.clear();
    if (_file.is_open()) _file.close();
    _full_path.clear();
    _request_parse.clear();
    _body_complete.clear();
    _is_cgi_script = false;
    _is_redirect = false;
    header_state = H_INCOMPLETE;
	body_state = B_INCOMPLETE;
	body_type = UNSET;
    _best_match = -1;
	_is_autoindex = false;
	_autoindex_to.clear();
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
	if (access(path, W_OK) != 0) {

		if (mode == 1) {
			sendError(403);
		}
		return (false);
	}
	return (true);
}

bool	HttpReceive::isMethodAllowed(ServerWrapper& server, ssize_t best_match, std::string& method) {
	
	if (server.getMethods(best_match).empty())
		return (false);
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

bool		HttpReceive::sendError(size_t error_code) {
	PollData &pd = _conn->getFdMap()[_fd];
	pd.has_error = true;
	pd.error_code = error_code;
	_conn->modifyEpollEvent(_fd, EPOLLOUT); 
	return(false);
}

bool	HttpReceive::sendOutErr(size_t error_code) {
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
		handlers[505] = &HttpReceive::send505Response;
	}
	if (error_code < 506 && handlers[error_code])
		 return (this->*handlers[error_code])();
	return (false);
}

std::map<std::string, Session>&		HttpReceive::getSession() {return (this->_session);}

void			HttpReceive::logger(std::map<std::string, std::string> _headers, int flag) {Logger::logger2(_headers, flag, this->getFd());}

void			HttpReceive::setClientCookie() {this->user_accepts_cookies = true;}

bool			HttpReceive::hasClientCookie() {return (this->user_accepts_cookies);}

bool			HttpReceive::isRedirection() {return (this->_is_redirect);}

bool 			HttpReceive::isCgiScript() {return (this->_is_cgi_script);}

ssize_t			HttpReceive::getBestMatch() {return (_best_match);}

int				HttpReceive::getFd() {return (this->_fd);}

char*			HttpReceive::getRequest() {return (this->_request);}

std::string		HttpReceive::getHeader(std::string index) {if (this->_headers.find(index) != this->_headers.end()) return (this->_headers[index]); else return ("");}

std::ifstream&	HttpReceive::getFile() {return (this->_file);}

std::string		HttpReceive::getFullPath() {return (this->_full_path);}

ServerWrapper&	HttpReceive::getServer() {return (this->_server);}

void			HttpReceive::setBestMatch(ssize_t _best_match) {this->_best_match = _best_match;}

void			HttpReceive::setFd(int fd) {this->_fd = fd;}

void			HttpReceive::setHeader(std::string index,std::string value) {this->_headers[index] = value;}

void			HttpReceive::setFullPath(const std::string& full_path) {this->_full_path = full_path;}

size_t			HttpReceive::getPostBodySize() {return (this->_body_complete.size());}

std::string		HttpReceive::getPostBody() {return (this->_body_complete);}

bool			HttpReceive::sendAutoResponse(const std::string &direction_path) {return HttpSend::sendAutoResponse(getFd(), *this, direction_path); }
		
bool			HttpReceive::sendDeleteResponse() {return HttpSend::sendDeleteResponse(getFd(), *this); }

bool			HttpReceive::sendGetResponse() {return HttpSend::sendGetResponse(getFd(), *this); }

bool			HttpReceive::sendPostResponse() {return HttpSend::sendPostResponse(getFd(), *this); }

bool			HttpReceive::sendHeadResponse() {return HttpSend::sendHeadResponse(getFd(), *this); }

bool			HttpReceive::sendCgiResponse() { return HttpSend::sendCgiResponse(getFd(), *this);}

bool			HttpReceive::sendRedirectResponse() {return HttpSend::sendRedirectResponse(getFd(), *this, getBestMatch()); }

bool			HttpReceive::send200Response() { return HttpSend::send200(getFd(), *this); }

bool			HttpReceive::send201Response() {  return HttpSend::send201(getFd(), *this); }

bool			HttpReceive::send204Response() { return HttpSend::send204(getFd(), *this); }

bool			HttpReceive::send301Response() { return HttpSend::send301(getFd(), *this); }

bool			HttpReceive::send302Response() { return HttpSend::send302(getFd(), *this); }

bool			HttpReceive::send400Response() { return HttpSend::send400(getFd(), *this); }

bool			HttpReceive::send401Response() { return HttpSend::send401(getFd(), *this); }

bool			HttpReceive::send403Response() { return HttpSend::send403(getFd(), *this); }

bool			HttpReceive::send404Response() { return HttpSend::send404(getFd(), *this); }

bool			HttpReceive::send405Response() { return HttpSend::send405(getFd(), *this); }

bool			HttpReceive::send411Response() { return HttpSend::send411(getFd(), *this); }

bool			HttpReceive::send413Response() { return HttpSend::send413(getFd(), *this); }

bool			HttpReceive::send414Response() { return HttpSend::send414(getFd(), *this); }

bool			HttpReceive::send415Response() { return HttpSend::send415(getFd(), *this); }

bool			HttpReceive::send500Response() { return HttpSend::send500(getFd(), *this); }

bool			HttpReceive::send501Response() { return HttpSend::send501(getFd(), *this); }

bool			HttpReceive::send505Response() { return HttpSend::send505(getFd(), *this); }


