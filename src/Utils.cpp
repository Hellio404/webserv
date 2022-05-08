#include "Utils.hpp"

namespace we
{
    bool LessCaseInsensitive::operator()(const std::string &lhs, const std::string &rhs) const
    {
        return (lhs.size() < rhs.size() || (lhs.size() == rhs.size() &&
                                            strcasecmp(lhs.c_str(), rhs.c_str()) < 0));
    }
    static clock_t start[256];
    static float ids_time[256] = {0};
    static const char* id_names[256] = {0};
    static int counter[256] = {0};

    void                        start_recording(unsigned char id, const char *name)
    {
        start[id] = clock();
        if (id_names[id] == 0)
            id_names[id] = name;
        counter[id]++;
    }

    void                        stop_recording(unsigned char id)
    {
        ids_time[id] += (clock() - start[id]) / (float)(CLOCKS_PER_SEC/1000);
    }

    // TODO: clear debugging functions
    void                        print_ids_time()
    {
        for (int i = 0; i < 256; i++)
        {
            if (id_names[i] != 0)
                printf("%s: %f ==> %d\n", id_names[i], ids_time[i], counter[i]);
        }
    }

    std::string::const_iterator find_first_not(std::string::const_iterator it, std::string::const_iterator end, const std::string &s)
    {
        while (it != end)
        {
            if (s.find(*it) == std::string::npos)
                break;
            it += 1;
        }
        return it;
    }

    std::string::const_iterator find_first_of(std::string::const_iterator it, std::string::const_iterator end, const std::string &s)
    {
        while (it != end)
        {
            if (s.find(*it) != std::string::npos)
                break;
            it += 1;
        }
        return it;
    }

    std::string to_hex(size_t number)
    {
        std::stringstream stream;
        stream << std::hex << number;
        return stream.str();
    }

    std::string to_env_uppercase(const std::string &s)
    {
        std::string res(s);
        for (size_t i = 0; i < res.size(); i++)
        {
            if (res[i] == '-')
                res[i] = '_';
            else
                res[i] = toupper(res[i]);
        }
        return res;
    }

    int char_to_hex(char c)
    {
        if (c <= '9')
            return c - '0';
        if (c <= 'F')
            return c - 'A' + 10;
        if (c <= 'f')
            return c - 'a' + 10;
        return -1;
    }

    std::string decode_percent(const std::string &str)
    {
        size_t i = 0;
        std::string ret;
        while (i < str.length())
        {
            if (str[i] == '%' && isxdigit(str[i + 1]) && isxdigit(str[i + 2]))
            {
                ret += static_cast<char>(char_to_hex(str[i + 1]) * 16 + char_to_hex(str[i + 2]));
                i += 2;
            }
            else
                ret += str[i];
            i++;
        }
        return ret;
    }

    bool is_protocol_supported(const std::string &protocol)
    {
        const char *supported_protocols[] = {
            "HTTP/1.1"};

        for (size_t i = 0; i < sizeof(supported_protocols) / sizeof(supported_protocols[0]); ++i)
        {
            if (protocol == supported_protocols[i])
                return true;
        }

        return false;
    }

    bool is_method_supported(const std::string &method)
    {
        const char *supported_methods[] = {
            "GET", "POST", "HEAD", "PUT", "DELETE"};

        for (size_t i = 0; i < sizeof(supported_methods) / sizeof(supported_methods[0]); ++i)
        {
            if (method == supported_methods[i])
                return true;
        }

        return false;
    }

