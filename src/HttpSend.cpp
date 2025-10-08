#include "../includes/HttpSend.hpp"
#include "../includes/HttpReceive.hpp" 


void		HttpSend::sendGetResponse(int fd, HttpReceive& _request) {

	std::ostringstream body_stream;
	body_stream << _request.getFile().rdbuf();
	std::string body = body_stream.str();

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
	oss << "Content-Type: " << getContentType(_request.getFullPath()) << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";

	std::string connection_header = _request.getHeader("Connection");
    if (connection_header == "keep-alive")
		oss << "Connection: keep-alive\r\n\r\n";
    else
		oss << "Connection: close\r\n\r\n";
	
	oss << body;

	std::string response = oss.str();
	send(fd, response.c_str(), response.size(), 0);
}

void		HttpSend::sendPostResponse(int fd, HttpReceive& _request) {

	std::ostringstream body_stream;
	body_stream << _request.getFile().rdbuf();
	std::string body = body_stream.str();
	
	std::ostringstream oss;
	oss << "HTTP/1.1 303 See Other\r\n";
	oss << "Location: "<< _request.getHeader("Path") << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	std::string connection_header = _request.getHeader("Connection");
    if (connection_header == "keep-alive")
		oss << "Connection: keep-alive\r\n\r\n";
    else
		oss << "Connection: close\r\n\r\n";
	oss << body;

	std::string response = oss.str();
	send(fd, response.c_str(), response.size(), 0);;
}

void		HttpSend::sendDeleteResponse(int fd, HttpReceive& _request) {

	std::ostringstream body_stream;
	body_stream << _request.getFile().rdbuf();
	std::string body = body_stream.str();

    std::ostringstream oss;
    oss << "HTTP/1.1 204 No Content\r\n";
    std::string connection_header = _request.getHeader("Connection");
    if (connection_header == "keep-alive")
		oss << "Connection: keep-alive\r\n\r\n";
    else
		oss << "Connection: close\r\n\r\n";
	oss << body;

	std::string response = oss.str();
	send(fd, response.c_str(), response.size(), 0);
}

void		HttpSend::sendRedirectResponse(int fd, HttpReceive& _request, size_t best_match) {

	std::string			response;
	std::ostringstream	oss;
	int					error_code = _request.getServer().getRedirectCode(best_match);
	std::string			url_or_loc = _request.getServer().getRedirectUrl(best_match);
   	std::string			connection_header = _request.getHeader("Connection");
	

	if (url_or_loc.empty() || (!url_or_loc.empty() && error_code != 301 && error_code != 302)) {

		sendErr(fd, _request, error_code);
	}
	else if (!url_or_loc.empty() && (error_code == 301 || error_code  == 302)) {

    	oss << "HTTP/1.1 " << error_code << getStatusMsg(error_code) << "\r\n";
    	oss << "Location: " << url_or_loc << "\r\n";
    	oss << "Content-Length: " << 0 << "\r\n";
		if (connection_header == "keep-alive")
			oss << "Connection: keep-alive\r\n\r\n";
		else
			oss << "Connection: close\r\n\r\n";
		oss << "\r\n";
	}
    response = oss.str();
	::send(fd, response.c_str(), response.size(), 0);
}

void		HttpSend::sendAutoResponse(int fd, HttpReceive& _request, const std::string &direction_path) {

	DIR * dir = opendir(direction_path.c_str());
	if (!dir) {
		send403(fd, _request);
		return ;
	}
	std::ostringstream body;
	body << "<html><head><title>Index of " << _request.getHeader("Path") << "</title></head><body>";
	body << "<h1>Index of " << _request.getHeader("Path") << "</h1><ul>";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;

		if (name == "." || name == "..")
			continue ;

		std::string href = _request.getHeader("Path");
		if (href.empty() || href[href.size()-1] != '/')
			href += "/";
		href += name;

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
    std::string connection_header = _request.getHeader("Connection");
    if (connection_header == "keep-alive")
		response << "Connection: keep-alive\r\n\r\n";
    else
		response << "Connection: close\r\n\r\n";
	response << bodyStr;

	send(fd, response.str().c_str(), response.str().size(), 0);
}

