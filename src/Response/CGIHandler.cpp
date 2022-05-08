#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    int cgi_handler(Connection *con)
    {
        std::cerr  << "CGI handler" << std::endl;
        struct stat st;
        std::string &requested_resource = con->requested_resource;

        // Move on to the next handler if the location is not a cgi script
        if (con->location->cgi.empty())
        {
            std::cerr << "Not a cgi script" << std::endl;
            return 0;
        }

        if (con->response_type != Connection::ResponseType_None)
            return 1;

        if (stat(requested_resource.c_str(), &st))
        {
            std::cerr << "CGI handler: stat failed" << std::endl;
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "404"));
            con->requested_resource = con->location->get_error_page(404);
            return 1;
        }

        int fd = open(requested_resource.c_str(), O_RDONLY);
        if (fd == -1 && (S_ISDIR(st.st_mode) || errno == EACCES))
        {
            std::cerr << "CGI handler: open failed" << std::endl;

            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "403"));
            con->requested_resource = con->location->get_error_page(403);
        }
        else if (fd == -1)
        {
            std::cerr << "CGI handler: open failed" << std::endl;
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "404"));
            con->requested_resource = con->location->get_error_page(404);
        }
        else
        {
            std::cerr << "CGI handler: file found" << std::endl;
            close(fd);

            con->response_type = Connection::ResponseType_CGI;

            con->res_headers.insert(std::make_pair("@response_code", "200"));
            con->res_headers.insert(std::make_pair("@handler", "cgi_handler"));

            con->keep_alive = false;
        }

        return 1;
    }
}
