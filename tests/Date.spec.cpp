#include <iostream>
#include <unistd.h>
#include "../src/Date.hpp"

int main()
{
  {
    we::Date date;
    std::cout << "\033[1;33m" << "Current date: \033[1;32m" << date.get_date_str() << std::endl << std::endl;

    sleep(2);
    we::Date date2;

    assert((date != date2) == true);
    assert((date == date2) == false);
    assert((date < date2) == true);
    assert((date <= date2) == true);
    assert((date > date2) == false);
    assert((date >= date2) == false);
    std::cout << "✅ \033[1;32m Test 0 passed!\033[0m ✅" << std::endl;
  }
  {
    we::Date date("Sun, 06 Nov 1994 08:49:37 GMT");
    assert(date.get_date_str().compare("Sun, 06 Nov 1994 08:49:37 GMT") == 0);
    std::cout << "✅ \033[1;32m Test 1 passed!\033[0m ✅" << std::endl;
  }
  {
    we::Date date("Sunday, 06-Nov-94 08:49:37 GMT");
    assert(date.get_date_str().compare("Sun, 06 Nov 1994 08:49:37 GMT") == 0);
    std::cout << "✅ \033[1;32m Test 2 passed!\033[0m ✅" << std::endl;
  }
  {
    we::Date date("Sun Nov  6 08:49:37 1994");
    assert(date.get_date_str().compare("Sun, 06 Nov 1994 08:49:37 GMT") == 0);
    std::cout << "✅ \033[1;32m Test 3 passed!\033[0m ✅" << std::endl;
  }
  {
    bool failed = false;
    try
    {
      we::Date date("Sunset Nov  6 08:49:37 1994");
    }
    catch(...)
    {
      failed = true;
    }
    assert(failed == true);
    std::cout << "✅ \033[1;32m Test 4 passed!\033[0m ✅" << std::endl;
  }
  {
    bool failed = false;
    try
    {
      we::Date date;
      date.set_date_str("Sunday, 06-Nov-94 08:49:37");
    }
    catch(...)
    {
      failed = true;
    }
    assert(failed == true);
    std::cout << "✅ \033[1;32m Test 5 passed!\033[0m ✅" << std::endl;
  }
  {
    struct timeval tv;
    tv.tv_sec = 3;
    tv.tv_usec = 0;
    we::Date date("Sun, 06 Nov 1994 08:49:37 GMT");
    assert((date + 3000).get_date_str().compare("Sun, 06 Nov 1994 08:49:40 GMT") == 0);
    assert((date + tv).get_date_str().compare("Sun, 06 Nov 1994 08:49:40 GMT") == 0);
    assert((date + &tv).get_date_str().compare("Sun, 06 Nov 1994 08:49:40 GMT") == 0);
    assert((date += 3000).get_date_str().compare("Sun, 06 Nov 1994 08:49:40 GMT") == 0);
    assert((date += tv).get_date_str().compare("Sun, 06 Nov 1994 08:49:43 GMT") == 0);
    assert((date += &tv).get_date_str().compare("Sun, 06 Nov 1994 08:49:46 GMT") == 0);
    std::cout << "✅ \033[1;32m Test 6 passed!\033[0m ✅" << std::endl;
  }
  {
    we::Date date("Sun, 06 Nov 1900 08:49:37 GMT");
    std::cout << "✅ \033[1;32m Test 7 passed!\033[0m ✅" << std::endl;
  }
}