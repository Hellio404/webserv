#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    int pre_access_handler(Connection *con)
    {
        if (!con->location->is_allowed_method(con->req_headers["@method"]))
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "405"));
            con->requested_resource = con->location->get_error_page(405);
            con->set_keep_alive(false);
            return 1;
        }

        return 0;
    }
}