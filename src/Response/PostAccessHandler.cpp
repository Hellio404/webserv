#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    int post_access_handler(Connection *con)
    {
        if (con->response_type != Connection::ResponseType_None)
            return 1;

        struct stat st;
        if (stat(con->req_headers["@requested_resource"].c_str(), &st) != 0)
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "404"));
            con->req_headers["@requested_resource"] = con->location->get_error_page(404);
            con->res_headers.insert(std::make_pair("X-Handler", "post_access"));
        }
        else
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "403"));
            con->req_headers["@requested_resource"] = con->location->get_error_page(403);
            con->res_headers.insert(std::make_pair("X-Handler", "post_access"));
        }
        return 1;
    }
}