#include "Connection.hpp"

namespace we
{

    BaseConnection::~BaseConnection() {}

    static void    timeout_event(EventData data)
    {
        Connection *conn = reinterpret_cast<Connection *>(data.pointer);
        conn->timeout();
        conn->client_timeout_event = NULL;
    }

    static void    body_timeout_event(EventData data)
    {

        Connection *conn = reinterpret_cast<Connection *>(data.pointer);
        conn->keep_alive = false;
        conn->response_type = Connection::ResponseType_File;
        conn->res_headers.insert(std::make_pair("@response_code", "408"));
        conn->requested_resource = conn->location->get_error_page(408);
        conn->status = Connection::Write;
        conn->multiplexing.remove(conn->client_sock);
        conn->multiplexing.add(conn->client_sock, conn, AMultiplexing::Write);
        conn->phase = Phase_End;
        response_server_handler(conn);
        conn->client_timeout_event = &conn->loop.add_event(timeout_event, conn->event_data, conn->server->server_send_timeout);
    }

    Connection::Connection(int connected_socket, EventLoop &loop, const Config &config, AMultiplexing &multiplexing) : config(config), multiplexing(multiplexing), loop(loop), client_header_parser(&this->req_headers, 5000)
    {
        this->client_sock = accept(connected_socket, (struct sockaddr *)&this->client_addr, &this->client_addr_len);
        if (this->client_sock == -1)
            throw std::runtime_error("accept() failed"); // TODO: try catch when creating a connection
        this->client_addr_str = inet_ntoa(reinterpret_cast<struct sockaddr_in *>(&this->client_addr)->sin_addr);
        this->event_data.pointer = this;
        this->connected_socket = connected_socket;

        
        this->client_headers_buffer = NULL;
        this->client_body_buffer = NULL;
        this->response_server = NULL;
        this->body_handler = NULL;
        this->client_timeout_event = NULL;
        this->reset();

        this->client_headers_buffer = new char[this->config.client_max_header_size];
    }

