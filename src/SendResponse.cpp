#include "../includes/SendResponse.hpp"
#include "../includes/Connection.hpp" 


void		SendResponse::sendGetResponse(int fd, Connection& _connection) {

	std::ostringstream body_stream;
	body_stream << _connection.getFile().rdbuf();
	std::string body = body_stream.str();

	std::ostringstream oss;
	oss << "HTTP/1.1 200 OK\r\n";
	oss << "Content-Type: " << getContentType(_connection.getFullPath()) << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n\r\n";
	oss << body;

	std::string response = oss.str();
	send(fd, response.c_str(), response.size(), 0);
	close(fd);
}

void		SendResponse::sendPostResponse(int fd, Connection& _connection, std::string _previous_full_path) {

	std::ostringstream body_stream;
	body_stream << _connection.getFile().rdbuf();
	std::string body = body_stream.str();
	

	std::ostringstream oss;
	oss << "HTTP/1.1 303 See Other\r\n";
	oss << "Location: "<< _previous_full_path  << "\r\n";
	oss << "Content-Length: " << body.size() << "\r\n";
	oss << "Connection: close\r\n\r\n";

	std::string response = oss.str();
	send(fd, response.c_str(), response.size(), 0);
	close(fd);
}

void		SendResponse::sendDeleteResponse(int fd, Connection& _connection) {

	std::ostringstream body_stream;
	body_stream << _connection.getFile().rdbuf();
	std::string body = body_stream.str();

    std::ostringstream oss;
    oss << "HTTP/1.1 204 No Content\r\n";
    oss << "Connection: close\r\n\r\n";
	oss << body;

	std::string response = oss.str();
	send(fd, response.c_str(), response.size(), 0);
	close(fd);
}

void		SendResponse::sendAutoResponse(int fd, Connection& _connection, const std::string &direction_path) {

	DIR * dir = opendir(direction_path.c_str());
	if (!dir) {
		send403(fd, _connection);
		return ;
	}
	std::ostringstream body;
	body << "<html><head><title>Index of " << _connection.getHeader("Path") << "</title></head><body>";
	body << "<h1>Index of " << _connection.getHeader("Path") << "</h1><ul>";

	struct dirent* entry;
	while ((entry = readdir(dir)) != NULL) {
		std::string name = entry->d_name;

		if (name == "." || name == "..")
			continue ;

		std::string href = _connection.getHeader("Path");
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
	response << "Connection: close\r\n\r\n";
	response << bodyStr;

	send(fd, response.str().c_str(), response.str().size(), 0);
}


void        SendResponse::sendErr(int fd, Connection& _connection, int error_code, const std::string& status) {

    std::ostringstream buf;
    std::string body;
    std::string response;
    std::string root = _connection.getServer().getErrorRoot(error_code).c_str();
    std::string file = _connection.getServer().getErrorFile(error_code).c_str();
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

            buf << "<!DOCTYPE html><html><head><title>" << status << "</title></head>"
            << "<body><h1>" << status << "</h1></body></html>";	
            body = buf.str();
    }
    
    std::ostringstream oss;
    oss << "HTTP/1.1 " << status << "\r\n"
    	<< "Content-Type: text/html\r\n"
    	<< "Content-Length: " << body.size() << "\r\n"
    	<< "Connection: close\r\n\r\n"
    	<< body;

    response = oss.str();
    ::send(fd, response.c_str(), response.size(), 0);
    ::close(fd);
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