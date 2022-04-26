#pragma once

#include <iomanip>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/errno.h>

# define PHASE_NUMBER 6

namespace we
{
  class Connection;
  enum Phase: int
  {
    Phase_Reserved_1 = 0,
    Phase_Reserved_2,
    Phase_Access,
    Phase_Logging,
    Phase_Reserved_3,
    Phase_Reserved_4
  };

  int logger_handler(Connection *);
  int post_access_handler(Connection *);
  int index_handler(Connection *);
  int autoindex_handler(Connection *);
  int file_handler(Connection *);
  int redirect_handler(Connection *);


  int conditional_handler(Connection *);

}