    Connection::~Connection()
    {
        if (this->client_timeout_event)
            this->loop.remove_event(*this->client_timeout_event);
        this->multiplexing.remove(this->client_sock);
        close(this->client_sock);
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

        if (this->is_http_10 == false && this->req_headers.find("Connection") != this->req_headers.end() && strcasecmp(this->req_headers["Connection"].c_str(), "close") == 0)
            this->keep_alive = 0;
        if (this->is_http_10 == false && this->req_headers.find("Transfer-Encoding") != this->req_headers.end() && strcasecmp(this->req_headers["Transfer-Encoding"].c_str(), "chunked") == 0)
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

            try
            {
                char *str = this->client_headers_buffer;
                bool end;
                end = client_header_parser.append(str, str + recv_ret);

                if (end)
                {
                    this->expanded_url = this->req_headers["@expanded_url"]; // Extract the expanded url
                    this->client_remaining_data = std::string(str, this->client_headers_buffer + recv_ret);

                    this->server = this->config.get_server_block(this->connected_socket, this->req_headers["Host"]);
                    this->location = this->server->get_location(this->expanded_url);
                    this->requested_resource = we::get_file_fullpath(this->location->root, this->expanded_url);

                    this->get_info_headers();
                    if (this->is_body_chunked || this->client_content_length > 0)
                    {
                        this->status = Connection::ReadBody;
                        if (this->client_timeout_event)
                            this->loop.remove_event(*this->client_timeout_event);
                        this->client_timeout_event = &this->loop.add_event(body_timeout_event, this->event_data, this->location->client_body_timeout);
                        this->body_handler = new BodyHandler(this); 
                    }
                    else
                    {
                        this->status = Connection::Write;
                        this->multiplexing.remove(this->client_sock);
                        this->multiplexing.add(this->client_sock, this, AMultiplexing::Write);
                        if (this->client_timeout_event)
                            this->loop.remove_event(*this->client_timeout_event);
                        this->client_timeout_event = &this->loop.add_event(timeout_event, this->event_data, this->server->server_send_timeout);
                    }
                }
            }
            catch (std::exception &e)
            {
                this->keep_alive = false;
                this->server = this->config.get_server_block(this->connected_socket, this->req_headers["Host"]);
                if (this->req_headers.count("@expanded_url") == 1)
                    this->location = this->server->get_location(this->req_headers["@expanded_url"]);
                else
                    this->location = this->server->get_location("/");
                this->response_type = Connection::ResponseType_File;
                if (dynamic_cast<we::HTTPStatusException *>(&e) != NULL)
                    this->res_headers.insert(std::make_pair("@response_code", we::to_string(dynamic_cast<we::HTTPStatusException *>(&e)->statusCode)));
                else
                    this->res_headers.insert(std::make_pair("@response_code", "500"));
                this->requested_resource = this->location->get_error_page(500);
                this->status = Connection::Write; 
                this->multiplexing.remove(this->client_sock);
                this->multiplexing.add(this->client_sock, this, AMultiplexing::Write);
                this->phase = Phase_End;
                response_server_handler(this);
                if (this->client_timeout_event)
                    this->loop.remove_event(*this->client_timeout_event);
                this->client_timeout_event = &this->loop.add_event(timeout_event, this->event_data, this->server->server_send_timeout);

            }
            catch (...)
            {
                goto close_connection;
            }
        }
        else if (this->status == Connection::ReadBody)
        {
            try
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

                if (this->body_handler->add_data(this->client_headers_buffer, this->client_headers_buffer + recv_ret))
                {
                    this->status = Connection::Write;
                    this->multiplexing.remove(this->client_sock);
                    this->multiplexing.add(this->client_sock, this, AMultiplexing::Write);
                    if (this->client_timeout_event)
                        this->loop.remove_event(*this->client_timeout_event);
                    this->client_timeout_event = &this->loop.add_event(timeout_event, this->event_data, this->server->server_send_timeout);
                }
                else
                {
                    if (this->client_timeout_event)
                        this->loop.remove_event(*this->client_timeout_event);
                    this->client_timeout_event = &this->loop.add_event(body_timeout_event, this->event_data, this->location->client_body_timeout);
                }
            }
            catch(const we::HTTPStatusException& e)
            {
                this->response_type = Connection::ResponseType_File;
                this->res_headers.insert(std::make_pair("@response_code", we::to_string(e.statusCode)));
                this->requested_resource = this->location->get_error_page(e.statusCode);

                this->status = Connection::Write;
                this->multiplexing.remove(this->client_sock);
                this->multiplexing.add(this->client_sock, this, AMultiplexing::Write);

                this->phase = Phase_End;
                response_server_handler(this);
                if (this->client_timeout_event)
                    this->loop.remove_event(*this->client_timeout_event);
                this->client_timeout_event = &this->loop.add_event(timeout_event, this->event_data, this->server->server_send_timeout);
            }
        }
        else if (this->status == Connection::Write)
        {
            try
            {
                if (this->phase < Phase_End)
                    process_handlers();
            } catch (...)
            {
                goto close_connection;
            }
            if (!this->response_server)
                goto close_connection;


            std::string buffer;
            bool ended = false;
            ssize_t &sended_bytes = this->response_server->get_next_data(buffer, ended);
            if (ended)
                goto finish_connection;

            if (this->client_timeout_event)
                this->loop.remove_event(*this->client_timeout_event);
            this->client_timeout_event = &this->loop.add_event(timeout_event, this->event_data, this->server->server_send_timeout);

            if (buffer.size() == 0)
                return;

            ssize_t ret = send(this->client_sock, buffer.c_str(), buffer.size(), 0);
            std::cerr << "Sended " << ret << " bytes" << std::endl;
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
        this->reset();
        return;
    }

    void    Connection::reset()
    {
       
        this->ranges.clear();
        this->client_started_header = false;
        this->is_http_10 = false;
        this->status = Connection::Read;
        this->server = NULL;
        this->location = NULL;

        this->client_content_length = 0;
        this->keep_alive = true;
        this->is_body_chunked = false;
        this->to_chunk = true;
        
        this->metadata_set = false;
        this->content_length = 0;
        this->mime_type.clear();
        this->etag.clear();

        if (this->client_headers_buffer)
            this->client_headers_buffer[0] = 0;
        if (this->client_body_buffer)
            delete [] this->client_body_buffer;
        this->client_body_buffer = NULL;
        this->client_header_parser.reset();
        if (this->response_server)
            delete this->response_server;
        this->response_server = NULL;
        this->response_type = Connection::ResponseType_None;
        this->phase = Phase_Pre_Start;

        this->redirect_count = this->config.max_internal_redirect;

        this->expanded_url.clear();
        this->requested_resource.clear();

        this->req_headers.clear();
        this->res_headers.clear();

        this->multiplexing.remove(this->client_sock);
        this->multiplexing.add(this->client_sock, this, AMultiplexing::Read);
        

        if (this->client_timeout_event)
            this->loop.remove_event(*this->client_timeout_event);
        this->client_timeout_event = &this->loop.add_event(timeout_event, this->event_data, this->config.client_header_timeout);

        if (body_handler)
        {
            delete body_handler;
            body_handler = NULL;
        }
    }


    void Connection::process_handlers()
    {
        while (this->phase < Phase_End)
        {
            for (size_t i = 0; i < this->location->handlers[this->phase].size(); i++)
            {
                if (this->location->handlers[this->phase][i](this))
                    break;
            }
            this->phase = Phase(int(this->phase) + 1);
        }
    }

    void  Connection::timeout()
    {
        this->multiplexing.remove(this->client_sock);
        close(this->client_sock);
        delete this;
        return;
    }
}