void			HttpSend::sendCgiResponse(int fd, HttpReceive& _request) {
	
	int		pipe_parent[2];
	int		pipe_child[2];
	pid_t	pid;
	
	if (pipe(pipe_parent) == -1) {
		std::cerr << "Parent pipe() failed: " << strerror(errno) << std::endl;
    	send500(fd, _request); return ;
	}
	if (pipe(pipe_child) == -1) {
		std::cerr << "Child pipe() failed: " << strerror(errno) << std::endl;
    	send500(fd, _request); return ;
	}
	pid = fork();
	if (pid < 0) {
		std::cerr << "fork() failed: " << strerror(errno) << std::endl;
    	send500(fd, _request); return ;
	}
	else if (pid == 0) {
		
		if (dup2(pipe_parent[0], STDIN_FILENO) == -1) {
			std::cerr << "Child dup2() failed: " << strerror(errno) << std::endl;
			close(pipe_parent[0]); close(pipe_parent[1]); close(pipe_child[0]); close(pipe_child[1]);
			send500(fd, _request); exit(1);
		}
		if (dup2(pipe_child[1], STDOUT_FILENO) == -1) {
			std::cerr << "Child dup2() failed: " << strerror(errno) << std::endl;
			close(pipe_parent[0]); close(pipe_parent[1]); close(pipe_child[0]); close(pipe_child[1]);
			send500(fd, _request); exit(1);
		}
		close(pipe_parent[1]);
		close(pipe_child[0]);
		
		std::string script_name;
		std::string script_file_path;
		size_t script_name_pos = _request.getHeader("Path").rfind('/');
		if (script_name_pos != std::string::npos) {
			script_name = _request.getHeader("Path").substr(script_name_pos + 1);
			script_file_path = _request.getHeader("Root") + script_name;
		}
		
		if (access(script_file_path.c_str(), R_OK) != 0) {
			//CHECK IF NEED TO CLOSE FDS
			std::cerr << "File not executable: " << strerror(errno) << std::endl;
			send403(fd, _request); exit(1);
		}

		std::vector<std::string> env_strings;
		std::vector<char*> envp;

		env_strings.push_back("REQUEST_METHOD=" + _request.getHeader("Method"));
		env_strings.push_back("CONTENT_LENGTH=" + _request.getHeader("Content-Length"));
		env_strings.push_back("CONTENT_TYPE=" + _request.getHeader("Content-Type"));
		env_strings.push_back("SCRIPT_NAME=" + script_name);
		env_strings.push_back("SCRIPT_FILEPATH=" + script_file_path);
		env_strings.push_back("SERVER_PROTOCOL=" + _request.getHeader("Version"));
		env_strings.push_back("QUERY_STRING=" + _request.getPostBody());
		
		for (size_t i = 0; i < env_strings.size(); i++)
			envp.push_back(const_cast<char*>(env_strings[i].c_str()));
		envp.push_back(NULL);

		char *argv[] = { (char*)script_file_path.c_str(), NULL };
		
		if (execve(script_file_path.c_str(), argv, envp.data()) == -1) {
			std::cerr << "Child execve() failed: " << strerror(errno) << std::endl;
			send500(fd, _request);exit(1);
		}
	}
	else {
		
		close(pipe_parent[0]);
		close(pipe_child[1]);

		write(pipe_parent[1], _request.getPostBody().c_str(), _request.getPostBodySize());
		close(pipe_parent[1]);

		char buffer[4096];
		std::string cgi_output;
		ssize_t n;
		
		while ((n = read(pipe_child[0], buffer, sizeof(buffer))) > 0)
			cgi_output.append(buffer, n);
		close(pipe_child[0]);
	

		std::ostringstream oss;
		oss << "HTTP/1.1 200 OK\r\n";
		std::string connection_header = _request.getHeader("Connection");
		if (connection_header == "keep-alive") {
			oss << "Connection: keep-alive\r\n";
		} else {
			oss << "Connection: close\r\n";
		}
		oss << cgi_output;

		std::string http_response = oss.str();
		send(_request.getFd(), http_response.c_str(), http_response.size(), 0);

		int status;
		waitpid(pid, &status, 0);
	}
}

void        HttpSend::sendErr(int fd, HttpReceive& _request, int error_code) {

    std::ostringstream buf;
    std::string body;
    std::string response;
    std::string root = _request.getServer().getErrorRoot(error_code).c_str();
    std::string file = _request.getServer().getErrorFile(error_code).c_str();
    std::string full_path = root + file;
    int         error = 1;

    if (isDirectory(root.c_str())) { 

    	std::ifstream file(full_path.c_str(), std::ios::in | std::ios::binary);
    	if (file) {
    		buf << file.rdbuf();
    		body = buf.str();
            error = 0;
    	}
    }
    if (error ==  1) {

            buf << "<!DOCTYPE html><html><head><title>" << getStatusMsg(error_code) << "</title></head>"
            << "<body><h1>" << getStatusMsg(error_code) << "</h1></body></html>";	
            body = buf.str();
    }

    std::ostringstream oss;
    oss << "HTTP/1.1 " << error_code << getStatusMsg(error_code) << "\r\n"
    	<< "Content-Type: text/html\r\n"
    	<< "Content-Length: " << body.size() << "\r\n"
    	<< "Connection: close\r\n\r\n"
    	<< body;

    response = oss.str();
    ::send(fd, response.c_str(), response.size(), 0);
    ::close(fd);
}

std::string		HttpSend::getStatusMsg(int error_code) {

	std::map<int, std::string> status_msg;
	status_msg[200] = " OK";
	status_msg[201] = " Created";
	status_msg[204] = " No Content";
	status_msg[301] = " Moved Permanently";
	status_msg[302] = " Found";
	status_msg[400] = " Bad Request";
	status_msg[401] = " Unauthorized";
	status_msg[403] = " Forbidden";
	status_msg[404] = " Not Found";
	status_msg[405] = " Method Not Allowed";
	status_msg[411] = " Length Required";
	status_msg[413] = " Payload Too Large";
	status_msg[414] = " URI Too Long";
	status_msg[415] = " Unsupported Media Type";
	status_msg[500] = " Internal Server Error";
	status_msg[501] = " Not Implemented";
	status_msg[502] = " Bad Gateway";
	status_msg[503] = " Service Unavailable";
	status_msg[504] = " Gateway Timeout";
	status_msg[505] = " HTTP Version Not Supported";
	if (!status_msg[error_code].empty())
		return (status_msg[error_code]);
	return ("");
}


std::string		getContentType(const std::string& path) {
	
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