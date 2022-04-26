#include "Connection.hpp"

namespace we
{
    Connection::Connection(int connected_socket, EventLoop &loop, const Config &config, AMultiplexing &multiplexing) : config(config), multiplexing(multiplexing), loop(loop), client_header_parser(&this->req_headers, 5000)
    {
        this->connected_socket = connected_socket;
        this->server = NULL;
        this->location = NULL;
        this->client_headers_buffer = NULL;
        this->client_body_buffer = NULL;
        this->client_started_header = false;

        this->phase = Phase_Reserved_1;
        this->redirect_count = config.max_internal_redirect;
        this->response_server = NULL;
        this->response_type = Connection::ResponseType_None;

        this->client_sock = accept(connected_socket, (struct sockaddr *)&this->client_addr, &this->client_addr_len);

        if (this->client_sock == -1)
            throw std::runtime_error("accept() failed");

        this->is_http_10 = false;
        this->is_body_chunked = false;
        this->keep_alive = true;
        this->expect = false;
        this->client_content_length = 0;

        this->client_addr_str = inet_ntoa(reinterpret_cast<struct sockaddr_in *>(&this->client_addr)->sin_addr);

        multiplexing.add(this->client_sock, this, AMultiplexing::Read);
        this->client_headers_buffer = new char[this->config.client_max_header_size];
        this->status = Connection::Read;
    }

    Connection::~Connection()
    {
        if (this->client_headers_buffer != NULL)
            delete[] this->client_headers_buffer;
        if (this->client_body_buffer != NULL)
            delete[] this->client_body_buffer;
        if (this->response_server != NULL)
            delete this->response_server;
    }

    void Connection::print_headers()
    {
        std::map<std::string, std::string, we::LessCaseInsensitive>::const_iterator it;
        for (it = this->req_headers.begin(); it != this->req_headers.end(); ++it)
            std::cout << it->first << ": " << '\'' << it->second << '\'' << std::endl;
    }
    
    void    Connection::get_info_headers()
    {
        if (strcasecmp(this->req_headers["@protocol"].c_str(), "HTTP/1.0") == 0)
        {
            this->is_http_10 = true;
            this->to_chunk = false;
            this->keep_alive = false;
        }

        if (this->is_http_10 == false && strcasecmp(this->req_headers["Connection"].c_str(), "close") == 0)
            this->keep_alive = 0;
        if (this->is_http_10 == false && strcasecmp(this->req_headers["Expect"].c_str(), "100-continue") == 0)
            this->expect = 1;
        if (this->is_http_10 == false && strcasecmp(this->req_headers["Transfer-Encoding"].c_str(), "chunked") == 0)
            this->is_body_chunked = 1;

        if (this->req_headers.find("Content-Length") != this->req_headers.end())
        {
            this->client_content_length = atoll(this->req_headers["Content-Length"].c_str());
            if (this->client_content_length < 0)
                throw we::HTTPStatusException(400, "");
            if (this->client_content_length > this->location->client_max_body_size)
                throw we::HTTPStatusException(413, "");
        }
        
    }

    void Connection::handle_connection()
    {
        if (this->status == Connection::Read)
        {
            ssize_t recv_ret;
            if (this->client_remaining_data.size())
            {
                recv_ret = this->client_remaining_data.size();
                std::memcpy(this->client_headers_buffer, this->client_remaining_data.c_str(), recv_ret);
                this->client_remaining_data.clear();
            }
            else
                recv_ret = recv(this->client_sock, this->client_headers_buffer, this->config.client_max_header_size, 0);

            if (recv_ret <= 0)
                goto close_connection;

            char *str = this->client_headers_buffer;
            bool end;
            end = client_header_parser.append(str, str + recv_ret);

            if (end)
            {
                this->expanded_url = this->req_headers["@expanded_url"]; // Extract the expanded url
                this->client_remaining_data = std::string(str, this->client_headers_buffer + recv_ret);

                // this->check_potential_body();
                this->status = Connection::Write;

                this->multiplexing.remove(this->client_sock);
                this->multiplexing.add(this->client_sock, this, AMultiplexing::Write);
                this->server = this->config.get_server_block(this->connected_socket, this->req_headers["Host"]);
                this->location = this->server->get_location(this->expanded_url);
                this->requested_resource = we::get_file_fullpath(this->location->root, this->expanded_url);

                this->get_info_headers();
            }
        }
        else if (this->status == Connection::Write)
        {
            if (this->phase <= Phase_Reserved_4)
                process_handlers();
            if (!this->response_server)
            {
                if (this->keep_alive)
                    this->res_headers.insert(std::make_pair("Connection", "keep-alive"));
                else
                    this->res_headers.insert(std::make_pair("Connection", "close"));
                try
                {
                    if (this->response_type == Connection::ResponseType_File)
                        this->response_server = new we::ResponseServerFile(this);
                    else if (this->response_type == Connection::ResponseType_Directory)
                        this->response_server = new ResponseServerDirectory(this);
                    else if (this->response_type == ResponseType_RangeFile)
                    {
                        if (this->ranges.size() == 1)
                            this->response_server = new ResponseServerFileSingleRange(this);
                        else
                            this->response_server = new ResponseServerFileMultiRange(this);
                    }
                }
                catch (const std::exception &e)
                {
                    std::cerr << e.what() << std::endl;
                    goto close_connection;
                }
            }

            std::string buffer;
            bool ended = false;
            ssize_t &sended_bytes = this->response_server->get_next_data(buffer, ended);
            if (ended)
                goto finish_connection;
            ssize_t ret = send(this->client_sock, buffer.c_str(), buffer.size(), 0);
            if (ret <= 0)
                goto close_connection;
            sended_bytes = ret;
        }

        return;
    finish_connection:
        this->multiplexing.remove(this->client_sock);
        if (this->keep_alive == false)
        {
        close_connection:
            close(this->client_sock);
            delete this;
            return;
        }
        else
        {
            this->status = Connection::Read;
            this->multiplexing.add(this->client_sock, this, AMultiplexing::Read);
            this->req_headers.clear();
            this->res_headers.clear();
            delete this->response_server;
            this->response_server = NULL;
            this->client_started_header = false;
            this->client_header_parser.reset();
            this->phase = Phase_Reserved_1;
            this->response_type = Connection::ResponseType_None;
        }
        return;
    }

    // void Connection::check_potential_body()
    // {
    //     if (this->req_headers.find("Content-Length") != this->req_headers.end())
    //         this->is_body_expected = true;

    //     map_type::const_iterator tmp = this->req_headers.find("Transfer-Encoding");
    //     if (tmp != this->req_headers.end())
    //     {
    //         this->is_body_chunked = !strcasecmp("chunked", tmp->second.c_str());
    //         if (this->is_body_chunked == false)
    //             throw we::HTTPStatusException(501, "Not Implemented");
    //         this->is_body_expected = true;
    //     }
    // }

    void Connection::process_handlers()
    {
        while (this->phase <= Phase_Reserved_4)
        {
            for (size_t i = 0; i < this->location->handlers[this->phase].size(); i++)
            {
                if (this->location->handlers[this->phase][i](this))
                    break;
            }
            this->phase = Phase(int(this->phase) + 1);
        }
    }

}
