#pragma once
#include <string>
#include <fstream>
#include "Utils.hpp"

namespace we
{
    class ChunckReader
    {
        static const size_t max_chunk_size = 2147483647;

    private:
        bool            wait_for_size;
        size_t          next_chunk_size;
        bool            skip_crlf;
        bool            skip_lf;
        bool            skip_to_crlf;
        std::fstream    &file;
        std::string     data_buffer;
        size_t          data_buffer_size;

    public:
        ChunckReader(std::fstream &f, size_t buffer_size);

        bool add_to_chunk(const char *buffer, const char *end);

        bool trailer_part_ended(const char *&buffer, const char *end);

        std::string get_leftover_data();

        void    add_to_data_buffer(const char *&buffer, const char *end);

        void add_to_chunksize(const char *&buffer, const char *end);

        void get_next_chunked_size(const char *it, const char *end);

        bool add_to_chunk(const std::string &str);

        ~ChunckReader();
    };
}

