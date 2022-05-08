#pragma once


#include <iomanip>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/errno.h>

# define PHASE_NUMBER 10

namespace we
{
  class Connection;
  enum Phase: int
  {
    Phase_Pre_Start = 0,
    Phase_Reserved_1,
    Phase_Reserved_2,
    Phase_Access,
    Phase_Post_Access,
    Phase_Inject_Headers,
    Phase_Reserved_3,
    Phase_Reserved_4,
    Phase_Logging,
    Phase_End,
  };

  int pre_access_handler(Connection *);
  int logger_handler(Connection *);
  int post_access_handler(Connection *);
  int index_handler(Connection *);
  int autoindex_handler(Connection *);
  int upload_handler(Connection *);
  int cgi_handler(Connection *);
  int file_handler(Connection *);
  int redirect_handler(Connection *);

  int response_server_handler(Connection *);

  int inject_header_handler(Connection *);

  int conditional_handler(Connection *);

}