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

  Date::Date(struct timeval *tvl): __data(*tvl)
  {
  }

  Date::Date(struct timeval &tvl): __data(tvl)
  {
  }

  Date::Date(std::string &date_str)
  {
    this->__parse_date_str(const_cast<char *>(date_str.c_str()));
  }

  Date::Date(const char *date_str)
  {
    this->__parse_date_str(const_cast<char *>(date_str));
  }

  std::string Date::get_date_str() const
  {
    static const char *wday_tab[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
    static const char *mon_tab[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

    struct tm *tm_ptr;
    std::stringstream ss;

    tm_ptr = localtime(&this->__data.tv_sec);

    // Sun, 06 Nov 1994 08:49:37 GMT    ; IMF-fixdate
    ss << wday_tab[tm_ptr->tm_wday];
    ss << ", " << std::setfill('0') << std::setw(2) << tm_ptr->tm_mday;
    ss << " " << mon_tab[tm_ptr->tm_mon] << " " << tm_ptr->tm_year + 1900 << " ";
    ss << std::setfill('0') << std::setw(2) << tm_ptr->tm_hour;
    ss << ":" << std::setfill('0') << std::setw(2) << tm_ptr->tm_min;
    ss << ":" << std::setfill('0') << std::setw(2) << tm_ptr->tm_sec << " GMT";

    return ss.str();
  }

  void Date::set_date_str(std::string &date_str)
  {
    this->__parse_date_str(const_cast<char *>(date_str.c_str()));
  }

  void Date::set_date_str(const char *date_str)
  {
    this->__parse_date_str(const_cast<char *>(date_str));
  }

  void Date::__parse_date_str(char *str)
  {
    struct tm tm;
    char *cp;
    char str_mon[500], str_wday[500];

    for (cp = str; *cp == ' ' || *cp == '\t'; ++cp)
      continue;

    bzero(&tm, sizeof(tm));

    // Sun, 06 Nov 1994 08:49:37 GMT    ; IMF-fixdate
    if (sscanf(cp, "%400[a-zA-Z], %d-%400[a-zA-Z]-%d %d:%d:%d GMT", str_wday, &tm.tm_mday, str_mon, &tm.tm_year, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 7 &&
        this->__parse_day(str_wday, &tm.tm_wday) && this->__parse_month(str_mon, &tm.tm_mon))
    {
      // Date is valid and parsed
    }
    // Sunday, 06-Nov-94 08:49:37 GMT   ; obsolete RFC 850 format
    else if (sscanf(cp, "%400[a-zA-Z], %d %400[a-zA-Z] %d %d:%d:%d GMT", str_wday, &tm.tm_mday, str_mon, &tm.tm_year, &tm.tm_hour, &tm.tm_min, &tm.tm_sec) == 7 &&
             this->__parse_day(str_wday, &tm.tm_wday) && this->__parse_month(str_mon, &tm.tm_mon))
    {
      // Date is valid and parsed
    }
    // Sun Nov  6 08:49:37 1994         ; ANSI C's asctime() format
    else if (sscanf(cp, "%400[a-zA-Z] %400[a-zA-Z] %d %d:%d:%d %d", str_wday, str_mon, &tm.tm_mday, &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &tm.tm_year) == 7 &&
             this->__parse_day(str_wday, &tm.tm_wday) && this->__parse_month(str_mon, &tm.tm_mon))
    {
      // Date is valid and parsed
    }
    else
      throw std::runtime_error("Invalid date format");

    if (tm.tm_year > 1900)
      tm.tm_year -= 1900;
    else if (tm.tm_year < 70)
      tm.tm_year += 100;

    this->__data.tv_sec = mktime(&tm);
    this->__data.tv_usec = 0;
  }

  bool Date::__parse_day(char *str_wday, int *wday_ptr)
  {
    static struct strint wday_tab[] = {
      {"sun", 0}, {"sunday", 0},
      {"mon", 1}, {"monday", 1},
      {"tue", 2}, {"tuesday", 2},
      {"wed", 3}, {"wednesday", 3},
      {"thu", 4}, {"thursday", 4},
      {"fri", 5}, {"friday", 5},
      {"sat", 6}, {"saturday", 6},
    };

    for (int i = 0; i < 14; ++i)
    {
      if (strcasecmp(wday_tab[i].str, str_wday) == 0)
      {
        *wday_ptr = wday_tab[i].num;
        return true;
      }
    }

    return false;
  }

  bool Date::__parse_month(char *str_mon, int *mon_ptr)
  {
    static struct strint mon_tab[] = {
      {"jan", 0}, {"january", 0},
      {"feb", 1}, {"february", 1},
      {"mar", 2}, {"march", 2},
      {"apr", 3}, {"april", 3},
      {"may", 4},
      {"jun", 5}, {"june", 5},
      {"jul", 6}, {"july", 6},
      {"aug", 7}, {"august", 7},
      {"sep", 8}, {"september", 8},
      {"oct", 9}, {"october", 9},
      {"nov", 10}, {"november", 10},
      {"dec", 11}, {"december", 11},
    };

    for (int i = 0; i < 23; ++i)
    {
      if (strcasecmp(mon_tab[i].str, str_mon) == 0)
      {
        *mon_ptr = mon_tab[i].num;
        return true;
      }
    }

    return false;
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

  Date Date::operator+(const struct timeval &other)
  {
    Date date;
    date.__data.tv_sec = this->__data.tv_sec + other.tv_sec;
    date.__data.tv_usec = this->__data.tv_usec + other.tv_usec;
    if (date.__data.tv_usec >= 1000000)
    {
      date.__data.tv_sec += date.__data.tv_usec / 1000000;
      date.__data.tv_usec = date.__data.tv_usec % 1000000;
    }
    return date;
  }

  Date Date::operator+(const struct timeval *other)
  {
    Date date;
    date.__data.tv_sec = this->__data.tv_sec + other->tv_sec;
    date.__data.tv_usec = this->__data.tv_usec + other->tv_usec;
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

  Date &Date::operator+=(const struct timeval &other)
  {
    this->__data.tv_sec += other.tv_sec;
    this->__data.tv_usec += other.tv_usec;
    if (this->__data.tv_usec >= 1000000)
    {
      this->__data.tv_sec += this->__data.tv_usec / 1000000;
      this->__data.tv_usec = this->__data.tv_usec % 1000000;
    }
    return *this;
  }

  Date &Date::operator+=(const struct timeval *other)
  {
    this->__data.tv_sec += other->tv_sec;
    this->__data.tv_usec += other->tv_usec;
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
    return this->__data.tv_sec == ms / 1000 && this->__data.tv_usec == (ms % 1000) * 1000;
  }

  bool Date::operator==(const Date &other) const
  {
    return this->__data.tv_sec == other.__data.tv_sec && this->__data.tv_usec == other.__data.tv_usec;
  }

  bool Date::operator==(const struct timeval &other) const
  {
    return this->__data.tv_sec == other.tv_sec && this->__data.tv_usec == other.tv_usec;
  }

  bool Date::operator==(const struct timeval *other) const
  {
    return this->__data.tv_sec == other->tv_sec && this->__data.tv_usec == other->tv_usec;
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

  bool Date::operator!=(const struct timeval &other) const
  {
    return !(*this == other);
  }

  bool Date::operator!=(const struct timeval *other) const
  {
    return !(*this == other);
  }

  // operator <
  bool Date::operator<(size_t ms) const
  {
    return this->__data.tv_sec < ms / 1000 || (this->__data.tv_sec == ms / 1000 && this->__data.tv_usec < (ms % 1000) * 1000);
  }

  bool Date::operator<(const Date &other) const
  {
    return this->__data.tv_sec < other.__data.tv_sec || (this->__data.tv_sec == other.__data.tv_sec && this->__data.tv_usec < other.__data.tv_usec);
  }

  bool Date::operator<(const struct timeval &other) const
  {
    return this->__data.tv_sec < other.tv_sec || (this->__data.tv_sec == other.tv_sec && this->__data.tv_usec < other.tv_usec);
  }

  bool Date::operator<(const struct timeval *other) const
  {
    return this->__data.tv_sec < other->tv_sec || (this->__data.tv_sec == other->tv_sec && this->__data.tv_usec < other->tv_usec);
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

  bool Date::operator<=(const struct timeval &other) const
  {
    return *this < other || *this == other;
  }

  bool Date::operator<=(const struct timeval *other) const
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

  bool Date::operator>(const struct timeval &other) const
  {
    return !(*this <= other);
  }

  bool Date::operator>(const struct timeval *other) const
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

  bool Date::operator>=(const struct timeval &other) const
  {
    return !(*this < other);
  }

  bool Date::operator>=(const struct timeval *other) const
  {
    return !(*this < other);
  }
}