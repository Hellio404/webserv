#include "Handler.hpp"
#include "Config.hpp"
#include "Connection.hpp"
namespace we
{
    static void internal_redirect(Connection *con, const std::string &url)
    {
        if (con->redirect_count == 0)
        {
            con->response_type = Connection::ResponseType_File;
            con->requested_resource = con->location->get_error_page(500);
            con->res_headers.insert(std::make_pair("@response_code", "500"));
            // TODO: we should add keep alive field to the connection and set it to false in case of errors
        }
        else
        {

            con->redirect_count--;
            con->phase = Phase_Pre_Start;
            con->requested_resource = url;
            con->location = con->server->get_location(con->requested_resource);
        }
    }

    int index_handler(Connection *con)
    {
        const LocationBlock *location = con->location;
        int fd;
        struct stat st;

        if (con->response_type != Connection::ResponseType_None)
            return 1;

        if (stat(con->requested_resource.c_str(), &st) == -1 || !S_ISDIR(st.st_mode))
            return 0;

        // TODO: use stat for better error handling
        for (int i = 0; i < location->index.size(); ++i)
        {
            std::string index_path = con->requested_resource + "/" + location->index[i];
            if ((fd = open(index_path.c_str(), O_RDONLY)) != -1)
            {
                close(fd);
                internal_redirect(con, index_path);               
                return 1;
            }
        }
        return 0;
    }

}