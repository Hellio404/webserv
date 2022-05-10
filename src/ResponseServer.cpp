#include "ResponseServer.hpp"

namespace we
{
    // ResponseServer
    ResponseServer::ResponseServer(Connection *connection) : connection(connection)
    {
        this->status_code = this->get_status_code();
        this->buffer_size = this->connection->server->server_body_buffer_size;
        this->_buffer = new char[this->buffer_size];
        this->last_read_bytes = 0;
        this->buffer_offset = 0;
        this->ended = false;
        this->internal_buffer = "";
        if (connection->req_headers["@method"] == "HEAD")
            this->ended = true;
    }

    void ResponseServer::transform_data(std::string &buffer)
    {
        if (this->connection->to_chunk == false)
            return;

        std::string chunk_size_str = we::to_hex(buffer.size()) + "\r\n";
        buffer.insert(0, chunk_size_str);
        buffer.append("\r\n");
        if (this->ended == true)
            buffer.append("0\r\n\r\n");

        return;
    }

    ssize_t &ResponseServer::get_next_data(std::string &buffer, bool &ended)
    {
        this->buffer_offset += this->last_read_bytes;
        this->last_read_bytes = 0;
        if (size_t(this->buffer_offset) >= this->internal_buffer.size())
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
            throw we::HTTPStatusException(500, "Internal Server Error");
        else
            this->status_code = std::atoi(resp_code->second.c_str());
        this->str_status_code = resp_code->second;
        return this->status_code;
    }

