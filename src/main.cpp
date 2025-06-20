#include "../includes/webserv.hpp"
#include "../includes/Servers.hpp"



int main(void)
{
    try
    {
        Servers servers("webserv.conf");

        for (size_t i = 0; i < servers.size(); i++)
        {
            std::cout << "=== Server [" << i << "] ===" << std::endl;

            size_t ipCount = servers[i].getIpCount();
            for (size_t j = 0; j < ipCount; ++j)
            {
                std::cout << "  IP[" << j << "]: " << servers[i].getIps(j) << std::endl;
                std::cout << "  Port[" << j << "]: " << servers[i].getPorts(j) << std::endl;
            }

            // size_t locCount = servers[i].getLocationCount();
            // for (size_t j = 0; j < locCount; ++j)
            // {
            //     std::cout << "  Location Path[" << j << "]: " << servers[i].getLocationPath(j) << std::endl;
            // }

            // std::cout << std::endl;
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return (0);
}
