#include "../../includes/HttpReceive.hpp"
#include "../../includes/webserv.hpp"

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

