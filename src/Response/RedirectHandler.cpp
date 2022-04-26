#include "Handler.hpp"
#include "Config.hpp"
#include "Connection.hpp"
namespace we
{
    int redirect_handler(Connection *con)
    {
        std::string &requested_resource = con->requested_resource;
        if (con->location->is_redirection == false)
        {
            struct stat st;
            if (requested_resource.size() && requested_resource[requested_resource.size() - 1] != '/' &&
                stat(requested_resource.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
            {
                std::string location = "http://" + con->req_headers["Host"] + con->expanded_url + "/";

                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("Location", location));
                con->res_headers.insert(std::make_pair("@response_code", "301"));
                con->res_headers.insert(std::make_pair("@handler", "redirect"));

                con->keep_alive = false;

                con->requested_resource = "";
                return 1;
            }

            return 0;
        }

        std::string location = con->location->redirect_url;
        if (strncasecmp(location.c_str(), "http://", 7) && strncasecmp(location.c_str(), "https://", 8))
        {
            location = "http://" + con->req_headers["Host"];
            if (con->location->redirect_url[0] != '/')
                location += "/";
            location += con->location->redirect_url;
        }

        con->response_type = Connection::ResponseType_File;
        con->res_headers.insert(std::make_pair("Location", location));
        con->res_headers.insert(std::make_pair("@response_code", we::to_string(con->location->return_code)));
        con->res_headers.insert(std::make_pair("@handler", "redirect"));

        con->keep_alive = false;

        con->requested_resource = "";
        return 1;
    }
}