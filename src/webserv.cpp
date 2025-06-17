#include "../includes/webserv.hpp"


TcpServer::TcpServer(std::string& ip_address, int port_address) 
: server_fd(-1), port(port_address), ip(ip_address), addr_len(sizeof(server_addr)) {
    if (!setupServerSocket()) {
        std::cerr << "Failed to set up server socket" << std::endl;
    }
    (void)addr_len;
}

TcpServer::~TcpServer() {

    for (std::vector<struct pollfd>::iterator it = poll_fds.begin(); it != poll_fds.end(); ++it) {

        close(it->fd);
    }
}

bool    TcpServer::setupServerSocket() {

    this->server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (this->server_fd < 0)
        return (false);
    fcntl(this->server_fd, F_SETFL, O_NONBLOCK);

    int opt = 1;
    setsockopt(this->server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    std::memset(&this->server_addr, 0, sizeof(this->server_addr));
    this->server_addr.sin_family = AF_INET;
    this->server_addr.sin_addr.s_addr = INADDR_ANY;
    this->server_addr.sin_port = htons(this->port);

    if (bind(this->server_fd, (struct sockaddr *)&this->server_addr, sizeof(this->server_addr)) < 0)
        return (false);
    if (listen(this->server_fd, SOMAXCONN) < 0)
        return (false);

    struct pollfd pfd;
    pfd.fd = this->server_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);

    std::cout << "Server listening on port: " << this->port << std::endl;
    return (true);
}

void    TcpServer::acceptNewClient() {

    int client_fd = accept(this->server_fd, NULL, NULL);
    if (client_fd < 0)
        return ;
    fcntl(client_fd, F_SETFL, O_NONBLOCK);

    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    poll_fds.push_back(pfd);
    std::cout << "New client connected: " << client_fd << std::endl;
}

void    TcpServer::removeClient(int index) {

    int client_fd = poll_fds[index].fd;
    std::cout << "Client disconnected: " << client_fd << std::endl;
    close(client_fd);
    poll_fds.erase(poll_fds.begin() + index);
    client_buffers.erase(client_fd);
}

void    TcpServer::handleClientData(int client_fd) {

    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, BUFFER_SIZE);
    int bytes = recv(client_fd, buffer, BUFFER_SIZE - 1, 0);

    if (bytes <= 0) {

        for (size_t i = 1; i < poll_fds.size(); ++i) {
            if (poll_fds[i].fd == client_fd) {
                removeClient(i);
                return ;
            }
        }
    }
    else {
        std::string& request = client_buffers[client_fd];
        request += std::string(buffer, bytes);

        // Print the full HTTP request to the terminal
        std::cout << "=== HTTP Request from client " << client_fd << " ===" << std::endl;
        std::cout << request << std::endl;
        std::cout << "==============================================" << std::endl;

        if (request.find("\r\n\r\n") != std::string::npos) {
            std::string html = 
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Content-Length: 280\r\n"
                "\r\n"
                "<!DOCTYPE html>"
                "<html>"
                "<head><title>My Colored Page</title></head>"
                "<body style='background-color:#282c34; color:#61dafb; font-family:sans-serif; text-align:center; padding-top:50px;'>"
                "<h1>Hello, colorful world!</h1>"
                "<p>This is a basic HTML page with a background and colored text.</p>"
                "</body></html>";
            send(client_fd, html.c_str(), html.size(), 0);
            request.clear();
        }
    }
}

void    TcpServer::run() {

    while (true) {

        int ret = poll(&poll_fds[0], poll_fds.size(), -1);
        if (ret < 0) {
            std::cerr << "Poll failed" << std::endl;
            break ;
        }
        for (size_t i = 0; i < poll_fds.size(); ++i) {

            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == this->server_fd)
                    acceptNewClient();
            }
            else
                handleClientData(poll_fds[i].fd);
        }
    }
}
