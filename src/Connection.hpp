#pragma once

#include "Multiplexing.hpp"
#include "FileUtility.hpp"
#include "EventLoop.hpp"
#include "Config.hpp"
#include "Utils.hpp"

#include <sys/socket.h>
#include <stdexcept>
#include <iostream>
#include <istream>
#include <ostream>

namespace we
{
    class AMultiplexing;

    class Connection
    {
    public:
        enum Status {
            Read,
            Write,
            Idle,
            Timeout
        };

        // checkers for if body is expected after headers and if it's chucked or not
        bool                                                                is_body_expected;
        bool                                                                is_body_chunked;

        // remaining data after end of headers
        std::string                                                         client_remaining_data;

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
        void        handle_connection();

    private:
        typedef std::map<std::string, std::string, LessCaseInsensitive>     map_type;

    private:
        void        check_potential_body(const map_type &);
        bool        is_protocol_supported(const std::string &);
        bool        is_method_supported(const std::string &);
        bool        is_crlf(std::string::const_iterator, std::string::const_iterator);
        bool        is_allowed_header_char(char);
        void        check_for_absolute_uri();

        void        parse_request(const std::string &);
        bool        parse_path(const std::string &);
        std::string parse_absolute_path(const std::string &);
        char        *skip_crlf(char *ptr, char*end);
        bool        end_of_headers(std::string::const_iterator start, std::string::const_iterator end);
        void        print_headers();

        bool        process_file_for_response();
    };
}