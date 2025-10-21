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
#include <iomanip>


class HttpReceive;


class HttpSend
{
	
	public:
		static bool sendErr(int fd, HttpReceive& _request, int error_code);
		
		static bool	sendGetResponse(int fd, HttpReceive& _request);
		static bool	sendPostResponse(int fd, HttpReceive& _request);
		static bool	sendDeleteResponse(int fd, HttpReceive& _request);
		static bool	sendHeadResponse(int fd, HttpReceive& _request);
		static bool	sendAutoResponse(int fd, HttpReceive& _request, const std::string &direction_path);
		static bool	sendRedirectResponse(int fd, HttpReceive& _request, size_t best_match);
		static bool	sendCgiResponse(int fd, HttpReceive& _request);

		static bool send200(int fd, HttpReceive& _request) {return sendErr(fd, _request, 200); }
		static bool send201(int fd, HttpReceive& _request) {return sendErr(fd, _request, 201); }
		static bool send204(int fd, HttpReceive& _request) {return sendErr(fd, _request, 204); }
		static bool send301(int fd, HttpReceive& _request) {return sendErr(fd, _request, 301); }
		static bool send302(int fd, HttpReceive& _request) {return sendErr(fd, _request, 302); }
		static bool send400(int fd, HttpReceive& _request) {return sendErr(fd, _request, 400); }
		static bool send401(int fd, HttpReceive& _request) {return sendErr(fd, _request, 401); }
		static bool send403(int fd, HttpReceive& _request) {return sendErr(fd, _request, 403); }
		static bool send404(int fd, HttpReceive& _request) {return sendErr(fd, _request, 404); }
		static bool send405(int fd, HttpReceive& _request) {return sendErr(fd, _request, 405); }
		static bool send411(int fd, HttpReceive& _request) {return sendErr(fd, _request, 411); }
		static bool send413(int fd, HttpReceive& _request) {return sendErr(fd, _request, 413); }
		static bool send414(int fd, HttpReceive& _request) {return sendErr(fd, _request, 414); }
		static bool send415(int fd, HttpReceive& _request) {return sendErr(fd, _request, 415); }
		static bool send500(int fd, HttpReceive& _request) {return sendErr(fd, _request, 500); }
		static bool send501(int fd, HttpReceive& _request) {return sendErr(fd, _request, 501); }
		static bool send505(int fd, HttpReceive& _request) {return sendErr(fd, _request, 505); }
};

std::string		getStatusMsg(int error_code);
std::string		getContentType(const std::string& path);
std::string		getInterpreter(std::string filename);

#endif

