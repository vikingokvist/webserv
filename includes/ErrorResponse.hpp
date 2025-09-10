/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponse.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/19 13:32:55 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/10 15:39:24 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERROR_RESPONSE_HPP
#define ERROR_RESPONSE_HPP

#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>

class ErrorResponse {
	
	public:
		static void send(int fd, const std::string& status, const std::string& title, int contentLength = 85) {
			std::ostringstream oss;
			oss << "HTTP/1.1 " << status << "\r\n"
				<< "Content-Type: text/html\r\n"
				<< "Content-Length: " << contentLength << "\r\n"
				<< "Connection: close\r\n\r\n"
				<< "<!DOCTYPE html><html><head><title>" << title << "</title></head>"
				<< "<body><h1>" << title << "</h1></body></html>";
		
			std::string response = oss.str();
			::send(fd, response.c_str(), response.size(), 0);
			::close(fd);
		}

	static void send400(int fd) { send(fd, "400 Bad Request", "400 Bad Request"); }
	static void send403(int fd) { send(fd, "403 Forbidden Access", "403 Forbidden Access"); }
	static void send404(int fd) { send(fd, "404 Not Found", "404 Not Found"); }
	static void send405(int fd) { send(fd, "405 Method Not Allowed", "405 Method Not Allowed"); }
	static void send505(int fd) { send(fd, "505 Version Not Supported", "505 Version Not Supported"); }
};

#endif
