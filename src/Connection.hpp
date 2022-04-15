#pragma once
# include "EventLoop.hpp"
# include <sys/socket.h>
# include "Config.hpp"
# include <istream>
# include <ostream>
# include "Multiplexing.hpp"
# include "Utils.hpp"
# include <stdexcept>
# include <iostream>

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
        bool                                                                client_started_header;
    
        const Config                                                        &config;
        AMultiplexing                                                       &multiplexing;
        EventLoop                                                           &loop;
        const Event                                                         *client_timeout_event;

        Status                                                              status;

        const ServerBlock*                                                  server;
        const LocationBlock*                                                location;
        
        char                                                                *client_headers_buffer;
        std::string                                                         client_headers;
        char                                                                *client_body_buffer;

        // std::istream                                                        requested_file;
        std::string                                                         requested_filepath;

        // std::ostream                                                        body_file;

        std::string                                                         start_line;
        std::map<std::string, std::string, LessCaseInsensitive>             req_headers;
        std::multimap<std::string, std::string, LessCaseInsensitive>        res_headers;

        Connection(int, EventLoop&, const Config&, AMultiplexing&);
        void    handle_connection();

    private:
        bool    is_protocol_supported(const std::string &);
        bool    is_method_supported(const std::string &);
        bool    is_crlf(std::string::const_iterator, std::string::const_iterator);
        bool    is_allowed_header_char(char);
        void    check_for_absolute_uri();

        void    parse_request(const std::string &);
        bool    parse_path(const std::string &);
        std::string parse_absolute_path(const std::string &);
        char *skip_crlf(char *ptr, char*end);
        bool end_of_headers(std::string::const_iterator start, std::string::const_iterator end);
        void print_headers();



    };

    

}

