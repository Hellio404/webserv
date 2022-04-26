#include "Handler.hpp"
#include "Connection.hpp"
#include "Logger.hpp"
namespace we
{
    int response_server_handler(Connection *con)
    {
        try
        {
            if (con->response_type == Connection::ResponseType_File)
                con->response_server = new we::ResponseServerFile(con);
            else if (con->response_type == Connection::ResponseType_Directory)
                con->response_server = new ResponseServerDirectory(con);
            else if (con->response_type == Connection::ResponseType_RangeFile)
            {
                if (con->ranges.size() == 1)
                    con->response_server = new ResponseServerFileSingleRange(con);
                else
                    con->response_server = new ResponseServerFileMultiRange(con);
            }
        }
        catch (const std::exception &e)
        {
            we::Logger::get_instance().error(con, e.what());

            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "500"));
            con->requested_resource = "";
            con->response_server = new we::ResponseServerFile(con);
        }

        return 1;
    }
}