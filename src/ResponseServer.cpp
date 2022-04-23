#include "ResponseServer.hpp"

namespace we
{
    static std::string get_default_error_str(unsigned int error_code)
    {
        const char *error_str = "<html>\n"
                                "<head><title>%1$d %2$s</title></head>\n"
                                "<body>\n"
                                "<center><h1>%1$d %2$s</h1></center>\n"
                                "<hr><center>BetterNginx/0.69.420 (HEAVEN)</center>\n"
                                "</body>\n"
                                "</html>\n";

        char buffer[2048];
        switch (error_code)
        {
        case 400:
            sprintf(buffer, error_str, 400, "Bad Request");
            break;
        case 401:
            sprintf(buffer, error_str, 401, "Unauthorized");
            break;
        case 402:
            sprintf(buffer, error_str, 402, "Payment Required");
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
        case 406:
            sprintf(buffer, error_str, 406, "Not Acceptable");
            break;
        case 407:
            sprintf(buffer, error_str, 407, "Proxy Authentication Required");
            break;
        case 408:
            sprintf(buffer, error_str, 408, "Request Timeout");
            break;
        case 409:
            sprintf(buffer, error_str, 409, "Conflict");
            break;
        case 410:
            sprintf(buffer, error_str, 410, "Gone");
            break;
        case 411:
            sprintf(buffer, error_str, 411, "Length Required");
            break;
        case 412:
            sprintf(buffer, error_str, 412, "Precondition Failed");
            break;
        case 413:
            sprintf(buffer, error_str, 413, "Request Entity Too Large");
            break;
        case 414:
            sprintf(buffer, error_str, 414, "Request-URI Too Long");
            break;
        case 415:
            sprintf(buffer, error_str, 415, "Unsupported Media Type");
            break;
        case 416:
            sprintf(buffer, error_str, 416, "Requested Range Not Satisfiable");
            break;
        case 417:
            sprintf(buffer, error_str, 417, "Expectation Failed");
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
        case 504:
            sprintf(buffer, error_str, 504, "Gateway Timeout");
            break;
        case 505:
            sprintf(buffer, error_str, 505, "HTTP Version Not Supported");
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
        if (this->ended)
            throw std::runtime_error("Directory response ended");

        char _buffer[size + 1];
        size_t read_bytes = std::min(this->response_buffer.size() - this->offset, size);
        memcpy(_buffer, this->response_buffer.c_str() + this->offset, read_bytes);
        _buffer[read_bytes] = '\0';
        buffer.assign(_buffer, read_bytes);
        this->offset += read_bytes;
        if (this->offset == this->response_buffer.size())
            this->ended = true;
        return this->transform_data(buffer);
    }

    ResponseServerDirectory::ResponseServerDirectory(std::string const& path, std::string const& location, bool to_chunk): ResponseServer(to_chunk)
    {
        this->location = location;
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

        this->response_buffer = "<!doctype html><html><head>"
                                "<title>Index of " + this->location + "</title></head>"
                                "<h1>Index of " + this->location + "</h1><hr><pre>";

        if ((dir = opendir(this->dir_path.c_str())) == NULL)
            throw std::runtime_error("Cannot open " + this->dir_path);

        do {
            if ((dp = readdir(dir)) == NULL)
                continue;

            std::string filepath = this->dir_path + "/" + dp->d_name;

            if (!strcmp(dp->d_name, ".") || stat(filepath.c_str(), &statbuf) == -1)
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

            this->response_buffer += tofill.str() + "\n";
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
            buffer = this->internal_buffer.substr(this->buffer_offset, size);
        return this->last_read_bytes;
    }

    ResponseServerFile::ResponseServerFile(std::string const &filep, unsigned int error_code, bool to_chunk) : ResponseServer(to_chunk)
    {
        this->file_path = filep;
        this->file.open(this->file_path.c_str(), std::ifstream::in);
        if (!this->file.is_open())
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
            this->ended = true;
        if (!this->file.eof() && !this->file.good())
            throw std::runtime_error("Unable to read file");
        return this->transform_data(buffer);
    }
}