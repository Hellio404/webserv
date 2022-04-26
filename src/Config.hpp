#pragma once

#include "Regex.hpp"
#include "Parser.hpp"
#include "Response/Handler.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <algorithm>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <climits>
#include <cstring>
#include <string>
#include <vector>
#include <map>

#define OFFSET_OF(type, member) ((size_t)&(((type*)0)->member))

namespace we
{
    class Connection;

    struct directive_data;
    struct directive_block;

    struct directive_dispatch
    {
        const char *name;
        void (*func)(void *, directive_data const &, size_t);
        size_t offset;
    };

    class ComparSockAddr: public std::binary_function<sockaddr, sockaddr, bool>
    {
    public:
        bool operator()(const struct sockaddr& lhs, const struct sockaddr& rhs) const
        {
            const struct sockaddr_in* lhs_in = reinterpret_cast<const struct sockaddr_in*>(&lhs);
            const struct sockaddr_in* rhs_in = reinterpret_cast<const struct sockaddr_in*>(&rhs);

            return (lhs_in->sin_addr.s_addr < rhs_in->sin_addr.s_addr ||
                    (lhs_in->sin_addr.s_addr == rhs_in->sin_addr.s_addr &&
                        lhs_in->sin_port < rhs_in->sin_port)
                    );
        }
    };

    class LocationBlock
    {
    private:
        struct AllowedMethods
        {
            int get: 2;
            int post: 2;
            int put: 2;
            int head: 2;
            int del: 2;
        };
        

    public:
        enum Modifier
        {
            Modifier_none = 0,
            Modifier_exact = 1,
            Modifier_prefix = 2,
            Modifier_suffix = 3,
            Modifier_regex = 4,
            Modifier_regex_icase = 5,
        };

        std::string                                         pattern;
        Modifier                                            modifier;
        ft::Regex                                           *regex;
        long long                                           client_body_timeout;
        bool                                                client_body_in_file;
        long long                                           client_body_buffer_size;
        long long                                           client_max_body_size;

        bool                                                is_redirection;
        std::string                                         redirect_url;
        int                                                 return_code;

        std::map<int, std::string>                          error_pages;

        std::vector<std::string>                            index;
        std::string                                         root;
        bool                                                allowed_method_found;
        bool                                                autoindex;
        bool                                                allow_upload;
        std::string                                         upload_dir;
        AllowedMethods                                      allowed_methods;
        std::vector<std::vector<int (*)(Connection *)> >    handlers;

    public:
        LocationBlock();
        ~LocationBlock();
        LocationBlock(LocationBlock const &);

        std::string                 get_error_page(int status) const;
        bool                        is_allowed_method(std::string method) const;
    };

    class ServerBlock
    {
    public:
        std::string                             listen_addr;
        int                                     listen_socket;
        std::vector<std::string>                server_names;

        long long                               server_send_timeout;
        long long                               server_body_buffer_size;

        bool                                    keep_alive;
        long long                               keep_alive_timeout;

        bool                                    enable_lingering_close;
        long long                               lingering_close_timeout;

        std::vector<LocationBlock>              locations;

    public:
        ServerBlock();

        const LocationBlock*                    get_location(const std::string& uri) const;
    };

    class Config
    {
    public:
        typedef std::map<int, std::vector<ServerBlock> >::const_iterator    server_block_const_iterator;
        typedef std::map<int, std::vector<ServerBlock> >::iterator          server_block_iterator;
        typedef std::map<sockaddr, int, ComparSockAddr>::const_iterator     server_sock_const_iterator;
        typedef std::map<sockaddr, int, ComparSockAddr>::iterator           server_sock_iterator;

    public:
        enum MultiplexingType
        {
            MulNone = 0,
            MulSelect,
            MulKqueue,
            MulPoll,
            MulEpoll
        };

    public:
        MultiplexingType                                                multiplex_type;

        unsigned int                                                    max_internal_redirect;
        long long                                                       client_header_timeout;
        long long                                                       client_header_buffer_size;
        long long                                                       client_max_header_size;

        std::map<std::string, std::string>                              mime_types;
        std::string                                                     default_type;

        std::map<int, std::vector<ServerBlock> >                        server_blocks;
        std::map<sockaddr, int, ComparSockAddr>                         server_socks;

    public:
        Config();

        const ServerBlock*        get_server_block(int socket, const std::string &host) const;
        std::string               get_mime_type(const std::string &) const;
    };

    void        init_config();
    bool        load_config(const std::string &, Config &);

    void        init_root_directives(Config &, directive_block *);
    void        init_server_directives(Config &, ServerBlock &, directive_block *, directive_block *);
    void        init_location_directives(LocationBlock &, directive_block *, directive_block *, directive_block *);

    // Debug functions
    void        print_config_block_info(const Config & val);

} // namespace we

#include "Connection.hpp"
