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
        ServerWrapper(const ServerConfig& cfg);
        ServerWrapper(const ServerWrapper& src);
        ServerWrapper& operator=(const ServerWrapper& src);
        ~ServerWrapper();

        std::string             getIps(size_t ip_index) const;
        size_t                  getIpCount() const;
        int                     getPorts(size_t port_index) const;
        size_t                  getPortCount() const;
        std::string             getServerNames(size_t server_name_index) const;
        size_t                  getServerNameCount() const;
        std::string             getErrorPages(int error_page_index) const;
        size_t                  getErrorPageCount() const;
        size_t                  getClientMaxBodySize() const;
        const LocationConfig&   getLocation(size_t loc_index) const;
        size_t                  getLocationCount() const;
        std::string             getLocationPath(size_t loc_index) const;
        std::string             getLocationRoot(size_t loc_index) const;
        std::string             getLocationIndex(size_t loc_index, size_t index_file) const;
        size_t                  getLocationIndexCount(size_t loc_index) const;
        bool                    getAutoIndex(size_t loc_index) const;
        size_t                  getRedirectCode(size_t loc_index) const;
        std::string             getMethods(size_t loc_index, size_t method_index) const;
        std::string             getRedirect(size_t loc_index) const;
        std::string             getCgiExtensions(size_t loc_index, size_t cgi_extension_index) const;
        size_t                  getCgiExtensionCount(size_t loc_index) const;
        std::string             getUploadStore(size_t loc_index) const;      

};

#endif
