#ifndef LOGGER_HPP
# define LOGGER_HPP

#include <iostream>
#include <string>
#include <map>
#include <ctime>
#include <iomanip>

#include "Connection.hpp"

const int SERVER_CONNECT = 1;
const int SERVER_DISCONNECT = 2;
const int CLIENT_CONNECT = 3;
const int CLIENT_TIME_OUT = 4;
const int CLIENT_DISCONNECT = 5;
const int CLIENT_REQUEST = 6;
const int CLIENT_BODY = 7;

#define LOGGER_RESET    "\033[0m"
#define LOGGER_RED      "\033[1;31m"
#define LOGGER_GREEN    "\033[1;92m"
#define LOGGER_YELLOW   "\033[93m" 
#define LOGGER_BLUE     "\033[34m"
#define LOGGER_BOLD_BLUE "\033[1;34m"
#define LOGGER_MAGENTA  "\033[1;35m"
#define LOGGER_CYAN     "\033[1;36m"
#define LOGGER_WHITE    "\033[37m"


class Logger {

    public:
        static void         logger(PollData& pd, int flag, int time_left); 
        static void         logger2(std::map<std::string, std::string> headers, int flag, int target_fd);

        static void         printServerConnect(PollData& pd, std::string time_stamp);
        static void         printServerDisconnect(PollData& pd, std::string time_stamp);
        static void         printClientConnect(PollData& pd, std::string time_stamp);
        static void         printClientDisconnect(PollData& pd, std::string time_stamp);
        static void         printClientTimeout(PollData& pd, std::string time_stamp, int time_left);
        static void         printClientRequest(std::map<std::string, std::string> headers, std::string time_stamp, int flag);
        static void         printSignalReceived(Servers& servers);

        static std::string  getTimestamp();
};

#endif