    std::string ResponseServer::make_response_header(const HeaderMap &headers)
    {
        std::string header = "HTTP/1.1 ";

        if (headers.find("@CGI_Status") != headers.end())
            header += headers.find("@CGI_Status")->second;
        else
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
        con->res_headers.erase("Transfer-Encoding");
        con->to_chunk = false;
        if (!this->file.is_open() && is_bodiless_response(this->status_code))
        {
            this->ended = true;
            con->res_headers.insert(std::make_pair("Content-Length", "0"));
            con->res_headers.insert(std::make_pair("Content-Type", "text/html"));

            // create the headers
            this->internal_buffer = this->make_response_header(this->connection->res_headers);
            return;
        }
        else if (!this->file.is_open() && this->status_code != 200)
        {
            std::string buf = get_default_page_code(this->status_code);
            this->ended = true;
            con->res_headers.insert(std::make_pair("Content-Length", we::to_string(buf.size())));
            con->res_headers.insert(std::make_pair("Content-Type", "text/html"));

            // create the headers
            this->internal_buffer = this->make_response_header(this->connection->res_headers);
            if (con->req_headers["@method"] != "HEAD")
                this->internal_buffer += buf;
            return;
        }
        else if (!this->file.is_open())
            throw we::HTTPStatusException(500, "Internal Server Error");

        con->res_headers.insert(std::make_pair("Content-Length", we::to_string(con->content_length)));
        con->res_headers.insert(std::make_pair("Content-Type", con->mime_type));
        try
        {
            con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        }
        catch (...)
        {
        }
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

    void ResponseServerFile::load_next_data(std::string &buffer)
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

    // ResponseServerFileMultiRange
    ResponseServerFileMultiRange::ResponseServerFileMultiRange(Connection *con) : ResponseServer(con)
    {
        if (con->ranges.size() < 2)
            throw std::runtime_error("Expected at least 2 ranges");

        this->file.open(con->requested_resource.c_str(), std::ifstream::in | std::ifstream::binary);
        if (!this->file.is_open())
            throw std::runtime_error("Unable to open file");

        std::vector<Range>::const_iterator it = con->ranges.begin();
        std::string *content_type = &con->mime_type;

        size_t boundry = we::get_next_boundry();
        size_t content_len = 0;
        while (it != con->ranges.end())
        {
            this->boundries.push_back(we::make_range_header(boundry, content_type, *it, con->content_length));
            content_len += this->boundries.back().size() + (it->second - it->first);
            ++it;
        }
        this->boundries.push_back("\r\n--" + we::to_string(boundry, 20, '0') + "--\r\n");
        content_len += this->boundries.back().size();
        con->res_headers.insert(std::make_pair("content-type", "multipart/byteranges; boundary=" + we::to_string(boundry, 20, '0')));
        con->res_headers.insert(std::make_pair("Content-Length", we::to_string(content_len)));
        try
        {
            con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        }
        catch (...)
        {
        }
        con->res_headers.insert(std::make_pair("ETag", con->etag));
        this->internal_buffer = this->make_response_header(this->connection->res_headers);
        this->current_boundry = this->boundries.begin();
        this->current_range = con->ranges.begin();
        this->add_header = true;
    }

    void ResponseServerFileMultiRange::load_next_data(std::string &buffer)
    {
        long long len = this->buffer_size;
        buffer.resize(this->buffer_size);
        size_t to_copy;
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
                    return;
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

    ResponseServerFileMultiRange::~ResponseServerFileMultiRange()
    {
    }
    // END OF ResponseServerFileMultiRange

    // ResponseServerDirectory
    ResponseServerDirectory::ResponseServerDirectory(Connection *con) : ResponseServer(con)
    {
        this->location = con->expanded_url;
        this->dir_path = con->requested_resource;
        this->offset = 0;
        this->load_directory_listing();
        this->internal_buffer = this->make_response_header(this->connection->res_headers);
    }

    void ResponseServerDirectory::load_next_data(std::string &buffer)
    {
        size_t read_bytes = std::min(this->response_buffer.size() - this->offset, (unsigned long long)this->buffer_size);
        memcpy(_buffer, this->response_buffer.c_str() + this->offset, read_bytes);
        _buffer[read_bytes] = '\0';
        buffer.assign(_buffer, read_bytes);
        this->offset += read_bytes;
        if (size_t(this->offset) == this->response_buffer.size())
            this->ended = true;
        return this->transform_data(buffer);
    }

    void ResponseServerDirectory::load_directory_listing()
    {
        DIR *dir;
        struct dirent *dp;
        struct tm *tm;
        struct stat statbuf;
        char datestring[256];

        if ((dir = opendir(this->dir_path.c_str())) == NULL)
            throw std::runtime_error("Cannot open " + this->dir_path);

        this->response_buffer = "<!doctype html><html><head>"
                                "<title>Index of " +
                                this->location + "</title></head>"
                                                 "<h1>Index of " +
                                this->location + "</h1><hr><pre>";
        do
        {
            if ((dp = readdir(dir)) == NULL)
                continue;

            std::string filepath = this->dir_path + "/" + dp->d_name;

            if (!strcmp(dp->d_name, ".") || stat(filepath.c_str(), &statbuf) == -1)
                continue;

            std::string tmp = dp->d_name;
            if (S_ISDIR(statbuf.st_mode))
                tmp += "/";

            tm = localtime(&statbuf.st_mtime);
            strftime(datestring, sizeof(datestring), "%d-%b-%Y %H:%M", tm);

            std::stringstream tofill;
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
        if (this->connection->to_chunk == false)
            this->connection->res_headers.insert(std::make_pair("Content-Length", we::to_string(this->response_buffer.size())));
        else if (this->connection->to_chunk)
            this->connection->res_headers.insert(std::make_pair("Transfer-Encoding", "chunked"));
    }

    ResponseServerDirectory::~ResponseServerDirectory()
    {
    }
    // END OF ResponseServerDirectory

    // ResponseServerFileSingleRange
    ResponseServerFileSingleRange::ResponseServerFileSingleRange(Connection *con) : ResponseServer(con)
    {
        this->offset = 0;

        if (con->ranges.size() != 1)
            throw std::runtime_error("Expected one range");
        this->file.open(con->requested_resource.c_str(), std::ifstream::in | std::ifstream::binary);
        if (!this->file.is_open())
            throw std::runtime_error("Unable to open file");

        this->range = con->ranges[0];
        std::string content_range = "bytes " + we::to_string(this->range.first) + "-" + we::to_string(this->range.second - 1) + "/" + we::to_string(con->content_length);
        con->res_headers.insert(std::make_pair("Content-Range", content_range));
        con->res_headers.insert(std::make_pair("Content-Length", we::to_string(this->range.second - this->range.first)));
        con->res_headers.insert(std::make_pair("Content-Type", con->mime_type));
        try
        {
            con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        }
        catch (...)
        {
        }
        con->res_headers.insert(std::make_pair("Etag", con->etag));
        this->internal_buffer = this->make_response_header(con->res_headers);
    }

    void ResponseServerFileSingleRange::load_next_data(std::string &buffer)
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
    // END OF ResponseServerFileSingleRange

    // ResponseServerCGI
    static void timeout_event(EventData data)
    {
        ResponseServerCGI *conn = reinterpret_cast<ResponseServerCGI *>(data.pointer);
        conn->timeout();
    }

    ResponseServerCGI::ResponseServerCGI(Connection *connection) : ResponseServer(connection), parser(&cgi_headers, -1, HeaderParser<Connection::RespHeaderMap>::State_Header, true)
    {
        this->headers_ended = false;
        int input_fd = 0;
        if (connection->body_handler)
        {
            input_fd = open(connection->body_handler->get_filename().c_str(), O_RDONLY);
            if (input_fd == -1)
                throw we::HTTPStatusException(500, "Internal Server Error");
        }
        if (pipe(fds) == -1)
        {
            if (input_fd)
                close(input_fd);
            throw we::HTTPStatusException(500, "Internal Server Error");
        }
        this->pid = fork();
        if (pid < 0)
        {
            if (input_fd)
                close(input_fd);
            close(fds[0]);
            close(fds[1]);
            throw we::HTTPStatusException(500, "Internal Server Error");
        }
        if (pid == 0)
        {
            if (dup2(fds[1], STDOUT_FILENO) == -1 || dup2(input_fd, STDIN_FILENO) == -1)
                exit(69);
            close(fds[0]);
            close(fds[1]);
            close(input_fd);
            this->set_environment();
            const char *argv[] = {connection->location->cgi.c_str(), connection->requested_resource.c_str(), NULL};
            execve(argv[0], (char **)argv, this->env);
            exit(69);
        }
        else
        {
            if (input_fd)
                close(input_fd);
            close(fds[1]);
            connection->multiplexing.add(fds[0], this, we::AMultiplexing::Read);
            event_data.pointer = this;
            this->event = &connection->loop.add_event(timeout_event, event_data, 30000);
        }

        if (this->connection->to_chunk)
            this->connection->res_headers.insert(std::make_pair("Transfer-Encoding", "chunked"));

        this->header_buffer = "";
    }

    void ResponseServerCGI::timeout()
    {
        this->event = NULL;
        if (!this->headers_ended)
        {
            Connection *con = this->connection;
            delete this;
            con->response_type = Connection::ResponseType_File;
            con->res_headers.erase("@response_code");
            con->res_headers.insert(std::make_pair("@response_code", "504"));
            con->requested_resource = con->location->get_error_page(504);
            con->set_keep_alive(false);
            con->status = Connection::Write;
            con->phase = Phase_End;
            con->metadata_set = false;
            post_access_handler(con);
            response_server_handler(con);
            con->set_timeout(con->server->server_send_timeout);
            return;
        }
        else
            delete this->connection;
    }

    void ResponseServerCGI::load_next_data(std::string &str)
    {
        str = "";
    }

    bool ResponseServerCGI::check_bad_exit()
    {
        int status;
        int ret = waitpid(this->pid, &status, WNOHANG);
        if (ret < 0 || (ret == pid && WEXITSTATUS(status) != 0))
        {
            Connection *con = this->connection;
            delete this;
            con->response_type = Connection::ResponseType_File;
            con->res_headers.erase("@response_code");
            con->res_headers.insert(std::make_pair("@response_code", "502"));
            con->requested_resource = con->location->get_error_page(502);
            con->set_keep_alive(false);
            con->status = Connection::Write;
            con->phase = Phase_End;
            con->metadata_set = false;
            post_access_handler(con);
            response_server_handler(con);
            con->set_timeout(con->server->server_send_timeout);
            return true;
        }
        return false;
    }

    void ResponseServerCGI::handle_connection()
    {
        this->connection->loop.remove_event(*this->event);
        this->event = &connection->loop.add_event(timeout_event, event_data, 30000);
        if (!headers_ended && check_bad_exit())
            return;
        if (size_t(this->buffer_size) <= internal_buffer.size())
            return;
        int ret;
        ret = read(fds[0], _buffer, buffer_size - internal_buffer.size());
        if (ret < 0)
        {
            this->connection->status = Connection::Close;
            return;
        }
        if (ret == 0) // EOF
        {
            this->connection->loop.remove_event(*this->event);
            this->event = NULL;
            kill(this->pid, SIGKILL);
            if (!ended && this->connection->to_chunk && headers_ended)
                internal_buffer.append("0\r\n\r\n");
            else if (!ended && !headers_ended && check_bad_exit())
                return;
            else if (!ended && !headers_ended)
            {
                internal_buffer = make_response_header(connection->res_headers);
                if (this->connection->to_chunk)
                    internal_buffer.append("0\r\n\r\n");
            }
            ended = true;
            this->connection->multiplexing.remove(fds[0]);
            return;
        }
        if (!headers_ended)
        {
            char *tmp_buff = _buffer;
            headers_ended = parser.append(tmp_buff, tmp_buff + ret);
            if (headers_ended)
            {
                std::string s(tmp_buff, _buffer + ret);
                if (cgi_headers.find("Status") != cgi_headers.end())
                {
                    connection->res_headers.erase("@response_code");
                    connection->res_headers.insert(std::make_pair("@CGI_Status", cgi_headers.find("Status")->second));
                    cgi_headers.erase("Status");
                }

                connection->res_headers.insert(cgi_headers.begin(), cgi_headers.end());
                header_buffer = make_response_header(connection->res_headers);
                if (s.size())
                    transform_data(s);
                internal_buffer.append(header_buffer);
                internal_buffer.append(s);
            }
        }
        else
        {
            std::string s(_buffer, ret);
            transform_data(s);
            internal_buffer.append(s);
        }
    }

    bool ResponseServerCGI::set_environment()
    {
        std::vector<std::string> env_v;

        env_v.push_back("REDIRECT_STATUS=200");
        env_v.push_back("GATEWAY_INTERFACE=CGI/1.1");
        env_v.push_back("SERVER_SOFTWARE=BetterNginx/0.69.420");
        env_v.push_back("SERVER_NAME=" + this->connection->server->server_names.front());

        env_v.push_back("REMOTE_ADDR=" + this->connection->client_addr_str);
        env_v.push_back("SERVER_PORT=" + this->connection->server->listen_port);
        env_v.push_back("REQUEST_METHOD=" + this->connection->req_headers["@method"]);
        env_v.push_back("SERVER_PROTOCOL=" + this->connection->req_headers["@protocol"]);

        env_v.push_back("PATH_INFO=" + this->connection->expanded_url);
        env_v.push_back("PATH_TRANSLATED=" + this->connection->expanded_url);
        env_v.push_back("SCRIPT_NAME=" + this->connection->requested_resource);
        env_v.push_back("SCRIPT_FILENAME=" + this->connection->requested_resource);

        if (this->connection->req_headers.find("@query") != this->connection->req_headers.end())
            env_v.push_back("QUERY_STRING=" + this->connection->req_headers["@query"]);
        if (this->connection->req_headers.find("Content-Type") != this->connection->req_headers.end())
            env_v.push_back("CONTENT_TYPE=" + this->connection->req_headers["Content-Type"]);
        if (connection->body_handler)
            env_v.push_back("CONTENT_LENGTH=" + we::to_string(connection->body_handler->filesize));

        Connection::ReqHeaderMap::const_iterator it = this->connection->req_headers.begin();
        for (; it != this->connection->req_headers.end(); ++it)
        {
            if (it->first[0] == '@')
                continue;
            env_v.push_back("HTTP_" + we::to_env_uppercase(it->first) + "=" + it->second);
        }

        this->env = new char *[env_v.size() + 1];
        for (size_t i = 0; i < env_v.size(); ++i)
        {
            this->env[i] = new char[env_v[i].size() + 1];
            memcpy(this->env[i], env_v[i].c_str(), env_v[i].size());
            this->env[i][env_v[i].size()] = '\0';
        }
        this->env[env_v.size()] = NULL;

        return true;
    }

    ResponseServerCGI::~ResponseServerCGI()
    {
        if (this->pid)
            kill(this->pid, SIGKILL);
        waitpid(this->pid, NULL, 0);
        this->connection->multiplexing.remove(fds[0]);
        this->connection->response_server = NULL;
        close(fds[0]);
        if (this->event)
            this->connection->loop.remove_event(*this->event);
    }
    // END OF ResponseServerCGI
}