#include "Connection.hpp"

namespace we
{
    void    set_answer_timeout(EventData ev)
    {
        Connection *c = static_cast<Connection *>(ev.pointer);
        throw std::runtime_error("Timeout: not implemented");
    }

    Connection::Connection(int connected_socket, EventLoop& loop, const Config& config, AMultiplexing& multiplexing)
        : connected_socket(connected_socket),
          config(config),
          multiplexing(multiplexing),
          loop(loop),
          client_timeout_event(NULL),
          status(Idle),
          server(NULL),
          location(NULL),
          client_headers_buffer(NULL),
          client_body_buffer(NULL),
          requested_file(NULL),
          body_file(NULL)
    {
        this->client_addr_len = sizeof(this->client_addr);
        this->client_sock = accept(connected_socket, (struct sockaddr*)&this->client_addr, &this->client_addr_len);
        // TODO: fcntl(this->client_sock, F_SETFL, O_NONBLOCK);
        if (this->client_sock == -1) {
            perror("accept");
            throw std::runtime_error("accept");
        }
        multiplexing.add(this->client_sock, this, AMultiplexing::Read);
        EventData ev;
        ev.pointer = this;
        this->client_timeout_event = &this->loop.add_event(set_answer_timeout, ev, this->config.client_header_timeout, false);
        this->status = Read;
        this->client_headers_buffer = new char[this->config.client_header_buffer_size];
    }

    char*   Connection::is_finished_header()
    {
        char *start = this->client_headers_buffer;
        bool last_is_lf = false;
        bool last_is_cr = false;
        while (*start)
        {
            if (*start == '\n')
            {
                if (last_is_lf)
                    break ;
                last_is_lf = true;
                last_is_cr = false;
            }
            else if (*start != '\r' || last_is_cr)
            {
                last_is_lf = false;
                last_is_cr = '\r' == *start;
            }
            else if (*start == '\r')
                last_is_cr = true;
            start++;
        }
       
        return *start == '\n' ? start + 1 : NULL;
    }

    bool Connection::handle_client()
    {
        ssize_t last_read = recv(this->client_sock, this->client_headers_buffer, this->config.client_header_buffer_size, 0);
        // TODO: handle last_read <= 0
        this->client_headers_buffer[last_read] = '\0';
        char *end = this->is_finished_header();
        if (end)
        {
            this->client_headers.insert(this->client_headers.end(), this->client_headers_buffer, end);
            // TODO: check if client_headers.size() > config.client_header_max_size
            strcpy(this->client_headers_buffer, end);
        }
        else
        {
            this->client_headers += this->client_headers_buffer;
            // TODO: check if client_headers.size() > config.client_header_max_size
        }
    }
}