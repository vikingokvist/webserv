/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:13:42 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/10 15:45:12 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
#define CONNECTION_HPP


#define BUFFER_SIZE 1024

class ServerWrapper;
#include "./ServerWrapper.hpp"
#include <iostream>
#include <map>
#include <iosfwd>
#include <dirent.h>


class Connection {
	
	private:
		int									_fd;
		char								_request[BUFFER_SIZE];
		std::map<std::string, std::string>	_headers;
		std::ifstream						_file;
		std::string							_full_path;
		ServerWrapper&						_server;
		
	public:
		Connection(ServerWrapper& _server);
	
		void								setFd(int _fd);
		bool								setRequest();
		bool 								setConnection(ServerWrapper& _server);
		void								setHeader(std::string index, std::string path);
		void								setFullPath(const std::string& full_path);
		
		std::string							getFullPath();
		int									getFd();
		char*								getRequest();
		std::string							getHeader(std::string index);
		std::ifstream&						getFile();
		std::string 						getContentType(const std::string& path);
		ServerWrapper&						getServer();
		
		bool								checkRequest();
		bool								saveRequest(char *str);
		bool								receiveRequest(ssize_t location_index);
		void								sendGetResponse();
		void								sendPostResponse();
		// void								sendDeleteResponse();	
		void								SendAutoResponse(const std::string &direction_path);
		void								send400Response(); // Línea de request mal formada
		void								send403Response(); //Acceso Denegado
		void								send404Response(); //  Ruta inválida o no existente
		void								send405Response(); // Método HTTP no soportado
		void								send505Response(); // Versión HTTP incorrecta
		
		bool								isMethodAllowed(Connection& connection, const std::string& method);
		ssize_t								getBestMatch(ServerWrapper& server, std::string req_path);
		bool								fileExistsAndReadable(const char* path);
		bool								isValidHttpVersion(const std::string& version);
		bool								isDirectory(const char* path);
		
};

#endif