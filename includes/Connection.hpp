/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   Connection.hpp                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaimesan <jaimesan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:13:42 by jaimesan          #+#    #+#             */
/*   Updated: 2025/09/16 11:59:53 by jaimesan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CONNECTION_HPP
#define CONNECTION_HPP


#define BUFFER_SIZE 6000

class ServerWrapper;
#include "./ServerWrapper.hpp"
#include <iostream>
#include <map>
#include <iosfwd>
#include <dirent.h>

#include "ErrorResponse.hpp"

extern std::string							_previus_full_path;



class Connection {
	
	private:
		int									_fd;
		char								_request[BUFFER_SIZE];
		std::map<std::string, std::string>	_headers;
		std::string							_post_body;
		std::string							_post_body_file_name;
		std::ifstream						_file;
		std::string							_full_path;
		ServerWrapper&						_server;
		ssize_t								_best_match;	
		
	public:
		Connection(ServerWrapper& _server);
		~Connection();
	
		bool 								setConnection(ServerWrapper& _server, int listening_fd);
		bool								prepareRequest();
		bool								recieveRequest();
		bool								saveRequest(char *_request);
		void								sendGetResponse();
		void								sendPostResponse();
		void								SendAutoResponse(const std::string &direction_path);
		
		bool								savePostBodyFile(std::string post_body);
		void								printParserHeader(void);
		void								setBestMatch(ssize_t _best_match);	
		void								setFd(int _fd);
		void								setHeader(std::string index, std::string path);
		void								setFullPath(const std::string& full_path);
		std::string							getFullPath();
		int									getFd();
		char*								getRequest();
		std::string							getHeader(std::string index);
		std::ifstream&						getFile();
		ServerWrapper&						getServer();
		bool								checkRequest();
		ssize_t								getBestMatch();
		ssize_t								getBestMatch(ServerWrapper& server, std::string req_path);
		bool								isMethodAllowed(Connection& connection, const std::string& method);
		void								removeSpaces(std::string& str1, std::string& str2);
		
		
		void								send200Response();
		void								send201Response();
		void								send204Response();
		void								send301Response();
		void								send302Response();
		void								send400Response();
		void								send401Response();
		void								send403Response();
		void								send404Response();
		void								send405Response();
		void								send413Response();
		void								send414Response();
		void								send500Response();
		void								send501Response();
		void								send502Response();
		void								send503Response();
		void								send504Response();
		void								send505Response();

		// void								sendDeleteResponse();	
		
		
};

#endif