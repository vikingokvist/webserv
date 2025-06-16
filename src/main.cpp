#include "../includes/webserv.hpp"


int main() {
    std::string ip = "0.0.0.0";
    TcpServer server(ip, 8080);
    server.run();
    return 0;
}