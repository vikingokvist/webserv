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
        std::vector<ServerWrapper>           servers;


    public:
        Servers();
        Servers(const std::string& filename);
        Servers(const Servers& src);
        Servers& operator=(const Servers& src);
        ~Servers();

        ServerWrapper& operator[](size_t index);
        const ServerWrapper& operator[](size_t index) const;
        size_t size() const;

};

#endif
