#include "Handler.hpp"
#include "Logger.hpp"
#include "Connection.hpp"

namespace we
{
    int logger_handler(Connection *con)
    {
        we::Logger::get_instance().log(con);
        return 1;
    }
}