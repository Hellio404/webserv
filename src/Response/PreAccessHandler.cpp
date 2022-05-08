#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    int pre_access_handler(Connection *con)
    {

        if (!con->location->is_allowed_method(con->req_headers["@method"]))
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "403"));
            con->requested_resource = con->location->get_error_page(403);
            con->res_headers.insert(std::make_pair("@handler", "pre_access"));
            return 1;
        }
        return 0;
    }
}