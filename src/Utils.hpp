#pragma once
#include <functional>
#include <string>
#include <string.h>

namespace we
{
    class LessCaseInsensitive: public std::binary_function<std::string, std::string, bool>
    {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const;
    };

    std::string::const_iterator find_first_of(std::string::const_iterator, std::string::const_iterator, const std::string&);
    std::string::const_iterator find_first_not(std::string::const_iterator, std::string::const_iterator, const std::string&);
    bool                        is_protocol_supported(const std::string &protocol);
    int                         char_to_hex(char c);
    std::string                 decode_percent(const std::string &str);
    bool                        is_protocol_supported(const std::string &protocol);
    bool                        is_method_supported(const std::string &method);
}