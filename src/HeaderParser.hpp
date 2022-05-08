#pragma once
#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <algorithm>
#include "Utils.hpp"
#define IS_EOL(data) ((data[0] == '\r') && (data[1] == '\n') || (data[0] == '\n'))
namespace we
{

    class HeaderParser
    {
    public:
        enum State
        {
            State_Start,
            State_Request_Line,
            State_Header,
        };

        typedef std::map<std::string, std::string, LessCaseInsensitive> Headers_t;
    private:

        State m_state;
        long long max_header_size;
        Headers_t *headers;
        bool end;
        std::string buffer;
        size_t req_size;

        HeaderParser();

        std::string dedote_path(const std::string &str);

    public:
        HeaderParser(Headers_t *, long long, State = State_Start);

        template <typename _Iterator>
        bool append(_Iterator &data, _Iterator end)
        {
            _Iterator start = data;
            _Iterator new_line;
            while ((new_line = std::find(data, end, '\n')) != end)
            {
                if (m_state != State_Start && IS_EOL(data) && this->buffer.empty())
                {
                    data = new_line + 1;
                    this->end = true;
                    goto check_size_and_return;
                }

                if (m_state == State_Start && IS_EOL(data))
                {
                    data = new_line + 1;
                    start = data;
                    continue;
                }
                else if (m_state == State_Start)
                    m_state = State_Request_Line;
                this->buffer.append(data, new_line + 1);
                data = new_line + 1;
                if (m_state == State_Request_Line)
                {
                    this->req_size += data - start;
                    start = data;
                    if (this->max_header_size != -1 && this->req_size > this->max_header_size)
                        throw we::HTTPStatusException(414, "");
                    this->handle_request_line();

                    m_state = State_Header;
                }
                else if (m_state == State_Header)
                {
                    this->req_size += data - start;
                    start = data;
                    if (this->max_header_size != -1 && this->req_size > this->max_header_size)
                        throw we::HTTPStatusException(400, "Request Header Or Cookie Too Large");
                    this->handle_header();
                }
            }
            if (data != end && m_state == State_Start)
                m_state = State_Request_Line;
            this->buffer.append(data, end);
            data = end;
        check_size_and_return:
            if (m_state != State_Start)
                this->req_size += data - start;
            if (this->max_header_size != -1 && this->req_size > this->max_header_size)
            {
                if (this->m_state == State_Request_Line)
                    throw we::HTTPStatusException(414, "");
                else
                    throw we::HTTPStatusException(400, "Request Header Or Cookie Too Large");
            }
            return this->end;
        }

        void handle_request_line();
        void handle_header();

        void reset();
        void switch_config(Headers_t *, long long, State = State_Start);

        void parse_path(std::string const &url);
    };
}