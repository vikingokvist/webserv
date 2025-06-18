#include "../includes/webserv.hpp"
#include "../includes/ConfigParser.hpp"


void printParsedConfig(const std::vector<ServerConfig>& servers) {
    std::vector<ServerConfig>::const_iterator server_it = servers.begin();
    size_t server_idx = 1;

    for (; server_it != servers.end(); ++server_it, ++server_idx) {
        std::cout << "===== Server " << server_idx << " =====" << std::endl;

        // IPs and Ports
        std::cout << "Listen:" << std::endl;
        std::vector<std::pair<std::string, int> >::const_iterator ip_it = server_it->ips_and_ports.begin();
        for (; ip_it != server_it->ips_and_ports.end(); ++ip_it) {
            std::cout << "  - " << ip_it->first << ":" << ip_it->second << std::endl;
        }

        // Server Names
        std::cout << "Server Names:" << std::endl;
        std::vector<std::string>::const_iterator name_it = server_it->server_names.begin();
        for (; name_it != server_it->server_names.end(); ++name_it) {
            std::cout << "  - " << *name_it << std::endl;
        }

        // Error Pages
        std::cout << "Error Pages:" << std::endl;
        std::map<int, std::string>::const_iterator err_it = server_it->error_pages.begin();
        for (; err_it != server_it->error_pages.end(); ++err_it) {
            std::cout << "  - " << err_it->first << " => " << err_it->second << std::endl;
        }

        // Client Max Body Size
        std::cout << "Client Max Body Size: " << server_it->client_max_body_size << " bytes" << std::endl;

        // Locations
        std::cout << "Locations:" << std::endl;
        std::vector<LocationConfig>::const_iterator loc_it = server_it->locations.begin();
        for (; loc_it != server_it->locations.end(); ++loc_it) {
            std::cout << "  Path: " << loc_it->path << std::endl;
            if (!loc_it->root.empty())
                std::cout << "    Root: " << loc_it->root << std::endl;
            if (!loc_it->index.empty())
                std::cout << "    Index: " << loc_it->index << std::endl;

            // Methods
            std::cout << "    Methods:";
            std::set<std::string>::const_iterator m_it = loc_it->methods.begin();
            for (; m_it != loc_it->methods.end(); ++m_it) {
                std::cout << " " << *m_it;
            }
            std::cout << std::endl;

            // Autoindex
            std::cout << "    AutoIndex: " << (loc_it->auto_index ? "on" : "off") << std::endl;

            // Redirect
            if (!loc_it->redirect.empty()) {
                std::cout << "    Redirect: " << loc_it->redirect_code << " => " << loc_it->redirect << std::endl;
            }

            // CGI Extensions
            if (!loc_it->cgi_extensions.empty()) {
                std::cout << "    CGI Extensions:";
                std::set<std::string>::const_iterator cgi_it = loc_it->cgi_extensions.begin();
                for (; cgi_it != loc_it->cgi_extensions.end(); ++cgi_it) {
                    std::cout << " " << *cgi_it;
                }
                std::cout << std::endl;
            }

            // Upload Store
            if (!loc_it->upload_store.empty())
                std::cout << "    Upload Store: " << loc_it->upload_store << std::endl;
        }

        std::cout << std::endl;
    }
}

int main() {
    try {
        ConfigParser parser("webserv.conf");
        printParsedConfig(parser.getServers()); // assuming getServers() returns _servers
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
