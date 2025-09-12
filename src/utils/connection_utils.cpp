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


bool			fileExistsAndReadable(const char* path) {
	
    struct stat st;
    if (stat(path, &st) != 0) {
        // No existe el archivo
        return (false);
    }
    if (!S_ISREG(st.st_mode)) {
        // No es un archivo regular
        return (false);
    }
    FILE* f = fopen(path, "r"); // CAMBIAR A OPEN
    if (!f) {
        // No se puede abrir para lectura (sin permiso)
        return (false);
    }
    fclose(f);// CAMBIAR A CLOSE
    return (true);
}

bool			isDirectory(const char* path) {

	struct stat st;
	return (stat(path, &st) == 0 && S_ISDIR(st.st_mode));
}

bool			isValidHttpVersion(const std::string& version) {
	
	return (version == "HTTP/1.0" || version == "HTTP/1.1");
}

