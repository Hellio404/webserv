#pragma once

#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <utility>
#include <string.h>
#include <iomanip>

namespace we
{
    class HTTPStatusException: public std::exception
    {
    public:
        explicit HTTPStatusException(int statusCode, const std::string &message): message(message)
        {
            if (statusCode < 400 || (statusCode > 417 && statusCode < 500) || statusCode > 505)
                this->statusCode = 500;
            else
                this->statusCode = statusCode;
        }

        virtual ~HTTPStatusException() throw() {}

        virtual const char *what() const throw()
        {
            return message.c_str();
        }

        int statusCode;
        std::string message;
    };

    class LessCaseInsensitive: public std::binary_function<std::string, std::string, bool>
    {
    public:
        bool operator()(const std::string& lhs, const std::string& rhs) const;
    };

    std::string::const_iterator find_first_of(std::string::const_iterator, std::string::const_iterator, const std::string&);
    std::string::const_iterator find_first_not(std::string::const_iterator, std::string::const_iterator, const std::string&);
    bool                        is_protocol_supported(const std::string &);
    int                         char_to_hex(char);
    std::string                 decode_percent(const std::string &);
    bool                        is_protocol_supported(const std::string &);
    bool                        is_method_supported(const std::string &);
    std::string                 to_hex(size_t);

    void                        start_recording(unsigned char, const char *);
    void                        stop_recording(unsigned char);

    void                        print_ids_time();

    std::string                 get_status_string(unsigned int);
    std::string                 get_default_page_code(unsigned int);

    template<class  T>
    std::string                 to_string(T const & val, size_t pad = 0, char padding = ' ')
    {
        std::ostringstream ss;
        ss << std::setfill(padding) << std::setw(pad) << val;
        return ss.str();
    }

    std::string                 capitalize_header(const std::string  & );

    size_t                      get_next_boundry();

    std::string                 make_range_header(size_t , const std::string *, std::pair<unsigned long long , unsigned long long > , unsigned long long );

    size_t                      number_len(size_t s);

    bool                        is_bodiless_response(unsigned int);

}