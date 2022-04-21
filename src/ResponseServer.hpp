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
    protected:
        ssize_t last_read_bytes;
        ssize_t offset;
        bool to_chunk;
        bool ended;
        std::string buffer;
        size_t data_start;
        size_t data_end;

        ResponseServer();

    public:
        ResponseServer(bool);
        virtual ~ResponseServer();

        virtual ssize_t &get_next_data(std::string &, size_t, bool &) = 0;
        ssize_t &transform_data(std::string &);
        void recalculate_offset();
    };

    class ResponseServerFile : public ResponseServer
    {
        std::ifstream file;
        std::string file_path;

    public:
        ResponseServerFile(std::string const &, bool);
        ssize_t &get_next_data(std::string &buffer, size_t size, bool &ended);
    };

    class ResponseServerDirectory : public ResponseServer
    {
    protected:
        std::string dir_path;
        std::string response_buffer;

        void load_directory_listing();

    public:
        ResponseServerDirectory(std::string const &, bool);
        ssize_t &get_next_data(std::string &, size_t, bool &);
    };
}
