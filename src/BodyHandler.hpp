#pragma once
#include "Connection.hpp"
#include "Chunker.hpp"
#include <fstream>
namespace we
{
    class Connection;

    class BodyHandler
    {
    private:
        /* data */
        Connection      *con;
        ChunckReader    *de_chunker;
        std::string     tmpfile_name;
        std::string     buffer_data;
        size_t          buffer_size;
        size_t          filesize;
        std::fstream    tmpfile;
        bool            is_moved;
        BodyHandler(/* args */);

    public:
        BodyHandler(Connection *);
        ~BodyHandler();

        void    open_tmpfile();
        void    move_tmpfile(std::string const& dest_path);
        bool    add_data(const char *, const char *);
        const std::string& get_filename();

    };
    
    
    
} // namespace we

