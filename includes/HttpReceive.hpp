/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpReceive.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaimesan <jaimesan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:13:42 by jaimesan          #+#    #+#             */
/*   Updated: 2025/10/07 13:08:54 by jaimesan         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef HTTP_RECEIVE_HPP
 #define HTTP_RECEIVE_HPP

#define BUFFER_SIZE 8192
#define MAX_URI_SIZE 1000
#define ERROR_CREATE_FILE "Error: Could not create file in: "

#include "./HttpSend.hpp"
#include "./ServerWrapper.hpp"
#include "./ConfigParser.hpp"
#include <iostream>
#include <map>
#include <iosfwd>
#include <dirent.h>
#include <ctime> 
#include <string>

class ServerWrapper;

struct Part
{
    std::string headers;
    std::string content;
    std::string filename;
	std::string	content_type;
};

enum RecvStatus
{
	RECV_INCOMPLETE,
	RECV_COMPLETE,
	RECV_ERROR,
	RECV_PAYLOAD_TOO_LARGE_ERROR,
	RECV_CLOSED,
	RECV_HEADER_COMPLETE
};

class HttpReceive {
	
	private:
		int									_fd;
		char								_request[BUFFER_SIZE];
		std::map<std::string, std::string>	_headers;
		std::vector<Part>					 parts;
		std::ifstream						_file;
		std::string							_full_path;
		ServerWrapper&						_server;
		ssize_t								_best_match;
		std::string							_request_complete;
		std::string							_post_body;
		bool								_is_cgi_script;
		bool								_is_redirect;
		bool								_headers_parsed;
		

		typedef void						(HttpReceive::*Handler)();
		
	public:
		HttpReceive(ServerWrapper& _server);
		~HttpReceive();
	
		bool								prepareRequest();
		RecvStatus							receiveRequest();
		bool								saveRequest();
		
		
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
		size_t								getPostBodySize();
		std::string							getPostBody();
		
		
		bool								checkRequest();
		ssize_t								getBestMatch();
		ssize_t								findBestMatch(ServerWrapper& server, std::string req_path);
		bool								isRedirection();
		bool								isCgiScript();
		bool								isMethodAllowed(ServerWrapper& server, ssize_t best_match, std::string& method);		
		bool								fileExistsAndReadable(const char* path, int mode);
		void								printParserHeader(void);
		void								parseMultipart(const std::string& body, const std::string& boundary);
		bool								parseChunkedBody();

		
		void								sendGetResponse();
		void								sendPostResponse();
		void								sendDeleteResponse();
		void								sendAutoResponse(const std::string &direction_path);
		void								sendCgiResponse();
		void								sendRedirectResponse();
		bool								sendError(size_t error_code);
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
		void								send411Response();
		void								send413Response();
		void								send414Response();
		void								send415Response();
		void								send500Response();
		void								send501Response();
		void								send502Response();
		void								send503Response();
		void								send504Response();
		void								send505Response();
		
};

#endif