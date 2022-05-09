#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    int autoindex_handler(Connection *con)
    {
        std::string &requested_resource = con->requested_resource;

        // Move on to the next handler if the requested_resource is not a directory (doesn't end with '/')
        if (requested_resource.size() && requested_resource[requested_resource.size() - 1] != '/')
            return 0;
        // Move on to the next handler if the request is not a GET/HEAD and/or autoindex is disabled
        if ((con->req_headers["@method"] != "GET" && con->req_headers["@method"] != "HEAD") || con->location->autoindex == false)
            return 0;

        if (con->response_type != Connection::ResponseType_None)
            return 1;

        DIR *dir = opendir(requested_resource.c_str());
        if (dir == NULL && (errno == ENOTDIR || errno == ENOENT || errno == ENAMETOOLONG))
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "404"));
            con->requested_resource = con->location->get_error_page(404);
        }
        else if (dir == NULL && errno == EACCES)
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "403"));
            con->requested_resource = con->location->get_error_page(403);
        }
        else if (dir == NULL)
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "500"));
            con->requested_resource = con->location->get_error_page(500);
        }
        else
        {
            closedir(dir);
            con->response_type = Connection::ResponseType_Directory;
            con->res_headers.insert(std::make_pair("@response_code", "200"));
        }

        con->set_keep_alive(false);
        return 1;
    }
}
