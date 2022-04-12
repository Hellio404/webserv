#pragma once
#include <vector>
#include <string>
#include <sys/socket.h>
namespace we
{
 /*
    + ADD LOGGING
 */
    class LocationBlock
    {

        struct AllowedMethods {
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
        std::string                             server_name;
        std::vector<struct sockaddr_in>         server_addrs;
        std::vector<int>                        server_socks;

        long long                               client_header_timeout;
        long long                               client_header_buffer_size;
        long long                               client_max_header_size;

        long long                               server_send_timeout;
        long long                               server_body_buffer_size;

        bool                                    keep_alive;
        long long                               keep_alive_timeout;

        bool                                    enable_lingering_close;
        long long                               lingering_close_timeout;

        std::vector<LocationBlock>              locations;

    };

    class Config
    {


    public:
        enum MultiplixingType
        {
            MulNone,
            MulSelect,
            MulKqueue,
            MulPoll,
            MulEpoll
        };

    public:

        MultiplixingType    multiplex_type;
        
        std::vector<ServerBlock>    server_blocks;

    };

} // namespace we