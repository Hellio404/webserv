#include "Handler.hpp"
#include "Connection.hpp"

namespace we
{
    static void set_file_metadata(Connection *con, struct stat *st)
    {
        assert(st != NULL);

        con->etag = "\"" + we::to_hex(st->st_mtime) + "-" + we::to_hex(st->st_size) + "\"";
        con->last_modified = st->st_mtime * 1000;

        con->res_headers.insert(std::make_pair("ETag", con->etag));
        try
        {
            con->res_headers.insert(std::make_pair("Last-Modified", con->last_modified.get_date_str()));
        } catch(...) {}
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
                con->body_handler->move_tmpfile(uploaded_file);

                struct stat st;
                if (stat(uploaded_file.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
                    throw std::runtime_error("File not found");

                set_file_metadata(con, &st);

                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "201"));
                con->requested_resource = con->location->get_error_page(201);
                con->res_headers.insert(std::make_pair("@handler", "upload"));
                return 1;
            }
            catch(...)
            {
                con->response_type = Connection::ResponseType_File;
                con->res_headers.insert(std::make_pair("@response_code", "500"));
                con->requested_resource = con->location->get_error_page(500);
                con->res_headers.insert(std::make_pair("@handler", "upload"));
                return 1;
            }
        }

        return 0;
    }
}
