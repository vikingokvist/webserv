/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpSend.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/19 13:32:55 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/30 15:28:17 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_SEND_HPP
# define HTTP_SEND_HPP

#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <fstream>

class HttpReceive;

class HttpSend {
	
	public:
		static void sendErr(int fd, HttpReceive& _connection, int error_code, const std::string& status);
		
		static void	sendGetResponse(int fd, HttpReceive& _connection);
		static void	sendPostResponse(int fd, HttpReceive& _connection, std::string _previous_full_path);
		static void	sendDeleteResponse(int fd, HttpReceive& _connection);
		static void	sendAutoResponse(int fd, HttpReceive& _connection, const std::string &direction_path);
		static void	sendCgiResponse(int fd, HttpReceive& _connection);
		static void send200(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 200, "200 OK"); }
		static void send201(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 201, "201 Created"); }
		static void send204(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 204, "204 No Content"); }
		static void send301(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 301, "301 Moved Permanently"); }
		static void send302(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 302, "302 Found"); }
		static void send400(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 400, "400 Bad Request"); }
		static void send401(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 401, "401 Unauthorized"); }
		static void send403(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 403, "403 Forbidden"); }
		static void send404(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 404, "404 Not Found"); }
		static void send405(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 405, "405 Method Not Allowed"); }
		static void send411(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 411, "411 Length Required"); }
		static void send413(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 413, "413 Payload Too Large"); }
		static void send414(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 414, "414 URI Too Long"); }
		static void send415(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 415, "415 Unsupported Media Type"); }
		static void send500(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 500, "500 Internal Server Error"); }
		static void send501(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 501, "501 Not Implemented"); }
		static void send502(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 502, "502 Bad Gateway"); }
		static void send503(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 503, "503 Service Unavailable"); }
		static void send504(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 504, "504 Gateway Timeout"); }
		static void send505(int fd, HttpReceive& _connection) { sendErr(fd, _connection, 505, "505 HTTP Version Not Supported"); }


};

std::string		getContentType(const std::string& path);

#endif
