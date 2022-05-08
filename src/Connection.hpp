#pragma once

#include "Config.hpp"
#include "Multiplexing.hpp"
#include "FileUtility.hpp"
#include "EventLoop.hpp"
#include "Utils.hpp"
#include "Date.hpp"
#include "ResponseServer.hpp"
#include "HeaderParser.hpp"
#include "Response/Handler.hpp"
#include "BodyHandler.hpp"
#include "ConnectionBase.hpp"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdexcept>
#include <iostream>
#include <istream>
#include <ostream>
#include <vector>
namespace we
{
    class ResponseServer;
    class AMultiplexing;
    class Config;
    class ServerBlock;
    class LocationBlock;
    class BodyHandler;

   

    class Connection: public BaseConnection
    {
    private:
        typedef unsigned long long ull;

    public:
        enum Status
        {
            Read,
            ReadBody,
            Write,
            Idle,
            Timeout
        };

        enum ResponseType
        {
            ResponseType_None,
            ResponseType_File,
            ResponseType_RangeFile,
            ResponseType_Directory,
            ResponseType_CGI
        };

        // checkers for if body is expected after headers and if it's chucked or not

        // remaining data after end of headers
        std::vector<std::pair<ull, ull> >                                   ranges;

        std::string                                                         client_remaining_data;

        int                                                                 connected_socket;
        int                                                                 client_sock;
        struct sockaddr_storage                                             client_addr;
        socklen_t                                                           client_addr_len;
        std::string                                                         client_addr_str;

        bool                                                                client_started_header;
        const Config                                                        &config;
        AMultiplexing                                                       &multiplexing;
        EventLoop                                                           &loop;
        EventData                                                           event_data;
        const Event                                                         *client_timeout_event;

        bool                                                                is_http_10;
        Status                                                              status;

        const ServerBlock*                                                  server;
        const LocationBlock*                                                location;


        long long                                                           client_content_length;
        unsigned long long                                                  keep_alive:1;
        unsigned long long                                                  is_body_chunked:1;
        unsigned long long                                                  to_chunk:1;

        bool                                                                metadata_set;
        unsigned long long                                                  content_length;
        std::string                                                         mime_type;
        std::string                                                         etag;
        Date                                                                last_modified;


        char                                                                *client_headers_buffer;
        char                                                                *client_body_buffer;
        HeaderParser                                                        client_header_parser;
        ResponseServer                                                      *response_server;
        ResponseType                                                        response_type;
        Phase                                                               phase;

        unsigned int                                                        redirect_count;

        std::string                                                         expanded_url;
        std::string                                                         requested_resource;

        std::map<std::string, std::string, LessCaseInsensitive>             req_headers;
        std::multimap<std::string, std::string, LessCaseInsensitive>        res_headers;

        BodyHandler                                                         *body_handler;

        Connection(int, EventLoop&, const Config&, AMultiplexing&);
        ~Connection();
        void        handle_connection();
        void        timeout();

    private:
        typedef std::map<std::string, std::string, LessCaseInsensitive>     map_type;

    private:
        void        check_potential_body();
        bool        is_protocol_supported(const std::string &);
        bool        is_method_supported(const std::string &);
        bool        is_crlf(std::string::const_iterator, std::string::const_iterator);
        bool        is_allowed_header_char(char);
        void        check_for_absolute_uri();

        void        parse_request(const std::string &);
        bool        parse_path(const std::string &);
        std::string dedote_path(const std::string &);
        char        *skip_crlf(char *ptr, char*end);
        bool        end_of_headers(std::string::const_iterator start, std::string::const_iterator end);
        void        print_headers();
        void        process_handlers();
        bool        process_file_for_response();
        void        get_info_headers();
        void        reset();

    };
}