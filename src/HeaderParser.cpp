#include "HeaderParser.hpp"
namespace we
{

    void HeaderParser::reset()
    {
        req_size = 0;
        m_state = State_Start;
        end = false;
        buffer.clear();
    }

    void HeaderParser::switch_config(Headers_t *headers, long long max_header_size, State state)
    {
        this->reset();
        this->headers = headers;
        this->max_header_size = max_header_size;
        this->m_state = state;
    }

    static bool compare_case_insensitive(char a, char b)
    {
        return std::tolower(a) == std::tolower(b);
    }

    void HeaderParser::handle_request_line()
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
        this->headers->operator[]("@protocol") = std::string(it - 8, it);
        while ((*it == ' ' || *it == '\t'))
            ++it;
        if (!IS_EOL(it))
            throw we::HTTPStatusException(400, "");
        this->buffer.clear();
        this->headers->operator[]("@method") = method;
        this->headers->operator[]("@path") = uri;
        this->parse_path(uri);
    }

    void HeaderParser::handle_header()
    {
        std::string::iterator it = this->buffer.begin(),
                              start_value, end_last_non_ws;
        std::string header_name;

        while (!IS_EOL(it) && *it != ':')
        {
            if (!isalnum(*it) && *it != '-')
                goto end_header;
            ++it;
        }
        header_name = std::string(this->buffer.begin(), it);
        if (IS_EOL(it))
        {
            this->headers->operator[](header_name) = "";
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

        this->headers->operator[](header_name) = std::string(start_value, end_last_non_ws);
    end_header:
        if (strcasecmp(header_name.c_str(), "host") == 0 && this->headers->operator[](header_name).empty())
            throw we::HTTPStatusException(400, "");
        this->buffer.clear();
    }

    HeaderParser::HeaderParser(Headers_t *_h, long long _m, State _s) : headers(_h), max_header_size(_m), m_state(_s)
    {
        req_size = 0;
        end = false;
    }

    std::string HeaderParser::dedote_path(const std::string &str)
    {
        std::string::const_iterator it = str.begin();
        std::string::const_iterator end = str.end();

        std::vector<std::pair<std::string::const_iterator, std::string::const_iterator> > stack;

        while (it != end)
        {
            char c = *it;
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

    void    HeaderParser::parse_path(std::string const &url)
    {
        Headers_t &_headers = *this->headers;
        std::string::const_iterator start = url.begin(),
                                    end = find(url.begin(), url.end(), '#'),
                                    first_qm = find(start, end, '?');
        if (first_qm != end)
        {
            _headers["@query"] = std::string(first_qm + 1, end);
            end = first_qm;
        }
        _headers["@encode_url"] = std::string(start, end);
        _headers["@decode_url"] = decode_percent(_headers["@encode_url"]);
        _headers["@expanded_url"] = this->dedote_path(_headers["@decode_url"]);
    }

}

// #include <fstream>
// #include <sstream>
// int main()
// {
//     std::fstream f("request");
//     std::stringstream ss;
//     ss << f.rdbuf();
//     std::string s = ss.str();
//     we::HeaderParser::Headers_t headers;
//     we::HeaderParser parser(&headers, 1500);
//     try
//     {
//         while (1)
//         {
//             std::string::iterator it = s.begin();
//             if (parser.append(it, s.end()))
//                 break;
//         }
//         // print all headers
//         we::HeaderParser::Headers_t::iterator it = headers.begin();
//         while (it != headers.end())
//         {
//             std::cout << "'" << it->first << "' : '" << it->second << "'" << std::endl;
//             ++it;
//         }
//     } catch (we::HTTPStatusException &e)
//     {
//         std::cout << e.statusCode << " " << e.what() <<std::endl;

//     }
//     catch (std::exception &e)
//     {
//         std::cout << "Exception: " << e.what() << std::endl;
//     }
// }








// GET               HTTP://ddds/index.html/../%26file///././//../         HTTP/1.1         
// Host: facebook.com
// Sec-Ch-Ua: "(Not(A:Brand";v="8", "Chromium";v="99"
// Sec-Ch-Ua-Mobile: ?0
// Sec-Ch-Ua-Platform: "macOS"
// Upgrade-Insecure-Requests: 1
// User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/99.0.4844.74 Safari/537.36
// Accept:     text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.9       
// Sec-Fetch-Site
// Sec-Fetch-Mode: navigate
// Sec-Fetch-User: ?1
// Sec-Fetch-Site:    loooo    l    
// Sec-Fetch-Dest:                       
// Accept-Encoding
// Accept-Language: en-US,en;q=0.9
// Connection: close

// dklsjakljdsakljdsa
// dkjsahkjhdkjhdkjash
// dashjkhdsajkhjkdhsakjhdkjsahjkdsahkjhdsajkh