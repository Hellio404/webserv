// Date is a class that represents a date. It can be constructed from a string
// or from a date (number, struct timeval). It can be converted to a string.
// When it is constructed from a string, it will be parsed and the date will
// be set to the date in the string.
// It can generate a string representation of the date in IMF-fixdate format
// "WDY, DD MON YYYY HH:MM:SS GMT".


#pragma once
#include <sys/time.h>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <string>
#include <ctime>

namespace we
{
  struct strint
  {
    const char *str;
    int num;
  };

  class Date
  {
    private:
      struct timeval __data;

      void __parse_date_str(char *);
      static bool __parse_day(char *, int *);
      static bool __parse_month(char *, int *);

    public:
      Date();
      Date(size_t);
      Date(struct timeval *);
      Date(struct timeval &);
      Date(std::string &date_str);
      Date(const char *date_str);

      std::string get_date_str() const;
      void set_date_str(std::string &date_str);
      void set_date_str(const char *date_str);

      // operator +
      Date operator+(size_t);
      Date operator+(const Date &);
      Date operator+(const struct timeval &);
      Date operator+(const struct timeval *);

      // operator +=
      Date &operator+=(size_t);
      Date &operator+=(const Date &);
      Date &operator+=(const struct timeval &);
      Date &operator+=(const struct timeval *);

      // operator ==
      bool operator==(size_t) const;
      bool operator==(const Date &) const;
      bool operator==(const struct timeval &) const;
      bool operator==(const struct timeval *) const;

      // operator !=
      bool operator!=(size_t) const;
      bool operator!=(const Date &) const;
      bool operator!=(const struct timeval &) const;
      bool operator!=(const struct timeval *) const;

      // operator <
      bool operator<(size_t) const;
      bool operator<(const Date &) const;
      bool operator<(const struct timeval &) const;
      bool operator<(const struct timeval *) const;

      // operator >
      bool operator>(size_t) const;
      bool operator>(const Date &) const;
      bool operator>(const struct timeval &) const;
      bool operator>(const struct timeval *) const;

      // operator <=
      bool operator<=(size_t) const;
      bool operator<=(const Date &) const;
      bool operator<=(const struct timeval &) const;
      bool operator<=(const struct timeval *) const;

      // operator >=
      bool operator>=(size_t) const;
      bool operator>=(const Date &) const;
      bool operator>=(const struct timeval &) const;
      bool operator>=(const struct timeval *) const;
  };
}