#include "../includes/webserv.hpp"
#include "../includes/Servers.hpp"

int main(void)
{
    try
    {
        Servers servers("webserv.conf");

        for (size_t i = 0; i < servers.size(); i++)
        {
            const ServerWrapper& server = servers[i];
            std::cout << "=== Server [" << i << "] ===\n";

            // IPs and Ports
            for (size_t j = 0; j < server.getIpCount(); ++j)
            {
                std::cout << "  IP[" << j << "]: " << server.getIps(j) << "\n";
                std::cout << "  Port[" << j << "]: " << server.getPorts(j) << "\n";
            }

            // Server Names
            for (size_t j = 0; j < server.getServerNameCount(); ++j)
            {
                std::cout << "  ServerName[" << j << "]: " << server.getServerNames(j) << "\n";
            }

            // Error Pages
            std::cout << "  ErrorPage[" << 0 << "]: " << server.getErrorPages(404) << "\n";
            std::cout << "  ErrorPage[" << 1 << "]: " << server.getErrorPages(500) << "\n";

            // Client Max Body Size
            std::cout << "  Client Max Body Size: " << server.getClientMaxBodySize() << "\n";

            // Locations
            for (size_t j = 0; j < server.getLocationCount(); ++j)
            {
                std::cout << "  --- Location [" << j << "] ---\n";
                std::cout << "    Path: " << server.getLocationPath(j) << "\n";
                std::cout << "    Root: " << server.getLocationRoot(j) << "\n";
                std::cout << "    Index: " << server.getLocationIndex(j) << "\n";
                std::cout << "    AutoIndex: " << (server.getAutoIndex(j) ? "on" : "off") << "\n";
                std::cout << "    Redirect: " << server.getRedirect(j) << "\n";
                std::cout << "    Redirect Code: " << server.getRedirectCode(j) << "\n";

                // Allowed Methods
                size_t k = 0;
                std::cout << "    Methods:";
                while (true)
                {
                    std::string method = server.getMethods(j, k++);
                    if (method.empty())
                        break;
                    std::cout << " " << method;
                }
                std::cout << "\n";

                // CGI Extensions
                std::cout << "    CGI Extensions:";
                for (size_t k = 0; k < server.getCgiExtensionCount(j); ++k)
                {
                    std::cout << " " << server.getCgiExtensions(j, k);
                }
                std::cout << "\n";

                // Upload Store
                std::cout << "    Upload Store: " << server.getUploadStore(j) << "\n";
            }

            std::cout << "\n";
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }

    return 0;
}
