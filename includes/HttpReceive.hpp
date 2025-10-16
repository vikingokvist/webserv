/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   HttpReceive.hpp                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: jaimesan <jaimesan@student.42.fr>          +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/06/17 13:13:42 by jaimesan          #+#    #+#             */
/*   Updated: 2025/10/16 15:40:31 by jaimesan         ###   ########.fr       */
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
		typedef void						(HttpReceive::*Handler)();
		
		
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

		std::map<std::string, Session>&		getSession();
		void								setClientCookie();
		bool								hasClientCookie();
		bool								sendOutErr(size_t error_code);
		void								sendGetResponse();
		void								sendPostResponse();
		void								sendDeleteResponse();
		void								sendHeadResponse();
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