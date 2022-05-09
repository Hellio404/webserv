#include "Handler.hpp"
#include "Config.hpp"
#include "Connection.hpp"

namespace we
{
    static void internal_redirect(Connection *con, const std::string &url, const std::string &new_expanded)
    {
        if (con->redirect_count == 0)
        {
            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "500"));
            con->requested_resource = con->location->get_error_page(500);
            con->keep_alive = false;
        }
        else
        {
            con->redirect_count--;
            con->phase = Phase_Pre_Start;
            con->requested_resource = url;
            con->expanded_url = new_expanded;
            con->location = con->server->get_location(new_expanded);
        }
    }

    int index_handler(Connection *con)
    {
        int fd;
        struct stat st;
        const LocationBlock *location = con->location;

        if (con->response_type != Connection::ResponseType_None)
            return 1;

        if (stat(con->requested_resource.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
            return 0;

        for (size_t i = 0; i < location->index.size(); ++i)
        {
            std::string index_path = con->requested_resource;
            if (index_path[index_path.size() - 1] != '/' && location->index[i][0] != '/')
                index_path += '/';
            index_path += location->index[i];
            if ((fd = open(index_path.c_str(), O_RDONLY)) != -1)
            {
                close(fd);
                std::string new_expanded = con->expanded_url;
                if (new_expanded[new_expanded.size() - 1] != '/' && location->index[i][0] != '/')
                    new_expanded += "/";
                new_expanded += location->index[i];
                internal_redirect(con, index_path, new_expanded);
                return 1;
            }
        }

        return 0;
    }
}