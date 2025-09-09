/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.cpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:19:49 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/09 12:25:46 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "../includes/Connection.hpp"
#include "../includes/ErrorResponse.hpp"


// --------------------- Constructor ---------------------

Connection::Connection() {}

// ----------------------- Getters -----------------------

int Connection::getFd() {
	return this->_fd;
}

char* Connection::getRequest() {
	return this->_request;
}

std::string Connection::getHeader(std::string index) {
	return this->_headers[index];
}

std::ifstream& Connection::getFile() {
	return this->_file;
}

std::string Connection::getFullPath() {
	return this->_full_path;
}

ServerWrapper*	Connection::getServer() {
	return this->_server;
}

// ----------------------- Setters -----------------------

void	Connection::setServer(ServerWrapper *_server) {
	this->_server = _server;
}

void Connection::setFd(int fd) {
	this->_fd = fd;
}

void Connection::setHeader(std::string index,std::string value) {
	this->_headers[index] = value;
}

void Connection::setFullPath(const std::string& full_path) {
	this->_full_path = full_path;
}


// -------------------- Utilidad --------------------

std::string Connection::getContentType(const std::string& path) {
	if (path.find(".css") != std::string::npos)
		return "text/css";
	if (path.find(".html") != std::string::npos)
		return "text/html";
	if (path.find(".js") != std::string::npos)
		return "application/javascript";
	if (path.find(".png") != std::string::npos)
		return "image/png";
	return "text/plain";
}

// -------------------- Responses HTTP --------------------

void Connection::sendGetResponse() {
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

void Connection::sendPostResponse() {
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

// Función para enviar respuestas con error estándar

void Connection::send400Response() {
	ErrorResponse::send400(getFd());
}

void Connection::send404Response() {
	ErrorResponse::send404(getFd());
}

void Connection::send405Response() {
	ErrorResponse::send405(getFd());
}

void Connection::send505Response() {
	ErrorResponse::send505(getFd());
}