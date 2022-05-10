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
    template <typename HeaderType>
    class HeaderParser
    {
    public:
        enum State
        {
            State_Start,
            State_Request_Line,
            State_Header,
        };

        typedef HeaderType Headers_t;

    private:
        State m_state;
        long long max_header_size;
        Headers_t *headers;
        bool end;
        std::string buffer;
        long long req_size;
        bool skip_checks;

        HeaderParser();

    public:

        template <typename _Iterator>
        bool append(_Iterator &data, _Iterator end)
        {
            _Iterator start = data;
            _Iterator new_line;

            while ((new_line = std::find(data, end, '\n')) != end)
            {
                if (m_state != State_Start && IS_EOL(data) && (this->buffer.empty() || this->buffer == "\r"))
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
                    if (this->max_header_size != -1 && this->req_size > this->max_header_size && !this->skip_checks)
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

        void reset()
        {
            req_size = 0;
            m_state = State_Start;
            end = false;
            buffer.clear();
        }

        void switch_config(Headers_t *headers, long long max_header_size, State state)
        {
            this->reset();
            this->headers = headers;
            this->max_header_size = max_header_size;
            this->m_state = state;
        }

        void handle_request_line()
        {
            std::string method, uri;
            method.reserve(8);
            uri.reserve(256);

            std::string::const_iterator it = this->buffer.begin();
            while (it != this->buffer.end() && *it != ' ' && *it != '\t')
            {
                if (!isupper(*it))
                    throw we::HTTPStatusException(400, "");
                method.push_back(*it++);
            }
            if (!we::is_method_supported(method))
                throw we::HTTPStatusException(501, "");
            while ((*it == ' ' || *it == '\t'))
                ++it;
            const char *http_full_url = "http://";

            if (std::equal(http_full_url, http_full_url + 7, it, compare_case_insensitive))
            {
                it += 7;
                while (it != this->buffer.end() && *it != '/')
                    ++it;
            }

            if (*it != '/')
                throw we::HTTPStatusException(400, "");

            while (it != this->buffer.end() && *it != ' ' && *it != '\t' && *it != '\r' && *it != '\n')
                uri.push_back(*it++);
            while ((*it == ' ' || *it == '\t'))
                ++it;
            const char *protocol = "HTTP/";
            if (!std::equal(protocol, protocol + 5, it))
                throw we::HTTPStatusException(400, "");
            it += 5;
            if (it[0] != '1' || it[1] != '.' || !(it[2] == '1' || it[2] == '0'))
                throw we::HTTPStatusException(505, "");
            it += 3;
            insert_or_assign(*this->headers, "@protocol", std::string(it - 8, it));
            while ((*it == ' ' || *it == '\t'))
                ++it;
            if (!IS_EOL(it))
                throw we::HTTPStatusException(400, "");
            this->buffer.clear();
            insert_or_assign(*this->headers, "@method", method);
            insert_or_assign(*this->headers, "@path", uri);
            this->parse_path(uri);
        }

        void handle_header()
        {
            std::string::iterator it = this->buffer.begin(),
                                  start_value, end_last_non_ws;
            std::string header_name;

            while (!IS_EOL(it) && *it != ':')
            {
                if (!isalnum(*it) && *it != '-' && !skip_checks)
                    goto end_header;
                ++it;
            }
            header_name = std::string(this->buffer.begin(), it);
            if (IS_EOL(it))
            {
                insert_or_assign(*this->headers, header_name, "");
                goto end_header;
            }
            ++it;
            while ((*it == ' ' || *it == '\t'))
                ++it;
            start_value = it;
            end_last_non_ws = it;
            while (!IS_EOL(it))
            {
                if (*it != ' ' && *it != '\t')
                    end_last_non_ws = it + 1;
                ++it;
            }

            insert_or_assign(*this->headers, header_name, std::string(start_value, end_last_non_ws));
        end_header:
            if (!this->skip_checks && strcasecmp(header_name.c_str(), "host") == 0 && this->headers->find(header_name)->second.empty())
                throw we::HTTPStatusException(400, "");
            this->buffer.clear();
        }

        HeaderParser(Headers_t *_h, long long _m, State _s = State_Start, bool skip_checks = false) : m_state(_s), max_header_size(_m), headers(_h)
        {
            this->skip_checks = skip_checks;
            req_size = 0;
            end = false;
        }

        void parse_path(std::string const &url)
        {
            Headers_t &_headers = *this->headers;
            std::string::const_iterator start = url.begin(),
                                        end = find(url.begin(), url.end(), '#'),
                                        first_qm = find(start, end, '?');
            if (first_qm != end)
            {
                insert_or_assign(_headers, "@query", std::string(first_qm + 1, end));
                end = first_qm;
            }
            insert_or_assign(_headers, "@encode_url", std::string(start, end));
            insert_or_assign(_headers, "@decode_url", decode_percent(_headers.find("@encode_url")->second));
            insert_or_assign(_headers, "@expanded_url", this->dedote_path(_headers.find("@decode_url")->second));
        }

    private:
        std::string dedote_path(const std::string &str)
        {
            std::string::const_iterator it = str.begin();
            std::string::const_iterator end = str.end();

            std::vector<std::pair<std::string::const_iterator, std::string::const_iterator> > stack;

            while (it != end)
            {
                if (std::equal(it, it + 3, "/..") && (it[3] == '/' || it + 3 == end))
                {
                    if (stack.empty() == false)
                        stack.pop_back();
                    else
                        throw we::HTTPStatusException(400, "");
                    it += 3;
                    continue;
                }
                if (std::equal(it, it + 2, "/.") && (it[2] == '/' || it + 2 == end))
                {
                    it += 2;
                    continue;
                }
                if (std::equal(it, it + 2, "//"))
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
            if (str[str.size() - 1] == '/')
                dir_name += "/";
            return dir_name;
        }
    };

    

}