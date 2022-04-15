#include "Utils.hpp"

namespace we
{
    bool LessCaseInsensitive::operator()(const std::string &lhs, const std::string &rhs) const
    {
        return (lhs.size() < rhs.size() || (lhs.size() == rhs.size() &&
                                            strcasecmp(lhs.c_str(), rhs.c_str()) < 0));
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
        int i = 0;
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

        for (int i = 0; i < sizeof(supported_protocols) / sizeof(supported_protocols[0]); ++i)
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

        for (int i = 0; i < sizeof(supported_methods) / sizeof(supported_methods[0]); ++i)
        {
            if (method == supported_methods[i])
                return true;
        }

        return false;
    }

}