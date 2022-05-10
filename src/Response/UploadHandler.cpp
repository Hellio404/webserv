#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    static void set_file_metadata(Connection *con, struct stat *st)
    {
        assert(st != NULL);

        try
        {
            con->etag = "\"" + we::to_hex(st->st_mtime) + "-" + we::to_hex(st->st_size) + "\"";
            con->last_modified = st->st_mtime * 1000;

            con->res_headers.insert(std::make_pair("ETag", con->etag));
            con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        }
        catch (...)
        {
            // Ignore exception
        }
    }

    int upload_handler(Connection *con)
    {
        if (con->response_type != Connection::ResponseType_None)
            return 0;

        if (con->req_headers["@method"] == "PUT" && con->location->allow_upload)
        {
            try
            {
                std::string uploaded_file = we::get_file_fullpath(con->location->upload_dir, con->expanded_url);

                if (uploaded_file[uploaded_file.size() - 1] == '/')
                    throw std::runtime_error("Directory upload is not allowed");

                std::string directories = uploaded_file.substr(0, uploaded_file.find_last_of('/'));
                mkdir_recursive(directories.c_str());

                con->body_handler->move_tmpfile(uploaded_file);

                struct stat st;
                if (stat(uploaded_file.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
                {
                    con->response_type = Connection::ResponseType_File;
                    con->res_headers.insert(std::make_pair("@response_code", "500"));
                    con->requested_resource = con->location->get_error_page(500);
                    con->set_keep_alive(false);
                    return 1;
                }

                set_file_metadata(con, &st);

                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "201"));
                con->requested_resource = con->location->get_error_page(201);
                return 1;
            }
            catch (...)
            {
                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "500"));
                con->requested_resource = con->location->get_error_page(500);
                con->set_keep_alive(false);
                return 1;
            }
        }

        if (con->req_headers["@method"] == "DELETE" && con->location->allow_upload)
        {
            try
            {
                std::string uploaded_file = we::get_file_fullpath(con->location->upload_dir, con->expanded_url);

                int fd = -1;
                struct stat st;
                if (stat(uploaded_file.c_str(), &st))
                {
                    con->response_type = Connection::ResponseType_File;
                    con->res_headers.insert(std::make_pair("@response_code", "404"));
                    con->requested_resource = con->location->get_error_page(404);
                    con->set_keep_alive(false);
                    return 1;
                }
                else if (S_ISDIR(st.st_mode))
                {
                    con->response_type = Connection::ResponseType_File;
                    con->res_headers.insert(std::make_pair("@response_code", "403"));
                    con->requested_resource = con->location->get_error_page(403);
                    con->set_keep_alive(false);
                    return 1;
                }

                fd = open(uploaded_file.c_str(), O_WRONLY);
                if (fd == -1)
                {
                    con->response_type = Connection::ResponseType_File;
                    con->res_headers.insert(std::make_pair("@response_code", "403"));
                    con->requested_resource = con->location->get_error_page(403);
                    con->set_keep_alive(false);
                    return 1;
                }

                unlink(uploaded_file.c_str());
                close(fd);

                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "200"));
                con->requested_resource = con->location->get_error_page(200);
                return 1;
            }
            catch(...)
            {
                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "500"));
                con->requested_resource = con->location->get_error_page(500);
                con->set_keep_alive(false);
                return 1;
            }
        }

        return 0;
    }
}
