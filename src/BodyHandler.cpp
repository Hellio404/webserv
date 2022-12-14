
#include "BodyHandler.hpp"

namespace we
{
    BodyHandler::BodyHandler(Connection *con) : con(con)
    {
        this->is_moved = false;
        this->de_chunker = NULL;
        this->filesize = 0;
        if (!con->location)
            throw we::HTTPStatusException(500, "Location not set");
        this->open_tmpfile();
        this->buffer_size = con->location->client_body_buffer_size;
        if (con->is_body_chunked)
        {
            de_chunker = new ChunckReader(this->tmpfile, this->buffer_size);
            if (!de_chunker)
                throw we::HTTPStatusException(500, "Could not create chunk reader");
        }
        else
            buffer_data.reserve(this->buffer_size);
    }

    bool BodyHandler::add_data(const char *start, const char *end)
    {
        if (de_chunker)
        {
            bool ret = de_chunker->add_to_chunk(start, end);
            if (ret)
                con->client_remaining_data = std::string(start, end);
            filesize = de_chunker->get_file_size();
            if (filesize > con->location->client_max_body_size)
                throw we::HTTPStatusException(413, "Request Entity Too Large");
            if (ret)
                tmpfile.close();
            return ret;
        }
        if ((end - start) + filesize + buffer_data.size() >= size_t(con->client_content_length))
        {
            buffer_data.append(start, end);
            tmpfile.write(buffer_data.c_str(), con->client_content_length - filesize);
            con->client_remaining_data = std::string(buffer_data.begin() + con->client_content_length - filesize, buffer_data.end());
            filesize = con->client_content_length;
            tmpfile.close();
            return true;
        }
        else if (buffer_data.size() + (end - start) < buffer_size)
            buffer_data.append(start, end);
        else
        {
            tmpfile.write(buffer_data.c_str(), buffer_data.size());
            filesize += buffer_data.size();
            buffer_data.clear();
            if (size_t(end - start) > this->buffer_size)
            {
                tmpfile.write(start, end - start);
                filesize += end - start;
            }
            else
                buffer_data.append(start, end);
        }
        return false;
    }

    void BodyHandler::open_tmpfile()
    {
        char tmp_file_name[] = "./tmp/we_better_nginx_tmpfile_XXXXXX";
        int fd = mkstemp(tmp_file_name);
        if (fd == -1)
            throw we::HTTPStatusException(500, "Could not create tmpfile");
        close(fd);
        tmpfile_name = tmp_file_name;
        tmpfile.open(tmpfile_name, std::ios::out | std::ios::binary);
        if (!tmpfile.is_open())
            throw we::HTTPStatusException(500, "Could not open tmpfile");
    }

    void BodyHandler::move_tmpfile(std::string const &dest_path)
    {
        if (de_chunker)
        {
            delete de_chunker;
            de_chunker = NULL;
        }
        if (tmpfile.is_open())
            tmpfile.close();
        if (rename(tmpfile_name.c_str(), dest_path.c_str()))
            throw we::HTTPStatusException(500, "Could not move tmpfile");
        this->is_moved = true;
        tmpfile_name = dest_path;
    }

    const std::string &BodyHandler::get_filename()
    {
        return tmpfile_name;
    }

    BodyHandler::~BodyHandler()
    {
        if (de_chunker)
            delete de_chunker;
        if (tmpfile.is_open())
            tmpfile.close();
        if (!is_moved)
            remove(tmpfile_name.c_str());
    }
}