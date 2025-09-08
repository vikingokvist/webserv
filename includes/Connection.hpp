/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: ctommasi <ctommasi@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:13:42 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/08 16:32:42 by ctommasi         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "../includes/webserv.hpp"
#include "../includes/ConfigParser.hpp"

#define BUFFER_SIZE 1024

class ServerWrapper; // forward declaration

class Connection {
	
	private:
		int _fd;
		char _request[BUFFER_SIZE];
		std::map<std::string, std::string> _headers;
		std::ifstream	_file;
		std::string		_full_path;
		ServerWrapper*	_server;
		
	public:
		Connection();
	
		void		setFd(int _fd);
		bool		setRequest();
		bool 		setConnection(ServerWrapper _server);
		void		setHeader(std::string index, std::string path);
		void		setFullPath(const std::string& full_path);
		void		setServer(ServerWrapper* server);
		
		std::string		getFullPath();
		int				getFd();
		char*			getRequest();
		std::string		getHeader(std::string index);
		std::ifstream&	getFile();
		std::string 	getContentType(const std::string& path);
		ServerWrapper*	getServer();
		
		bool		checkRequest();
		bool		saveRequest(char *str);
		
		bool		receiveRequest(LocationConfig _location);
		
		void		sendGetResponse();
		void		sendPostResponse();
		// void		sendDeleteResponse();

		void		send400Response(); // Línea de request mal formada
		void		send404Response(); //  Ruta inválida o no existente
		void		send405Response(); // Método HTTP no soportado
		void		send505Response(); // Versión HTTP incorrecta
		
};

#endif