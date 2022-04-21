#include "ResponseServer.hpp"

namespace we
{
    ResponseServer::ResponseServer() {}

    ResponseServer::ResponseServer(bool to_chunk) : last_read_bytes(0), offset(0), to_chunk(to_chunk), ended(false) {}

    ResponseServer::~ResponseServer() {}

    void ResponseServer::recalculate_offset()
    {
        if (this->last_read_bytes < this->data_start)
            return;
        this->last_read_bytes -= this->data_start;
        this->offset += std::min(size_t(this->last_read_bytes), this->data_end - this->data_start);
    }

    ssize_t &ResponseServer::transform_data(std::string &buffer)
    {
        if (this->to_chunk == false)
        {
            this->data_start = 0;
            this->data_end = buffer.size();
            return this->last_read_bytes;
        }
        std::string chunk_size_str = we::to_hex(buffer.size()) + "\r\n";
        this->data_start = chunk_size_str.size();
        buffer.insert(0, chunk_size_str);
        this->data_end = buffer.size();
        buffer.append("\r\n");
        return this->last_read_bytes;
    }

    ResponseServerDirectory::ResponseServerDirectory(std::string const& path, bool to_chunk): ResponseServer(to_chunk)
    {
        this->dir_path = path;
        this->load_directory_listing();
    }

    void    ResponseServerDirectory::load_directory_listing()
    {
        DIR             *dir;
        struct dirent   *dp;
        struct tm       *tm;
        struct stat     statbuf;
        char            datestring[256];

        this->response_buffer = "<!doctype html><html>"
                                "<head><title>Index of /</title></head>"
                                "<h1>Index of /</h1><hr><pre>";

        if ((dir = opendir(this->dir_path.c_str())) == NULL)
            throw std::runtime_error("Cannot open " + this->dir_path);

        do {
            if ((dp = readdir(dir)) == NULL)
                continue;

            if (!strcmp(dp->d_name, ".") || stat(dp->d_name, &statbuf) == -1)
                continue ;

            std::string tmp = dp->d_name;
            if (S_ISDIR(statbuf.st_mode))
                tmp += "/";

            tm = localtime(&statbuf.st_mtime);
            strftime(datestring, sizeof(datestring), "%d-%b-%Y %H:%M", tm);

            std::stringstream   tofill;
            tofill << "<a href=\"" + tmp + "\">" + tmp + "</a>";
            tofill << std::setw(80 - tmp.size()) << datestring;
            if (S_ISDIR(statbuf.st_mode) == 0)
                tofill << std::setw(40) << (intmax_t)statbuf.st_size;
            else
                tofill << std::setw(40) << '-';

            this->response_buffer += tofill.str() + "<br>";
        } while (dp != NULL);

        this->response_buffer += "</pre><hr></body></html>";
    }

    ResponseServerFile::ResponseServerFile(std::string const &filep, bool to_chunk = false) : ResponseServer(to_chunk)
    {
        file_path = filep;
        file.open(file_path.c_str(), std::ifstream::in);
        if (!file.is_open())
            throw std::runtime_error("Unable to open file");
    }

    ssize_t &ResponseServerFile::get_next_data(std::string &buffer, size_t size, bool &ended)
    {
        this->recalculate_offset();
        this->last_read_bytes = 0;

        if (this->ended)
            throw std::runtime_error("File ended");
        this->file.clear();
        this->file.seekg(offset, std::ios::beg);
        if (!this->file.good())
            throw std::runtime_error("Unable to seek in file");

        char _buffer[size + 1];
        this->file.read(_buffer, size);
        buffer.assign(_buffer, this->file.gcount());

        if (this->file.eof() && this->file.gcount() == 0)
        {
            this->ended = true;
            ended = true;
            if (to_chunk)
                buffer = "0\r\n\r\n";
            return last_read_bytes;
        }
        if (!this->file.eof() && !this->file.good())
            throw std::runtime_error("Unable to read file");
        return this->transform_data(buffer);
    }
}