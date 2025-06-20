#ifndef SERVER_WRAPPER_HPP
# define SERVER_WRAPPER_HPP

# include "./ConfigParser.hpp"
# include "./webserv.hpp"


class ServerWrapper
{
    private:

        const ServerConfig* config;

    public:

        ServerWrapper();
        ServerWrapper(const ServerConfig* cfg);
        ServerWrapper(const ServerWrapper& src);
        ServerWrapper& operator=(const ServerWrapper& src);
        ~ServerWrapper();

        std::string             getIps(size_t ip_index) const;
        size_t                  getIpSize() const;
        int                     getPorts(size_t port_index) const;
        size_t                  getPortSize() const;
        std::string             getServerNames(size_t server_name_index) const;
        size_t                  getServerNameSize() const;
        std::string             getErrorPages(int error_page_index) const;
        int                     getErrorPageCode(size_t error_page_index) const;
        size_t                  getErrorPageSize() const;
        size_t                  getClientMaxBodySize() const;
        const LocationConfig&   getLocation(size_t index) const;
        size_t                  getLocationCount() const;
        std::string             getLocationPath() const;
        std::string             getLocationRoot() const;
        std::string             getLocationIndex() const;
        bool                    getIfRedirec() const;
        std::string             getRedirectCode() const;
        std::string             getCgiExtensions(size_t index) const;
        size_t                  getCgiExtensionSize() const;
        std::string             getUploadStore() const;      


};

#endif