    std::string get_status_string(unsigned int status_code)
    {
        switch (status_code)
        {
        // 1xx Informational
        // Success (2xx)
        case 200:
            return "OK";
        case 201:
            return "Created";
        case 202:
            return "Accepted";
        case 203:
            return "Non-Authoritative Information";
        case 204:
            return "No Content";
        case 205:
            return "Reset Content";
        case 206:
            return "Partial Content";
        // Redirections (3xx)
        case 300:
            return "Multiple Choices";
        case 301:
            return "Moved Permanently";
        case 302:
            return "Found";
        case 303:
            return "See Other";
        case 304:
            return "Not Modified";
        case 305:
            return "Use Proxy";
        case 306:
            return "Switch Proxy";
        case 307:
            return "Temporary Redirect";
        case 308:
            return "Permanent Redirect";
        // Client Errors (4xx)
        case 400:
            return "Bad Request";
        case 401:
            return "Unauthorized";
        case 402:
            return "Payment Required";
        case 403:
            return "Forbidden";
        case 404:
            return "Not Found";
        case 405:
            return "Method Not Allowed";
        case 406:
            return "Not Acceptable";
        case 407:
            return "Proxy Authentication Required";
        case 408:
            return "Request Timeout";
        case 409:
            return "Conflict";
        case 410:
            return "Gone";
        case 411:
            return "Length Required";
        case 412:
            return "Precondition Failed";
        case 413:
            return "Request Entity Too Large";
        case 414:
            return "Request-URI Too Long";
        case 415:
            return "Unsupported Media Type";
        case 416:
            return "Requested Range Not Satisfiable";
        case 417:
            return "Expectation Failed";
        // Server Errors (5xx)
        case 500:
            return "Internal Server Error";
        case 501:
            return "Not Implemented";
        case 502:
            return "Bad Gateway";
        case 503:
            return "Service Unavailable";
        case 504:
            return "Gateway Timeout";
        case 505:
            return "HTTP Version Not Supported";
        default:
            throw std::runtime_error("Unknown status code " + we::to_string(status_code));
        }
        return "";
    }

    std::string get_default_page_code(unsigned int status_code)
    {
        const char *status_str = "<html>\n"
                                "<head><title>%1$d %2$s</title></head>\n"
                                "<body>\n"
                                "<center><h1>%1$d %2$s</h1></center>\n"
                                "<hr><center>BetterNginx/0.69.420 (HEAVEN)</center>\n"
                                "</body>\n"
                                "</html>\n";

        char buffer[2048];
        sprintf(buffer, status_str, status_code, get_status_string(status_code).c_str());
        return std::string(buffer);
    }

    std::string capitalize_header(const std::string  & original)
    {
        if (original.size() == 0)
            return original;
        std::string cap = "";
        cap.reserve(original.size());
        for (size_t i = 0; i < original.size(); i++)
        {
            if (i == 0 || (isalpha(original[i]) && !isalnum(original[i - 1])))
                cap += toupper(original[i]);
            else
                cap += original[i];
        }
        return cap;
    }

    size_t get_next_boundry()
    {
        size_t  static  current = 0;

        return ++current;
    }

    std::string make_range_header(size_t boundry, const std::string *Content_Type, std::pair<unsigned long long, unsigned long long> range, unsigned long long size)
    {
        std::string range_header = "\r\n--" + to_string(boundry, 20, '0') + "\r\n";

        if (Content_Type != NULL)
        {
            range_header += "Content-Type: ";
            range_header += (*Content_Type) + "\r\n";
        }

        range_header += "Content-Range: bytes ";
        range_header += to_string(range.first) + "-" + to_string(range.second) + "/" + to_string(size);
        range_header += "\r\n";
        range_header += "\r\n";

        return range_header;
    }

    size_t                      number_len(size_t s)
    {
        size_t len = 1;

        while (s > 10)
        {
            s /= 10;
            ++len;
        }
        return len;
    }

    bool                        is_bodiless_response(unsigned int status_code)
    {
        switch (status_code)
        {
        case 100: // Continue
        case 101: // Switching Protocols
        case 204: // No Content
        case 205: // Reset Content
        case 304: // Not Modified
            return true;
        default:
            return false;
        }
    }

    bool compare_case_insensitive(char a, char b)
    {
        return std::tolower(a) == std::tolower(b);
    }


    void insert_or_assign(std::map<std::string, std::string, LessCaseInsensitive>  &map, std::string const &key, std::string const &value)
    {
        map[key] = value;
    }

    void insert_or_assign(std::multimap<std::string, std::string, LessCaseInsensitive>  &map, std::string const &key, std::string const &value)
    {
        map.insert(std::make_pair(key, value));
    }


}
