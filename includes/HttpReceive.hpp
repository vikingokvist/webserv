/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpReceive.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaimesan <jaimesan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:13:42 by jaimesan          #+#    #+#             */
/*   Updated: 2025/10/21 15:09:29 by jaimesan         ###   ########.fr       */
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
class Connection; 

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
	RECV_CLOSED
};

enum HeaderStatus {H_INCOMPLETE, H_COMPLETE};

enum BodyStatus {B_INCOMPLETE, B_COMPLETE};

enum BodyTransferType {UNSET, SET, PLAIN, CHUNKED, MULTIPART};

struct Session;

class HttpReceive {
			
	private:
		int									_fd;
		char								_request[BUFFER_SIZE];
		Connection*							_conn;
		std::map<std::string, std::string>	_headers;
		std::vector<Part>					 parts;
		std::ifstream						_file;
		std::string							_full_path;
		ServerWrapper&						_server;
		ssize_t								_best_match;
		std::string							_request_parse;
		std::string							_body_complete;
		HeaderStatus						header_state;
		BodyStatus							body_state;
		BodyTransferType					body_type;
		uint64_t							_total_bytes;
		bool								_is_cgi_script;
		bool								_is_redirect;
		bool								user_accepts_cookies;
		std::map<std::string, Session>		_session;
		bool								_is_autoindex;
		std::string							_autoindex_to;
		typedef bool						(HttpReceive::*Handler)();
		
	public:
		HttpReceive(ServerWrapper& _server, std::map<std::string, Session>& session);
		HttpReceive(ServerWrapper& server, std::map<std::string, Session>& session, Connection* conn)
			:  _conn(conn) , _server(server){
			this->_session = session;
			this->user_accepts_cookies = false;
			this->_is_cgi_script = false;
			this->_is_redirect = false;
			this->body_state = B_INCOMPLETE;
			this->header_state = H_INCOMPLETE;
			this->body_type = UNSET;
			this->_total_bytes = 0;
			this->_is_autoindex = false;
		}
		~HttpReceive();
		bool								prepareRequest();
		RecvStatus							receiveRequest();
		void 								resetForNextRequest();
		bool								checkRequest();
		void								setFd(int _fd);
		int									getFd();
		std::string							getFullPath();
		char*								getRequest();
		std::string							getHeader(std::string index);
		std::ifstream&						getFile();
		ServerWrapper&						getServer();
		size_t								getPostBodySize();
		std::string							getPostBody();
		bool getIsAutoIndex() { return _is_autoindex; }
		std::string getAutoIndex() { return _autoindex_to; }

		std::map<std::string, Session>&		getSession();
		void								setClientCookie();
		bool								hasClientCookie();
		bool								sendOutErr(size_t error_code);
		bool								sendGetResponse();
		bool								sendPostResponse();
		bool								sendDeleteResponse();
		bool								sendHeadResponse();
		bool								sendAutoResponse(const std::string &direction_path);
		bool								sendCgiResponse();
		bool								sendRedirectResponse();
		bool								sendError(size_t error_code);
		bool								send200Response();
		bool								send201Response();
		bool								send204Response();
		bool								send301Response();
		bool								send302Response();
		bool								send400Response();
		bool								send401Response();
		bool								send403Response();
		bool								send404Response();
		bool								send405Response();
		bool								send411Response();
		bool								send413Response();
		bool								send414Response();
		bool								send415Response();
		bool								send500Response();
		bool								send501Response();
		bool								send505Response();
		bool								isRedirection();
		bool								isCgiScript();
	
	private:
		bool								parseHeader(std::string header_complete);
		void								setBestMatch(ssize_t _best_match);	
		bool								methodGET(ServerWrapper &server, size_t best_match);
		bool								methodDELETE(ServerWrapper &server, size_t best_match);
		bool								methodPOST(ServerWrapper &server, size_t best_match);
		bool								methodHEAD(ServerWrapper &server, size_t best_match);
		ssize_t								getBestMatch();
		ssize_t								findBestMatch(ServerWrapper& server, std::string req_path);
		bool								isMethodAllowed(ServerWrapper& server, ssize_t best_match, std::string& method);		
		bool								fileExistsAndReadable(const char* path, int mode);
		void								parseMultipart(const std::string& body, const std::string& boundary);
		bool								parseChunkedBody(std::string& _body_recv);
		void								logger(std::map<std::string, std::string> _headers, int flag);
		void								setHeader(std::string index, std::string path);
		void								setFullPath(const std::string& full_path);
	
};

#endif