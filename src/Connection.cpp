#include "Connection.hpp"

namespace we
{
    Connection::Connection(int connected_socket, EventLoop &loop, const Config &config, AMultiplexing &multiplexing) : config(config), multiplexing(multiplexing), loop(loop)
    {
        std::cerr << "New connection " << connected_socket  << std::endl;
        this->connected_socket = connected_socket;
        this->server = NULL;
        this->location = NULL;
        this->client_headers_buffer = NULL;
        this->client_body_buffer = NULL;
        this->client_started_header = false;
        this->is_body_expected = false;

        this->client_sock = accept(connected_socket, (struct sockaddr *)&this->client_addr, &this->client_addr_len);

        if (this->client_sock == -1)
            throw std::runtime_error("accept() failed");
        
        multiplexing.add(this->client_sock, this, AMultiplexing::Read);
        this->client_headers_buffer = new char[4096];
        this->status = Connection::Read;

    }

    char *Connection::skip_crlf(char *ptr, char*end)
    {
        while (ptr != end && (*ptr == '\r' || *ptr == '\n'))
            ptr++;
        if (ptr != end)
            this->client_started_header = true;
        return ptr;
    }

    bool Connection::end_of_headers(std::string::const_iterator start, std::string::const_iterator end)
    {
        size_t  to_skip = 0;
        while (start != end)
        {
            if (*start == '\r' && *(start + 1)   == '\n' && *(start + 2) == '\r' && *(start + 3) == '\n')
            {
                to_skip = 4;
                break ;
            }
            if (*start == '\r' && *(start + 1) == '\n' && *(start + 2) == '\n')
            {
                to_skip = 3;
                break ;
            }
            if (*start == '\n' && *(start + 1) == '\r' && *(start + 2) == '\n')
            {
                to_skip = 3;
                break ;
            }
            if (*start == '\n' && *(start + 1) == '\n')
            {
                to_skip = 2;
                break ;
            }
            ++start;
        }
        // TODO: move the rest of the data to the body string
        if (to_skip > 0)
        {
            this->client_remaining_data = std::string(start + to_skip, end);
            this->client_headers.erase(start + to_skip, end);
            return true;
        }
        return false;
    }

    void Connection::print_headers()
    {
        std::map<std::string, std::string, we::LessCaseInsensitive>::const_iterator it;
        for (it = this->req_headers.begin(); it != this->req_headers.end(); ++it)
        {
            std::cout << it->first << ": " << '\'' << it->second << '\'' << std::endl;
        }
    }

    void    Connection::handle_connection()
    {
        if (this->status == Connection::Read)
        {
            // todo check if client_remaining_data is not empty then read from it instead/
            // of recv (Keep-Alive)
            ssize_t recv_ret = recv(this->client_sock, this->client_headers_buffer, 4096, 0);
            std::cerr << "recv_ret: " << recv_ret << std::endl;
            // TODO: handle error
            char *buffer_ptr = this->client_headers_buffer;
            if (!this->client_started_header)
                buffer_ptr = skip_crlf(buffer_ptr, buffer_ptr + recv_ret);
            this->client_headers.append(buffer_ptr, recv_ret);

            std::string::const_iterator start = this->client_headers.end();
            start -= std::min(recv_ret + 4, (ssize_t)this->client_headers.size());
            if (end_of_headers(start, this->client_headers.end()))
            {
                this->parse_request(this->client_headers);
                if (!this->parse_path(this->req_headers["@path"]))
                {
                    //TODO: handle error
                    std::cerr << "Bad Request" << std::endl;
                }
                this->check_potential_body(this->req_headers);
                this->client_headers.clear();
                this->status = Connection::Write;
                this->multiplexing.remove(this->client_sock);
                this->multiplexing.add(this->client_sock, this, AMultiplexing::Write);
                print_headers();
            }
        }

    }

    void Connection::check_potential_body(const map_type & val)
    {
        if (val.find("Content-Length") != val.end())
            this->is_body_expected = true;
        map_type::const_iterator    tmp = val.find("Transfer-Encoding");
        if (tmp != val.end())
        {
            this->is_body_chunked = !strcasecmp("chunked", tmp->second.c_str());
            // TODO: if not chuncked -> error not implimented.
            this->is_body_expected = true;
        }

    }

    bool Connection::is_crlf(std::string::const_iterator it, std::string::const_iterator end)
    {
        if (it != end && *it == '\n')
            return true;
        if (it != end && it + 1 != end && *it == '\r' && *(it + 1) == '\n')
            return true;
        return false;
    }

    bool Connection::is_allowed_header_char(char c)
    {
        if (isalnum(c) || c == '!' || (c >= '#' && c <= '\'') ||
            c == '*' || c == '+' || c == '-' || c == '.' || c == '^' ||
            c == '_' || c == '`' || c == '|' || c == '~')
            return true;
        return false;
    }

    void Connection::check_for_absolute_uri()
    {
        std::string &uri = this->req_headers["@path"];
        if (strncasecmp(uri.c_str(), "http://", 7) != 0)
            return;
        std::string::iterator path = std::find(uri.begin() + 7, uri.end(), '/');
        uri = std::string(path, uri.end());
    }

