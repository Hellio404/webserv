#include "ResponseServer.hpp"

namespace we
{

    std::string get_default_error_str(unsigned int error_code)
    {
        const char *error_str = "<html>\n"
                                "<head><title>%1$d %2$s</title></head>\n"
                                "<body>\n"
                                "<center><h1>%1$d %2$s</h1></center>\n"
                                "<hr><center>BetterNginx/0.69.420 (HEAVEN)</center>\n"
                                "</body>\n"
                                "</html>\n";
        // TODO: add more error codes
        char buffer[2048];
        switch (error_code)
        {
        case 400:
            sprintf(buffer, error_str, 400, "Bad Request");
            break;
        case 401:
            sprintf(buffer, error_str, 401, "Unauthorized");
            break;
        case 403:
            sprintf(buffer, error_str, 403, "Forbidden");
            break;
        case 404:
            sprintf(buffer, error_str, 404, "Not Found");
            break;
        case 405:
            sprintf(buffer, error_str, 405, "Method Not Allowed");
            break;
        case 500:
            sprintf(buffer, error_str, 500, "Internal Server Error");
            break;
        case 501:
            sprintf(buffer, error_str, 501, "Not Implemented");
            break;
        case 502:
            sprintf(buffer, error_str, 502, "Bad Gateway");
            break;
        case 503:
            sprintf(buffer, error_str, 503, "Service Unavailable");
            break;
        default:
            throw std::runtime_error("no default error string for this error code");
        }
        return std::string(buffer);
    }

    ResponseServer::ResponseServer(bool to_chunk) : 
    last_read_bytes(0), offset(0), to_chunk(to_chunk), ended(false), buffer_offset(0) {}

    ResponseServer::~ResponseServer() {}


    ssize_t &ResponseServer::transform_data(std::string &buffer)
    {
        if (this->to_chunk == false)
        {
            return this->last_read_bytes;
        }
        std::string chunk_size_str = we::to_hex(buffer.size()) + "\r\n";
        buffer.insert(0, chunk_size_str);
        buffer.append("\r\n");
        if (this->ended == true)
            buffer.append("0\r\n\r\n");
        return this->last_read_bytes;
    }
    ssize_t &ResponseServerDirectory::load_next_data(std::string &buffer, size_t size, bool &ended)
    {

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

    ssize_t &ResponseServer::get_next_data(std::string &buffer, size_t size, bool &ended)
    {
        this->buffer_offset += this->last_read_bytes;
        this->last_read_bytes = 0;
        if (this->buffer_offset >= this->internal_buffer.size())
        {
            if (this->ended == true)
            {
                ended = true;
                buffer = "";
                return this->last_read_bytes;
            }
            this->load_next_data(this->internal_buffer, size, ended);
            buffer = this->internal_buffer;
            this->buffer_offset = 0;
        }
        else
        {
            buffer = this->internal_buffer.substr(this->buffer_offset, size);
            if (this->ended)
                ended = true;
        }
        return this->last_read_bytes;
    }

    ResponseServerFile::ResponseServerFile(std::string const &filep, unsigned int error_code, bool to_chunk) : ResponseServer(to_chunk)
    {
        file_path = filep;
        file.open(file_path.c_str(), std::ifstream::in);
        if (!file.is_open())
        {
            this->ended = true;
            this->internal_buffer = get_default_error_str(error_code);
            this->transform_data(this->internal_buffer);
        }
    }

    ssize_t &ResponseServerFile::load_next_data(std::string &buffer, size_t size, bool &ended)
    {
        if (this->ended)
            throw std::runtime_error("File ended");
        this->file.clear();
        this->file.seekg(this->offset, std::ios::beg);
        if (!this->file.good())
            throw std::runtime_error("Unable to seek in file");

        char _buffer[size];
        this->file.read(_buffer, size);
        buffer.assign(_buffer, this->file.gcount());
        this->offset += this->file.gcount();
        if (this->file.eof())
        {
            this->ended = true;
            ended = true;
        }
        if (!this->file.eof() && !this->file.good())
            throw std::runtime_error("Unable to read file");
        return this->transform_data(buffer);
    }
}