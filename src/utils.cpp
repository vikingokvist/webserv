#include "../includes/HttpReceive.hpp"
#include "../includes/webserv.hpp"

std::map<std::string, std::string>	parseUrlEncoded(const std::string& body) {

	std::map<std::string, std::string> url_data_pairs;
	std::string key;
	std::string value;
	size_t i  = 0;
	
	for (; i < body.size(); i++) {

		if (body[i] != '=')
			key += body[i];
		if (body[i] == '=') {
			i++;
			for (; i < body.size(); i++) {
				
				if (body[i] != '&')
					value += body[i];
				if (body[i] == '&') {
					break ;
				}
			}
			url_data_pairs[key] = value;
			key = "";
			value = "";
		}
	}
	return (url_data_pairs);
}

void		removeSpaces(std::string& str1, std::string& str2) {

	while (!str1.empty() && isspace(str1[str1.size() - 1]))
		str1.erase(str1.size() - 1);

	while (!str2.empty() && isspace(str2[0]))
		str2.erase(0, 1);

}

bool	isNumber(const std::string &s) {

    if (s.empty())
		return (false);
    for (std::string::size_type i = 0; i < s.size(); ++i) {

        if (!std::isdigit(s[i]))
			return (false);
    }
    return (true);
}

int checkContentLength(const char *num_str, unsigned long max_size) {

    char *endptr = NULL;
    errno = 0;

    unsigned long val = strtoul(num_str, &endptr, 10);

    if ((errno == ERANGE) || val > ULONG_MAX)
		return (-1);
    if (val > max_size)
		return (-1);
    return (0);
}

bool	isValidHeaderName(std::string header_name) {

	for (size_t i = 0; i < header_name.size(); i++) {

		if (!(isalnum(header_name[i]) || header_name[i] == '-'))
			return (false);
	}
	return (true);
}

bool	isValidHeaderValue(std::string header_value) {

	for (size_t i = 0; i < header_value.size(); i++) {

		if (!((header_value[i] >= 32 && header_value[i] <= 126) || header_value[i] == '\t'))
			return (false);
	}
	return (true);
}

bool			isDirectory(const char* path) {

	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

bool			isValidHttpVersion(const std::string& version) {
	
	return (version == "HTTP/1.0" || version == "HTTP/1.1");
}

bool	isMissingRequiredHeaders(std::map<std::string, std::string> &headers) {

	if (headers.find("Method") == headers.end()
		|| headers.find("Path") == headers.end()
		|| headers.find("Host") == headers.end())
		return (true);
	return (false);
}

bool	isUnsupportedMethod(std::map<std::string, std::string> &headers) {

	std::map<std::string, std::string>::iterator it = headers.find("Method");
	if (it == headers.end())
		return (false);
	std::string method = it->second;
	if (method != "GET" && method != "POST" && method != "DELETE" && method != "HEAD")
		return (true);
	return (false);
}

bool	isPathTraversal(std::map<std::string, std::string> &headers) {

	if (headers["Path"].find("../") != std::string::npos)
		return (true);
	return (false);
}

bool	isUriTooLong(std::map<std::string, std::string> &headers) {

	if (headers["Path"].size() >= MAX_URI_SIZE)
		return (true);
	return (false);
}

bool	isInvalidHttpVersion(std::map<std::string, std::string> &headers) {

	return !(headers["Version"] == "HTTP/1.0" || headers["Version"] == "HTTP/1.1");
}

bool	isInvalidContentLength(std::map<std::string, std::string> &headers) {

	if (headers["Content-Length"].empty())
		return (false);
	if (!isNumber(headers["Content-Length"]))
		return (true);
	return (false);
}

bool	isMissingContentLengthForPost(std::map<std::string, std::string> &headers) {

	if (headers["Method"] == "POST" && headers["Content-Length"].empty())
		return (true);
	return (false);
}

bool	isContentLengthTooLarge(std::map<std::string, std::string> &headers, size_t max_client_size) {

	if (headers["Content-Length"].empty())
		return (false);
	size_t content_length = std::atoi(headers["Content-Length"].c_str());
	if (content_length > max_client_size)
		return (true);
	return (false);
}

bool	isMissingContentTypeForPost(std::map<std::string, std::string> &headers) {

	if (headers["Method"] == "POST"
		&& headers.find("Content-Type") == headers.end())
		return (true);
	return (false);
}

bool	clientHasCookiesEnabled(std::map<std::string, std::string> &headers) {

	if (headers.find("X-Cookies-Allowed") != headers.end() && headers["X-Cookies-Allowed"] != "false")
		return (true);
	return (false);
}


