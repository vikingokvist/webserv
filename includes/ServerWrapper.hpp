#ifndef SERVER_WRAPPER_HPP
# define SERVER_WRAPPER_HPP


#include "./webserv.hpp"


class ServerWrapper
{
    private:

        const ServerConfig*                 config;
        int	                                _fd;                        // Descriptor de socket, se inicializa internamente
		std::vector<std::string>            _server_name;              // server_name localhost;
		uint16_t                            _port;                     // listen 9999;
		in_addr_t                           _host;                     // host 127.0.0.1;
		sa_family_t                         _sin_family;               // Tipo de socket, normalmente AF_INET
		unsigned long		                _client_max_body_size;     // client_max_body_size 5;
		struct sockaddr_in	                _server_adress;            // Se construye con host, puerto y familia
		std::vector<LocationConfig>         _locations;
        std::vector<Endpoint>               endpoints;
        std::vector<int>                    sockets;

    public:

        ServerWrapper();
        ServerWrapper(const ServerConfig& cfg);
        ServerWrapper(const ServerWrapper& src);
        ServerWrapper& operator=(const ServerWrapper& src);
        ~ServerWrapper();


        void addEndpoint(const std::string &ip, uint16_t port) {
            endpoints.push_back(Endpoint(ip, port));
        }
        const std::vector<Endpoint>& getEndpoints() const {
            return endpoints;
        }
        void addSocket(int sock) {
            sockets.push_back(sock);
        }
        const std::vector<int>& getSockets() const {
            return sockets;
        }
        std::string                         getIps(size_t ip_index) const;
        size_t                              getIpCount() const;
        uint16_t                            getPorts(size_t port_index) const;
        size_t                              getPortCount() const;
        std::string                         getServerName(size_t server_name_index) const;
        std::vector<std::string>            getServerNamesList(void) const;
        size_t                              getServerNameCount() const;
        std::string                         getErrorFile(int error_page_index) const;
        std::string                         getErrorRoot(int error_page_index) const;
        size_t                              getErrorPageCount() const;
        unsigned long                       getClientMaxBodySize() const;
        const std::vector<std::string>&     getDefaultIndices() const;
        std::string                         getDefaultIndexFile(size_t index_file) const;
        size_t                              getDefaultIndexCount() const;
        std::string                         getDefaultRoot() const;
        const std::vector<LocationConfig>&  getLocations() const;
        const LocationConfig&               getLocation(size_t loc_index) const;
        const std::vector<std::string>&     getLocationIndices(size_t loc_index) const;
        size_t                              getLocationCount() const;
        std::string                         getLocationPath(size_t loc_index) const;
        std::string                         getLocationRoot(size_t loc_index) const;
        std::string                         getLocationIndexFile(size_t loc_index, size_t index_file) const;
        size_t                              getLocationIndexCount(size_t loc_index) const;
        bool                                getAutoIndex(size_t loc_index) const;
        size_t                              getRedirectCode(size_t loc_index) const;
        size_t                              getMethodsSize(size_t loc_index) const;
        std::set<std::string>               getMethods(size_t loc_index) const;
        std::string                         getMethod(size_t loc_index, size_t method_index) const;
        std::string                         getRedirect(size_t loc_index) const;
        std::string                         getCgiExtensions(size_t loc_index, size_t cgi_extension_index) const;
        size_t                              getCgiExtensionCount(size_t loc_index) const;
        std::string                         getUploadStore(size_t loc_index) const;
        uint16_t                            getCountIpPorts ();
        

        void                                setSocket(int _fd);
		void                                setServerName(const std::vector<std::string>& _server_name);
		void                                setPort(uint16_t _port);
		void                                setHost(in_addr_t _host);
		void                                setSinFamily(sa_family_t  _sin_family);
		void                                setMaxClientSize(unsigned long _client_max_body_size);
		void                                setLocations(const std::vector<LocationConfig>& _locations);


		int                                 getSocket() const;
		const std::vector<std::string>&     getServerName() const;
		uint16_t			                getPort() const;
		in_addr_t			                getHost() const;
		sa_family_t			                getSinFamily() const;
		unsigned long		                getMaxClientSize() const;
		struct sockaddr_in*	                getSockAddr();
        int                                 getFD() const;


        void	                            setupSockAddr();
		void	                            bindAndListen();
		void	                            setupSocket();
		void	                            setupServerConfig(const std::vector<std::string>& _server_name, uint16_t _port, in_addr_t _host,
				sa_family_t _sin_family, unsigned long _client_max_body_size);


};

#endif

