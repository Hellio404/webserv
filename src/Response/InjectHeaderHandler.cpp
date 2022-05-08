#include "Handler.hpp"
#include "Connection.hpp"
#include "Logger.hpp"

namespace we
{
    int inject_header_handler(Connection *con)
    {
        std::map<std::string, std::string>::const_iterator it = con->location->added_headers.begin();

        for (; it != con->location->added_headers.end(); ++it)
        {
            con->res_headers.insert(*it);
        }

        // Inject default headers
        we::Date date;

        con->res_headers.insert(std::make_pair("Server", "BetterNginx/0.69.420"));
        con->res_headers.insert(std::make_pair("Date", date.get_date_str()));

        return 0;
    }
}