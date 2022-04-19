#pragma once

#include "Parser.hpp"

#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <errno.h>
#include <string>
#include <vector>
#include <map>

#define OFFSET_OF(type, member) ((size_t)&(((type*)0)->member))

namespace we
{
    struct directive_data;

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
        };

    public:
        std::string                 pattern;
        long long                   client_body_timeout;
        bool                        client_body_in_file;
        long long                   client_body_buffer_size;
        long long                   client_max_body_size;

        std::vector<std::string>    index;
        std::string                 root;
        bool                        autoindex;
        bool                        allow_upload;
        std::string                 upload_dir;
        AllowedMethods              allowed_methods;

        bool                        is_allowed_method(std::string method) const;
    };

    class ServerBlock
    {
    public:
        int                                     listen_socket;
        std::string                             server_name;
        std::vector<struct sockaddr_in>         server_addrs;
        std::vector<int>                        server_socks;

        long long                               server_send_timeout;
        long long                               server_body_buffer_size;

        bool                                    keep_alive;
        long long                               keep_alive_timeout;

        bool                                    enable_lingering_close;
        long long                               lingering_close_timeout;

        std::vector<LocationBlock>              locations;
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
            MulNone,
            MulSelect,
            MulKqueue,
            MulPoll,
            MulEpoll
        };

    public:
        MultiplexingType                                                multiplex_type;

        long long                                                       client_header_timeout;
        long long                                                       client_header_buffer_size;
        long long                                                       client_max_header_size;

        std::map<int, std::vector<ServerBlock> >                        server_blocks;
        std::map<sockaddr, int, ComparSockAddr>                         server_socks;

        const ServerBlock*        get_server_block(int socket, const std::string &host) const;
    };

    bool    load_config(const std::string& file_name, Config& config);

    // data.args[0] == 10s | 20m | 30h
    void    set_time_directive(void *, we::directive_data const &, size_t);
    // data.args[0] == 1024 | -2048 | 4096 | -8192 | -16384 | 32768 | 65536 (long long)
    void    set_number_directive(void *, we::directive_data const &, size_t);
    // data.args[0] == 1024 | 2048 | 4096 | 8192 | 16384 | 32768 | 65536 (long long) positive
    void    set_unumber_directive(void *, we::directive_data const &, size_t);
    // data.args[0] == on | off
    void    set_boolean_directive(void *, we::directive_data const &, size_t);
    // data.args[0] == "some string"
    void    set_string_directive(void *, we::directive_data const &, size_t);
    // data.args[0] == 1mb | 2mb | 4mb | 8mb | 16mb | 32mb | 64mb | 128mb | 256mb | 512mb | 1024mb
    void    set_size_directive(void *, we::directive_data const &, size_t);

} // namespace we