#ifndef SERVERS_HPP
# define SERVERS_HPP

#include <vector>
#include <string>
#include "ConfigParser.hpp"
#include "ServerWrapper.hpp"


class Servers
{
    private:
        ConfigParser                        parser;
        std::vector<ServerWrapper>          servers;


    public:
        Servers() {};

        Servers(const Servers& src) {*this = src;};

        Servers& operator=(const Servers& src) {(void)src; return (*this);};

        ~Servers() {};

        Servers(const std::string& filename) : parser(filename) {
            const std::vector<ServerConfig>& configs = parser.getServers();
            for (size_t i = 0; i < configs.size(); ++i) {
                servers.push_back(ServerWrapper(configs[i]));
            }
        };
        
        ServerWrapper& operator[](size_t index) {
    
            if (index >= servers.size()) {
                throw std::out_of_range("Invalid server index");
            }
            return servers[index];
        };

        const ServerWrapper& operator[](size_t index) const {
        
            if (index >= servers.size()) {
                throw std::out_of_range("Invalid server index");
            }
            return servers[index];
        };

        void clearServers() {
            for (size_t i = 0; i < servers.size(); ++i) {
                int sock = servers[i].getSocket(i);
                if (sock > 0) {
                    close(sock);
                }
            }
            servers.clear();
        }

        size_t  size() const {return (servers.size());};
};

#endif
