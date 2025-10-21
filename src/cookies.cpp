
#include "../includes/webserv.hpp"

void addSession(std::map<std::string, Session>& session, const std::string& session_id) {
    Session s;
    s.session_id = session_id;
    s._current_time = std::time(0);
    session[session_id] = s;
};
  
double getSessionDuration(std::map<std::string, Session>& session, const std::string& session_id) {
    std::map<std::string, Session>::iterator it = session.find(session_id);
    if (it == session.end())
        return -1.0;

    time_t now = std::time(NULL);
    double seconds = difftime(now, it->second._current_time);
    return (seconds);
};
         
std::string parseSessionId(const std::string &cookie_header) {
    std::string key = "session_id=";
    std::size_t pos = cookie_header.find(key);
    if (pos == std::string::npos)
        return ("");
    pos += key.size();
    std::size_t end = cookie_header.find(";", pos);
    if (end == std::string::npos)
        end = cookie_header.size();
    return (cookie_header.substr(pos, end - pos));
};
    
std::string generateSessionId() {

    static bool seeded = false;
    if (!seeded) {
        std::srand(std::time(0));
        seeded = true;
    }
    std::ostringstream oss;
    for (int i = 0; i < 16; ++i) {
        int r = std::rand() % 256;
        oss << std::hex << std::setw(2) << std::setfill('0') << r;
    }
    return (oss.str());
}

std::string		ensureSession(std::map<std::string, Session>& session, std::string session_id, bool& is_new_session) {
        
    std::string client_session_id = parseSessionId(session_id);
    std::map<std::string, Session>::iterator it = session.find(session_id);
    is_new_session = false;

	if (client_session_id.empty() || it == session.end()) {
		std::string new_session_id = generateSessionId();
		addSession(session, new_session_id);
		is_new_session = true;
		return (new_session_id);
	}
	return (client_session_id);
};