#include "Date.hpp"

namespace we
{
    Date::Date()
    {
        gettimeofday(&this->__data, NULL);
    }

    Date::Date(size_t ms)
    {
        this->__data.tv_sec = ms / 1000;
        this->__data.tv_usec = (ms % 1000) * 1000;
    }

    Date::Date(const struct timeval *tvl) : __data(*tvl)
    {
    }

    Date::Date(const struct timeval &tvl) : __data(tvl)
    {
    }

    Date::Date(const std::string &date_str)
    {
        this->__parse_date_str(date_str.c_str());
    }

    Date::Date(const char *date_str)
    {
        this->__parse_date_str(date_str);
    }

    std::string Date::get_date_str() const
    {
        char buff[64];
        struct tm *tm = localtime(&this->__data.tv_sec);

        // Sun, 06 Nov 1994 08:49:37 GMT    ; IMF-fixdate
        if (strftime(buff, sizeof(buff), "%a, %d %b %Y %H:%M:%S GMT", tm) == 0)
            throw std::runtime_error("Failed to format date string");

        return std::string(buff);
    }

    void Date::set_date_str(const std::string &date_str)
    {
        this->__parse_date_str(date_str.c_str());
    }

    void Date::set_date_str(const char *date_str)
    {
        this->__parse_date_str(date_str);
    }

    void Date::__parse_date_str(const char *str)
    {
        struct tm tm;
        const char *ret;

        bzero(&tm, sizeof(tm));
        // Sunday, 06-Nov-94 08:49:37 GMT   ; obsolete RFC 850 format
        ret = strptime(str, "%A, %d-%b-%y %H:%M:%S GMT", &tm);
        if (ret != NULL && *ret == '\0')
        {
            if (tm.tm_year < 0)
                throw std::runtime_error("Failed to parse date string");
            this->__data.tv_sec = mktime(&tm);
            this->__data.tv_usec = 0;
            return;
        }
        // Sun, 06 Nov 1994 08:49:37 GMT    ; IMF-fixdate
        ret = strptime(str, "%a, %d %b %Y %H:%M:%S GMT", &tm);
        if (ret != NULL && *ret == '\0')
        {
            if (tm.tm_year < 0)
                throw std::runtime_error("Failed to parse date string");
            this->__data.tv_sec = mktime(&tm);
            this->__data.tv_usec = 0;
            return;
        }
        // Sun Nov  6 08:49:37 1994         ; ANSI C's asctime() format
        ret = strptime(str, "%a %b %d %H:%M:%S %Y", &tm);
        if (ret != NULL && *ret == '\0')
        {
            if (tm.tm_year < 0)
                throw std::runtime_error("Failed to parse date string");
            this->__data.tv_sec = mktime(&tm);
            this->__data.tv_usec = 0;
            return;
        }

        throw std::runtime_error("Invalid date format");
    }

    // operator +
    Date Date::operator+(size_t ms)
    {
        Date date;
        date.__data.tv_sec = this->__data.tv_sec + ms / 1000;
        date.__data.tv_usec = this->__data.tv_usec + (ms % 1000) * 1000;
        if (date.__data.tv_usec >= 1000000)
        {
            date.__data.tv_sec += date.__data.tv_usec / 1000000;
            date.__data.tv_usec = date.__data.tv_usec % 1000000;
        }
        return date;
    }

    Date Date::operator+(const Date &other)
    {
        Date date;
        date.__data.tv_sec = this->__data.tv_sec + other.__data.tv_sec;
        date.__data.tv_usec = this->__data.tv_usec + other.__data.tv_usec;
        if (date.__data.tv_usec >= 1000000)
        {
            date.__data.tv_sec += date.__data.tv_usec / 1000000;
            date.__data.tv_usec = date.__data.tv_usec % 1000000;
        }
        return date;
    }

    // operator +=
    Date &Date::operator+=(size_t ms)
    {
        this->__data.tv_sec += ms / 1000;
        this->__data.tv_usec += (ms % 1000) * 1000;
        if (this->__data.tv_usec >= 1000000)
        {
            this->__data.tv_sec += this->__data.tv_usec / 1000000;
            this->__data.tv_usec = this->__data.tv_usec % 1000000;
        }
        return *this;
    }

    Date &Date::operator+=(const Date &other)
    {
        this->__data.tv_sec += other.__data.tv_sec;
        this->__data.tv_usec += other.__data.tv_usec;
        if (this->__data.tv_usec >= 1000000)
        {
            this->__data.tv_sec += this->__data.tv_usec / 1000000;
            this->__data.tv_usec = this->__data.tv_usec % 1000000;
        }
        return *this;
    }

    // operator ==
    bool Date::operator==(size_t ms) const
    {
        return this->__data.tv_sec == (time_t)(ms / 1000);
    }

    bool Date::operator==(const Date &other) const
    {
        return this->__data.tv_sec == other.__data.tv_sec;
    }

    // operator !=
    bool Date::operator!=(size_t ms) const
    {
        return !(*this == ms);
    }

    bool Date::operator!=(const Date &other) const
    {
        return !(*this == other);
    }

    // operator <
    bool Date::operator<(size_t ms) const
    {
        return this->__data.tv_sec < (time_t)(ms / 1000);
    }

    bool Date::operator<(const Date &other) const
    {
        return this->__data.tv_sec < other.__data.tv_sec;
    }

    // operator <=
    bool Date::operator<=(size_t ms) const
    {
        return *this < ms || *this == ms;
    }

    bool Date::operator<=(const Date &other) const
    {
        return *this < other || *this == other;
    }

    // operator >
    bool Date::operator>(size_t ms) const
    {
        return !(*this <= ms);
    }

    bool Date::operator>(const Date &other) const
    {
        return !(*this <= other);
    }

    // operator >=
    bool Date::operator>=(size_t ms) const
    {
        return !(*this < ms);
    }

    bool Date::operator>=(const Date &other) const
    {
        return !(*this < other);
    }
}