#include "../../includes/Connection.hpp"
#include "../../includes/ErrorResponse.hpp"
#include "../../includes/webserv.hpp"


std::string		getContentType(const std::string& path) {
	
	if (path.find(".css") != std::string::npos)
		return ("text/css");
	if (path.find(".html") != std::string::npos)
		return ("text/html");
	if (path.find(".js") != std::string::npos)
		return ("application/javascript");
	if (path.find(".png") != std::string::npos)
		return ("image/png");
	return ("text/plain");
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

