#include "ResponseServer.hpp"

namespace we
{
    // ResponseServer

    ResponseServer::ResponseServer(Connection *connection) : connection(connection)
    {
        this->status_code = this->get_status_code();
        this->buffer_size = this->connection->location->client_body_buffer_size;
        this->_buffer = new char[this->buffer_size];
        this->last_read_bytes = 0;
        this->buffer_offset = 0;
        this->ended = false;
        this->internal_buffer = "";
    }


    void    ResponseServer::transform_data(std::string &buffer)
    {
        if (this->connection->to_chunk == false)
            return ;
        
        std::string chunk_size_str = we::to_hex(buffer.size()) + "\r\n";
        buffer.insert(0, chunk_size_str);
        buffer.append("\r\n");
        if (this->ended == true)
            buffer.append("0\r\n\r\n");
        return ; 
    }
    
    ssize_t &ResponseServer::get_next_data(std::string &buffer, bool &ended)
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
            this->load_next_data(this->internal_buffer);
            buffer = this->internal_buffer;
            this->buffer_offset = 0;
        }
        else
            buffer = this->internal_buffer.substr(this->buffer_offset, this->buffer_size);
        return this->last_read_bytes;
    }

    unsigned int ResponseServer::get_status_code()
    {
        HeaderMap::const_iterator resp_code = this->connection->res_headers.find("@response_code");
        if (resp_code == this->connection->res_headers.end())
            throw std::runtime_error("ResponseServer::make_response_header: @response_code not found");
        else
            this->status_code = std::atoi(resp_code->second.c_str());
        this->str_status_code = resp_code->second;
        return this->status_code;
    }

     std::string ResponseServer::make_response_header(const HeaderMap& headers)
    {
        std::string header = "HTTP/1.1 ";
        // print all headers
       
        header += this->str_status_code + " " + we::get_status_string(this->status_code);
        header += "\r\n";
        for (HeaderMap::const_iterator it = headers.begin(); it != headers.end(); ++it)
        {
            if (it->first[0] == '@')
                continue;
            header += we::capitalize_header(it->first) + ": " + it->second + "\r\n";
        }
        header += "\r\n";
        return header;
    }

    ResponseServer::~ResponseServer() 
    {
        delete[] this->_buffer;
    }

    // END OF ResponseServer


    // ResponseServerFile

    ResponseServerFile::ResponseServerFile(Connection *con) : ResponseServer(con)
    {
        this->file.open(con->requested_resource.c_str(), std::ifstream::in | std::ifstream::binary);
        con->to_chunk = false;
        if (!this->file.is_open() && this->status_code / 100 > 2)
        {
            this->ended = true;
            std::string buf = get_default_page_code(this->status_code);
            con->res_headers.insert(std::make_pair("Content-Length", we::to_string(buf.size())));
            con->res_headers.insert(std::make_pair("Content-Type", "text/html"));
            // create the headers
            this->internal_buffer = this->make_response_header(this->connection->res_headers); 
            this->internal_buffer += buf;
            return ;
        }
        else if (!this->file.is_open())
            throw   std::runtime_error("File not opened");

        con->res_headers.insert(std::make_pair("Content-Length",  we::to_string(con->content_length) ));
        con->res_headers.insert(std::make_pair("Content-Type", con->mime_type));
        con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        con->res_headers.insert(std::make_pair("Etag", con->etag));

        if (this->status_code == 200)
            con->res_headers.insert(std::make_pair("Accept-Range", "bytes"));

        this->internal_buffer = this->make_response_header(this->connection->res_headers);
    }

    ResponseServerFile::~ResponseServerFile()
    {
        if (this->file.is_open())
            this->file.close();
    }


    void    ResponseServerFile::load_next_data(std::string &buffer)
    {
        this->file.read(this->_buffer, this->buffer_size);
        buffer.assign(this->_buffer, this->file.gcount());
        if (this->file.eof())
            this->ended = true;
        if (!this->file.eof() && !this->file.good())
            throw std::runtime_error("Unable to read file");
        return this->transform_data(buffer);
    }

    // END OF ResponseServerFile

    ResponseServerFileMultiRange::ResponseServerFileMultiRange(Connection *con) : ResponseServer(con)
    {
        if (con->ranges.size() < 2)
            throw std::runtime_error("Expected at least 2 ranges");

        this->file.open(con->requested_resource.c_str(), std::ifstream::in | std::ifstream::binary);
        if (!this->file.is_open())
            throw std::runtime_error("Unable to open file");
    
        std::vector<Range>::const_iterator it = con->ranges.begin();
        std::string *content_type = &con->mime_type; // TODO: always mime_type ?

        size_t  boundry = we::get_next_boundry();
        size_t  content_len = 0;
        while (it != con->ranges.end())
        {
            this->boundries.push_back(we::make_range_header(boundry, content_type, *it, con->content_length));
            content_len += this->boundries.back().size() + (it->second - it->first);
            ++it;
        }
        this->boundries.push_back("\r\n--" + we::to_string(boundry, 20, '0') +"--\r\n");
        content_len += this->boundries.back().size();
        con->res_headers.insert(std::make_pair("content-type", "multipart/byteranges; boundary=" + we::to_string(boundry, 20, '0')));
        con->res_headers.insert(std::make_pair("Content-Length",  we::to_string(content_len) ));
        con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        con->res_headers.insert(std::make_pair("ETag", con->etag));
        this->internal_buffer = this->make_response_header(this->connection->res_headers);
        this->current_boundry = this->boundries.begin();
        this->current_range = con->ranges.begin();
        this->add_header = true;
    }


    void    ResponseServerFileMultiRange::load_next_data(std::string &buffer)
    {
        long long len = this->buffer_size;
        buffer.resize(this->buffer_size);
        size_t  to_copy;
        buffer.clear();

        while (len > 0)
        {
            if (this->add_header)
            {
                buffer += *this->current_boundry;
                len -= this->current_boundry->size();
                ++this->current_boundry;
                this->add_header = false;

                if (this->current_boundry == this->boundries.end())
                {
                    this->ended = true;
                    return ;
                }
                continue;
            }
            to_copy = std::min((unsigned long long)len, this->current_range->second - this->current_range->first);

            this->file.seekg(this->current_range->first, std::ios::beg);
            this->file.read(this->_buffer, to_copy);
            to_copy = std::min((size_t)this->file.gcount(), to_copy);
            buffer += std::string(this->_buffer, to_copy);
            len -= to_copy;
            this->current_range->first += to_copy;
            if (this->current_range->second <= this->current_range->first)
            {
                ++this->current_range;
                this->add_header = true;
            }
        }
    }

    ResponseServerFileMultiRange::~ResponseServerFileMultiRange() {}






    // ResponseServerDirectory

    ResponseServerDirectory::ResponseServerDirectory(Connection *con): ResponseServer(con)
    {
        this->location = con->expanded_url;
        this->dir_path = con->requested_resource;
        this->offset = 0;
        this->load_directory_listing();
        this->internal_buffer = this->make_response_header(this->connection->res_headers);
    }

    void    ResponseServerDirectory::load_next_data(std::string &buffer)
    {
        size_t read_bytes = std::min(this->response_buffer.size() - this->offset, (unsigned long long)this->buffer_size);
        memcpy(_buffer, this->response_buffer.c_str() + this->offset, read_bytes);
        _buffer[read_bytes] = '\0';
        buffer.assign(_buffer, read_bytes);
        this->offset += read_bytes;
        if (this->offset == this->response_buffer.size())
            this->ended = true;
        return this->transform_data(buffer);
    }

    void    ResponseServerDirectory::load_directory_listing()
    {
        DIR             *dir;
        struct dirent   *dp;
        struct tm       *tm;
        struct stat     statbuf;
        char            datestring[256];

        if ((dir = opendir(this->dir_path.c_str())) == NULL)
            throw std::runtime_error("Cannot open " + this->dir_path);
        
        this->response_buffer = "<!doctype html><html><head>"
                                "<title>Index of " + this->location + "</title></head>"
                                "<h1>Index of " + this->location + "</h1><hr><pre>";
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

        closedir(dir);

        this->connection->res_headers.insert(std::make_pair("Content-Type", "text/html"));
        if (this->connection->is_http_10 || this->connection->to_chunk == false)
            this->connection->res_headers.insert(std::make_pair("Content-Length", we::to_string(this->response_buffer.size())));
        else if (this->connection->to_chunk)
            this->connection->res_headers.insert(std::make_pair("Transfer-Encoding", "chunked"));
    }

    ResponseServerDirectory::~ResponseServerDirectory()
    {
    }

    // END OF ResponseServerDirectory

    // ResponseServerFileSingleRange

    ResponseServerFileSingleRange::ResponseServerFileSingleRange(Connection *con): ResponseServer(con)
    {
        this->offset = 0;

        if (con->ranges.size() != 1)
            throw std::runtime_error("Expected one range");
         this->file.open(con->requested_resource.c_str(), std::ifstream::in | std::ifstream::binary);
        if (!this->file.is_open())
            throw std::runtime_error("Unable to open file");

        this->range = con->ranges[0];
        std::string content_range = "bytes " + we::to_string(this->range.first) + "-" + we::to_string(this->range.second - 1) + "/" + we::to_string(con->content_length);
        con->res_headers.insert(std::make_pair("Content-Range",  content_range));
        con->res_headers.insert(std::make_pair("Content-Length",  we::to_string(this->range.second - this->range.first)));
        con->res_headers.insert(std::make_pair("Content-Type", con->mime_type));
        con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        con->res_headers.insert(std::make_pair("Etag", con->etag));
        this->internal_buffer = this->make_response_header(con->res_headers);
    }

    void    ResponseServerFileSingleRange::load_next_data(std::string &buffer)
    {
        size_t to_copy;

        to_copy = std::min((unsigned long long)this->buffer_size, this->range.second - this->range.first);

        this->file.seekg(this->range.first, std::ios::beg);
        this->file.read(this->_buffer, to_copy);
        to_copy = (size_t)this->file.gcount();
        buffer += std::string(this->_buffer, to_copy);
        this->range.first += to_copy;
        if (this->range.second <= this->range.first)
            this->ended = true;
    }


    ResponseServerFileSingleRange::~ResponseServerFileSingleRange()
    {
        this->file.close();
    }
}


/*

size_t get_next_boundry();

std::string make_range_header(size_t boundry, std::string const *Content-Type, pair<ull, ull> range, ull size);

*/
