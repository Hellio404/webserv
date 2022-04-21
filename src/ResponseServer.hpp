#pragma once

#include "Utils.hpp"

#include <sys/stat.h>
#include <dirent.h>
#include <iostream>
#include <fstream>
#include <cstdlib>

namespace we
{
    class ResponseServer
    {
        ResponseServer();
    protected:
        ssize_t     last_read_bytes;
        ssize_t     offset;
        ssize_t     buffer_offset;
        bool        to_chunk;
        bool        ended;
        std::string internal_buffer;


    public:
        ResponseServer(bool);
        virtual ~ResponseServer();

        ssize_t &get_next_data(std::string &, size_t, bool &) ;
        virtual ssize_t &load_next_data(std::string &, size_t, bool &) = 0;
        ssize_t &transform_data(std::string &);
    };

    class ResponseServerFile : public ResponseServer
    {
        std::ifstream file;
        std::string file_path;
        unsigned int error_code;

    public:
        ResponseServerFile(std::string const &, unsigned int error_code, bool = false);
        ssize_t &load_next_data(std::string &buffer, size_t size, bool &ended);
    };

    class ResponseServerDirectory : public ResponseServer
    {
    protected:
        std::string dir_path;
        std::string response_buffer;

        void load_directory_listing();

    public:
        ResponseServerDirectory(std::string const &, bool);
        ssize_t &load_next_data(std::string &, size_t, bool &);
    };
}