    void Connection::parse_request(const std::string &r)
    {
        std::string::const_iterator it = find_first_not(r.begin(), r.end(), "\r\n");
        std::string::const_iterator end = r.end();

        std::string::const_iterator curr_end = find_first_of(it, end, " \t\n\r");
        // do some checks if the method is valid
        // methods should be case-sensitive and should be limited to the set of the following:
        // GET, HEAD, POST, PUT, DELETE
        // if the method is not valid, return "501 Not Implemented"
        this->req_headers["@method"] = std::string(it, curr_end);
        if (we::is_method_supported(this->req_headers["@method"]) == false)
        {
            // TODO: 501 Not Implemented
            std::cerr << "501 Not Implemented" << std::endl;
            return;
        }

        it = find_first_not(curr_end, end, " \t\r");
        curr_end = find_first_of(it, end, " \t\n\r");
        // do some checks if path is found
        // path should not be empty or too long (should not be longer than max header size)
        // if failed, return "414 URI Too Long"
        std::string &path = this->req_headers["@path"];
        path = std::string(it, curr_end);
        if (path.empty() || path.size() > 8192) // TODO: change to max uri size
        {
            // TODO: 414 URI Too Long
            std::cerr << "414 URI Too Long" << std::endl;
            return;
        }
        this->check_for_absolute_uri();

        it = find_first_not(curr_end, end, " \t\r");
        curr_end = find_first_of(it, end, " \t\r\n");
        // do checks if the protocol is supported (HTTP/1.1)
        // protocol is case-sensitive
        // if protocol is not supported, respond with a "505 HTTP Version Not Supported"
        this->req_headers["@protocol"] = std::string(it, curr_end);
        if (we::is_protocol_supported(this->req_headers["@protocol"]) == false)
        {
            // TODO: 505 HTTP Version Not Supported
            std::cerr << "505 HTTP Version Not Supported" << std::endl;
            return;
        }
        it = find_first_not(curr_end, end, " \t");
        // check if *it == '\n' ----> is_crlf(it, end)
        // if not, then return "400 Bad Request"
        // Example: (GET / HTTP/1.1   abc\n)

        // parse headers
        while (it != end)
        {
            it = find_first_not(it, end, "\r\n");
            curr_end = it;
            while (curr_end != end && is_allowed_header_char(*curr_end))
                ++curr_end;
            if (*curr_end != ':' && *curr_end != '\n')
            {
                it = find_first_of(curr_end, end, "\n");
                continue;
            }
            std::string key = std::string(it, curr_end);
            it = curr_end + (*curr_end == ':' ? 1 : 0);
            it = find_first_not(it, end, " \t");
            std::string::const_iterator last_non_ws = curr_end = it;
            while (curr_end != end && *curr_end != '\n')
            {
                if (*curr_end != ' ' && *curr_end != '\t' && *curr_end != '\r')
                    last_non_ws = curr_end + 1;
                ++curr_end;
            }
            std::string value = std::string(it, last_non_ws);
            it = curr_end;
            this->req_headers[key] = value;
        }
    }

    std::string Connection::parse_absolute_path(const std::string &str)
    {
        std::string::const_iterator it = str.begin();
        std::string::const_iterator end = str.end();

        std::vector<std::pair<std::string::const_iterator, std::string::const_iterator> > stack;

        while (it != end)
        {
            char c = *it;
            if (c == '/' && *(it + 1) == '.' && *(it + 2) == '.' && ((*(it + 3) == '/') || (it + 3 == end)))
            {
                if (stack.empty() == false)
                    stack.pop_back();
                else
                    throw std::runtime_error("invalid path");
                it += 3;
                continue;
            }
            if (c == '/' && *(it + 1) == '.' && ((*(it + 2) == '/') || (it + 2 == end)))
            {
                it += 2;
                continue;
            }
            if (c == '/' && *(it + 1) == '/')
            {
                ++it;
                continue;
            }

            std::string::const_iterator pos = std::find(it, end, '/');
            if (pos == it)
                ++it;
            else
            {
                stack.push_back(std::make_pair(it, pos));
                it = pos;
            }
        }

        if (stack.empty())
            return "/";

        std::string dir_name;
        for (size_t i = 0; i < stack.size(); ++i)
        {
            dir_name += "/";
            dir_name += std::string(stack[i].first, stack[i].second);
        }
        return dir_name;
    }

    bool Connection::parse_path(std::string const &url)
    {
        const std::string discarded_fragment = std::string(url.begin(), find_first_of(url.begin(), url.end(), "#"));
        std::string::const_iterator first_qm = find_first_of(discarded_fragment.begin(), discarded_fragment.end(), "?");
        if (first_qm != discarded_fragment.end())
            this->req_headers["@query"] = std::string(first_qm + 1, discarded_fragment.end());
        this->req_headers["@encode_url"] = std::string(discarded_fragment.begin(), first_qm);
        this->req_headers["@decode_url"] = decode_percent(this->req_headers["@encode_url"]);
        try
        {
            this->req_headers["@expanded_url"] = this->parse_absolute_path(this->req_headers["@decode_url"]);
        }
        catch (std::runtime_error &e)
        {
            return false;
        }
        return true;
    }

}
