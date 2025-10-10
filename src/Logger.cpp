#include "../includes/Logger.hpp"


void    Logger::logger(PollData& pd, int flag, int time_left) {

    std::string time_stamp = getTimestamp();

    if (flag == SERVER_CONNECT) printServerConnect(pd, time_stamp);
	else if (flag == SERVER_DISCONNECT) printServerDisconnect(pd, time_stamp);
	else if (flag == CLIENT_CONNECT) printClientConnect(pd, time_stamp);
	else if (flag == CLIENT_TIME_OUT) printClientTimeout(pd, time_stamp, time_left);
	else if (flag == CLIENT_DISCONNECT) printClientDisconnect(pd, time_stamp);
}

void    Logger::logger2(std::map<std::string, std::string> headers, int flag, int target_fd) {

    std::string time_stamp = getTimestamp();

    if (flag == CLIENT_REQUEST) printClientRequest(headers, time_stamp, target_fd);
}

void    Logger::printServerConnect(PollData& pd, std::string time_stamp) {
    //[14:32:21:23/05/2025] - Server[127.0.0.1:9998] - listening on socket(fd).
    std::cout << time_stamp << "Server" << pd.ip_port << LOGGER_BOLD_BLUE <<"listening on socket(" << pd.fd << ")." << LOGGER_RESET <<std::endl;
}

void    Logger::printServerDisconnect(PollData& pd, std::string time_stamp) {

    std::cout << time_stamp << "Server" << pd.ip_port << LOGGER_RED <<"disconnected from socket(" << pd.fd << ")." << LOGGER_RESET <<std::endl;
}

void    Logger::printClientConnect(PollData& pd, std::string time_stamp) {
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - connected to socket(fd).
    std::cout << time_stamp << "Client" << pd.ip_port << LOGGER_GREEN << "connected to socket(" << pd.fd << ")." << LOGGER_RESET <<std::endl;
}

void    Logger::printClientDisconnect(PollData& pd, std::string time_stamp) {
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - disconnected from socket(fd).
    std::cout << time_stamp << "Client" << pd.ip_port << LOGGER_RED << "disconnected from socket(" << pd.fd << ")." << LOGGER_RESET <<std::endl;
}

void    Logger::printClientTimeout(PollData& pd, std::string time_stamp, int time_left) {
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - 5s to timeout on socket(fd).
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - 4s to timeout on socket(fd).
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - 3s to timeout on socket(fd).
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - 2s to timeout on socket(fd).
    //[14:32:21:23/05/2025] - Client[127.0.0.1:9998] - 1s to timeout on socket(fd).
    std::cout << time_stamp << "Client" << pd.ip_port << LOGGER_YELLOW << time_left << "s to timeout from socket(" << pd.fd << ")." << LOGGER_RESET << std::endl;
}


void    Logger::printClientRequest(std::map<std::string, std::string> headers, std::string time_stamp, int target_fd) {
    
    std::cout << time_stamp << "Server[" << headers["Host"] << "] - " << LOGGER_GREEN << "received request from socket(" << target_fd << ")." << LOGGER_RESET <<std::endl;
    std::cout << LOGGER_BLUE << "------------------------------------------------------------------------------------------" << LOGGER_RESET << std::endl;
    std::map<std::string, std::string>::iterator it = headers.begin();
	for ( ; it != headers.end(); ++it) {
		std::cout << LOGGER_BLUE << "[" << it->first << "] = " << it->second << LOGGER_RESET << std::endl;
	}
    std::cout << LOGGER_BLUE << "------------------------------------------------------------------------------------------" << LOGGER_RESET << std::endl;
}

void         Logger::printSignalReceived(Servers& servers) {

    std::string time_stamp = getTimestamp();
    std::cout << std::endl << time_stamp << LOGGER_MAGENTA << "SIGINT signal received." << LOGGER_RESET << std::endl;
    for (size_t i = 0; i < servers.size(); i++) {
        for (size_t j = 0; j < servers[i].getIpCount(); j++) {
            std::ostringstream oss;
            oss << servers[i].getPorts(j);
            std::cout << time_stamp << "Server[" << servers[i].getIps(j) << ":" << oss.str() << "] - " << LOGGER_RED << "disconnected socket()." << LOGGER_RESET << std::endl;
        }
    }
    std::cout << time_stamp << LOGGER_MAGENTA << "All resources freed." << LOGGER_RESET << std::endl;
}

std::string     Logger::getTimestamp()
{
	time_t now = std::time(0);
	struct tm *lt = std::localtime(&now);

	char buf[32];
	std::strftime(buf, sizeof(buf), "[%H:%M:%S:%d:%m:%Y] - ", lt);

	return (std::string(buf));
}
