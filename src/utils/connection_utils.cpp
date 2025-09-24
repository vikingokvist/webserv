#include "../../includes/Connection.hpp"
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

std::vector<Part>	parseMultipart(const std::string& body, const std::string& boundary) {
	
	std::vector<Part> parts;
	std::string delimiter = "--" + boundary;
	std::string endDelimiter = delimiter + "--";

	size_t start = 0;
	while (true) {
		size_t pos = body.find(delimiter, start);
		if (pos == std::string::npos) break;
		pos += delimiter.size();

		// Saltar CRLF
		if (body.substr(pos, 2) == "\r\n") pos += 2;

		// Si encontramos el delimitador final
		if (body.compare(pos, endDelimiter.size(), endDelimiter) == 0)
			break;

		// Buscar fin de headers
		size_t headerEnd = body.find("\r\n\r\n", pos);
		if (headerEnd == std::string::npos)
			break;

			
		std::string headers = body.substr(pos, headerEnd - pos);

		// Extraer Content-Type
		std::string content_type;
		size_t ct_pos = headers.find("Content-Type:");
		if (ct_pos != std::string::npos) {
			ct_pos += 14; // salto "Content-Type:"
			size_t ct_end = headers.find("\r\n", ct_pos);
			if (ct_end == std::string::npos) ct_end = headers.size();
				content_type = headers.substr(ct_pos, ct_end - ct_pos);

		}
		pos = headerEnd + 4; // saltar \r\n\r\n

		// Buscar siguiente boundary
		size_t next = body.find(delimiter, pos);
		if (next == std::string::npos) break;

		std::string content = body.substr(pos, next - pos);
		// Eliminar CRLF final del contenido si existe
		if (!content.empty() && content.substr(content.size()-2) == "\r\n") {
			content.erase(content.size()-2);
		}
		
		std::string filename;
		size_t find_filename = headers.find("filename=");
		if (find_filename != std::string::npos) {
			size_t startQuote = headers.find('"', find_filename);
			if (startQuote != std::string::npos) {
				++startQuote;
				size_t endQuote = headers.find('"', startQuote);
				if (endQuote != std::string::npos) {
					filename = headers.substr(startQuote, endQuote - startQuote);
				}
			}
		}
		Part p;
		p.headers = headers;
		p.content = content;
		p.filename = filename;
		p.content_type = content_type;
		parts.push_back(p);
		start = next;
	}
	return (parts);
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

