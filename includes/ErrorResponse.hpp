/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorResponse.hpp                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/19 13:32:55 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/11 17:50:21 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERROR_RESPONSE_HPP
#define ERROR_RESPONSE_HPP

#include <string>
#include <sstream>
#include <unistd.h>
#include <sys/socket.h>
#include <fstream>

class Connection;

class ErrorResponse {
	
	public:
		static void send(int fd, Connection& _connection, int error_code, const std::string& status);
		
		static void send400(int fd, Connection& _connection) { send(fd, _connection, 400,"400 Bad Request"); }
		static void send403(int fd, Connection& _connection) { send(fd, _connection, 403,"403 Forbidden Access"); }
		static void send404(int fd, Connection& _connection) { send(fd, _connection, 404,"404 Not Found"); }
		static void send405(int fd, Connection& _connection) { send(fd, _connection, 405,"405 Method Not Allowed"); }
		static void send505(int fd, Connection& _connection) { send(fd, _connection, 505,"505 Version Not Supported"); }
};

#endif
