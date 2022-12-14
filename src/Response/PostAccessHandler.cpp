#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    int post_access_handler(Connection *con)
    {
        if (con->response_type == Connection::ResponseType_None)
        {
            struct stat st;
            if (con->req_headers["@method"] != "GET" && con->req_headers["@method"] != "HEAD")
            {
                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "405"));
                con->requested_resource = con->location->get_error_page(405);
                con->set_keep_alive(false);
            }
            else if (stat(con->requested_resource.c_str(), &st) != 0)
            {
                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "404"));
                con->requested_resource = con->location->get_error_page(404);
                con->set_keep_alive(false);
            }
            else
            {
                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "403"));
                con->requested_resource = con->location->get_error_page(403);
                con->set_keep_alive(false);
            }
        }

        if (con->response_type == Connection::ResponseType_File && con->metadata_set == false)
        {
            struct stat st;
            if (stat(con->requested_resource.c_str(), &st) == 0)
            {
                std::string content_length = we::to_string(st.st_size);
                con->content_length = st.st_size;
                con->etag = "\"" + we::to_string(st.st_mtime) + "-" + content_length + "-" + we::to_string(st.st_size) + "\"";
                con->last_modified = st.st_mtime * 1000;
                con->mime_type = con->config.get_mime_type(con->requested_resource);
            }
        }

        if (con->res_headers.find("@response_code")->second[0] >= '4')
            con->set_keep_alive(false);

        return 1;
    }
}