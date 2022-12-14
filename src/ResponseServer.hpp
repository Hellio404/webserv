#pragma once

#include "Utils.hpp"
#include <signal.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <map>
#include "Connection.hpp"
#include "ConnectionBase.hpp"
#include "HeaderParser.hpp"

namespace we
{
    class ResponseServer
    {
        ResponseServer();

    protected:
        ssize_t         last_read_bytes;
        ssize_t         buffer_offset;
        long long       buffer_size;
        unsigned int    status_code;
        std::string     str_status_code;
        bool            ended;
        std::string     internal_buffer;
        Connection      *connection;
        char *          _buffer;

    public:
        typedef std::multimap<std::string, std::string, LessCaseInsensitive>    HeaderMap;
        
        ResponseServer(Connection *);

        ssize_t&            get_next_data(std::string &, bool &) ;
        unsigned int        get_status_code();
        void                transform_data(std::string &);
        std::string         make_response_header(const HeaderMap&);

        virtual void        load_next_data(std::string &) = 0;
        virtual             ~ResponseServer();
    };

    class ResponseServerFile : public ResponseServer
    {
        std::ifstream   file;
        std::string     file_path;

    public:
        ResponseServerFile(Connection *);
        void    load_next_data(std::string &);
        ~ResponseServerFile();
    };

    class ResponseServerDirectory : public ResponseServer
    {
    protected:
        std::string     location;
        std::string     dir_path;
        std::string     response_buffer;
        long long       offset;


        void load_directory_listing();

    public:
        ResponseServerDirectory(Connection *);
        void    load_next_data(std::string &);
        ~ResponseServerDirectory();
    };

    class ResponseServerFileMultiRange : public ResponseServer
    {
        typedef std::pair<unsigned long long, unsigned long long>    Range;

        std::ifstream                               file;
        std::vector<std::string>::const_iterator    current_boundry;
        std::vector<Range>::iterator                current_range;
        std::vector<std::string>                    boundries;
        bool                                        add_header;

    public:
        ResponseServerFileMultiRange(Connection *);
        void    load_next_data(std::string &);
        ~ResponseServerFileMultiRange();
    };

    class ResponseServerFileSingleRange : public ResponseServer
    {
        typedef std::pair<unsigned long long, unsigned long long>    Range;

        std::ifstream                               file;
        long long                                   offset;
        Range                                       range;

    public:
        ResponseServerFileSingleRange(Connection *);
        void    load_next_data(std::string &);
        ~ResponseServerFileSingleRange();
    };

    class ResponseServerCGI : public ResponseServer, public BaseConnection
    {
        typedef std::multimap<std::string, std::string, LessCaseInsensitive>   HeaderMap;
        int             fds[2];
        pid_t           pid;
        bool            headers_ended;
        EventData       event_data;
        HeaderMap       cgi_headers;
        HeaderParser<HeaderMap> parser;
        char            **env;
        const Event     *event;
        std::string     header_buffer;

        bool set_environment();
        bool check_bad_exit();

    public:
        void timeout();
        ResponseServerCGI(Connection *);
        ~ResponseServerCGI();
        void    load_next_data(std::string &);
        void    handle_connection();
    };
}