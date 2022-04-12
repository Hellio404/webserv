#pragma once
# include "EventLoop.hpp"
# include <sys/socket.h>
# include "Config.hpp"
# include <istream>
# include <ostream>
namespace we
{
    class ComparLessCaseInsensitive: public std::binary_function<std::string, std::string, bool>
    {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const
        {
            return (lhs.size() < rhs.size() || (lhs.size() == rhs.size() &&
                    strcasecmp(lhs.c_str(), rhs.c_str()) < 0));
        }
    };

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
        EventLoop                                                           &loop;
        Event                                                               *client_timeout_event;
        Status                                                              status;
        const ServerBlock*                                                  server;
        const LocationBlock*                                                location;
        const Config                                                        &config;

        char                                                                *client_headers_buffer;
        char                                                                *client_body_buffer;

        std::istream                                                        requested_file;
        std::string                                                         requested_filepath;

        std::ostream                                                        body_file;

        std::string                                                         start_line;
        std::map<std::string, std::string, ComparLessCaseInsensitive>       req_headers;
        std::multimap<std::string, std::string, ComparLessCaseInsensitive>  res_headers;

        Connection(int, EventLoop&, const Config&);
    };
}

