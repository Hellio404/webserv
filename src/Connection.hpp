#pragma once
# include "EventLoop.hpp"
# include <sys/socket.h>
# include "Config.hpp"
# include <istream>
# include <ostream>
# include "Multiplexing.hpp"
# include "Utils.hpp"
# include <stdexcept>

namespace we
{
    class AMultiplexing;

    class Connection {
    public:
        enum Status {
            Read,
            Write,
            Idle,
            Timeout
        };
        int                                                                 connected_socket;
        int                                                                 client_sock;
        struct sockaddr_storage                                             client_addr;
        socklen_t                                                           client_addr_len;
    
        const Config                                                        &config;
        AMultiplexing                                                       &multiplexing;
        EventLoop                                                           &loop;
        const Event                                                         *client_timeout_event;

        Status                                                              status;

        const ServerBlock*                                                  server;
        const LocationBlock*                                                location;
        
        char                                                                *client_headers_buffer;
        char                                                                *client_body_buffer;

        std::istream                                                        requested_file;
        std::string                                                         requested_filepath;

        std::ostream                                                        body_file;

        std::string                                                         start_line;
        std::map<std::string, std::string, LessCaseInsensitive>             req_headers;
        std::multimap<std::string, std::string, LessCaseInsensitive>        res_headers;

        Connection(int, EventLoop&, const Config&, AMultiplexing&);
    };

    

}

