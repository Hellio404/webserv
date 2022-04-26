#include "Handler.hpp"
#include "Logger.hpp"
#include "Connection.hpp"

namespace we
{
    int logger_handler(Connection *con)
    {
        Logger &logger = Logger::get_instance();

        logger.logAccess(con);

        return 1;
    }
}


