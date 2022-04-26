#include "Handler.hpp"
#include "Config.hpp"
#include "Connection.hpp"
#include "Date.hpp"

namespace we
{
  static int test_if_unmodified(Connection *con, struct stat *st)
  {
    Date unmodified_since(con->req_headers["If-Unmodified-Since"]);

    if (unmodified_since >= (st->st_mtime * 1000))
      return 1;

    return 0;
  }

  static int test_if_modified(Connection *con, struct stat *st)
  {
    Date modified_since(con->req_headers["If-Modified-Since"]);

    if (modified_since != (st->st_mtime * 1000))
      return 1;

    return 0;
  }

  static int test_if_match(Connection *con, const std::string &header, bool weak)
  {
      u_char ch;

      if (header.size() == 1 && header[0] == '*')
        return 1;

      if (con->etag.empty())
        return 0;

      std::string etag = con->etag;
      if (weak && etag.size() > 2 && strncmp(etag.c_str(), "W/", 2) == 0)
        etag = etag.substr(2);

      std::string::const_iterator start = header.begin();
      std::string::const_iterator end = header.end();

      while (start != end)
      {
          if (weak && end - start > 2 && strncmp(&(*start), "W/", 2) == 0)
            start += 2;

          if (etag.size() > (size_t)(end - start))
            return 0;

          if (strncmp(&(*start), etag.c_str(), etag.size()) != 0)
            goto skip;

          start += etag.size();
          while (start < end)
          {
            ch = *start;

            if (ch == ' ' || ch == '\t')
            {
              ++start;
              continue;
            }

            break;
          }

          if (start == end || *start == ',')
            return 1;
      skip:
          while (start < end && *start != ',') { ++start; }
          while (start < end) {
            ch = *start;

            if (ch == ' ' || ch == '\t' || ch == ',')
            {
              ++start;
              continue;
            }

            break;
          }
      }

      return 0;
  }

  static std::string generate_etag(struct stat *st)
  {
      std::stringstream ss;
      ss << "\"" << std::hex << st->st_mtime << "-" << std::hex << st->st_size << "\"";
      return ss.str();
  }

  int conditional_handler(Connection *con)
  {
    struct stat st;
    std::string &requested_resource = con->req_headers["@requested_resource"];

    if (con->response_type != Connection::ResponseType_None)
      return 0;

    if (stat(requested_resource.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
      return 0;

    con->etag = generate_etag(&st);
    con->last_modified

    if (con->req_headers.count("If-Unmodified-Since") && !test_if_unmodified(con, &st))
    {
      con->response_type = Connection::ResponseType_File;
      con->res_headers.insert(std::make_pair("@response_code", "412"));
      con->req_headers["@requested_resource"] = con->location->get_error_page(412);
      return 1;
    }

    if (con->req_headers.count("If-Match") && !test_if_match(con, con->req_headers["If-Match"], false))
    {
      con->response_type = Connection::ResponseType_File;
      con->res_headers.insert(std::make_pair("@response_code", "412"));
      con->req_headers["@requested_resource"] = con->location->get_error_page(412);
      return 1;
    }

    if (con->req_headers.count("If-Modified-Since") || con->req_headers.count("If-None-Match"))
    {
      if (con->req_headers.count("If-Modified-Since") && test_if_modified(con, &st))
        return 0;

      if (con->req_headers.count("If-None-Match") && !test_if_match(con, con->req_headers["If-None-Match"], true))
        return 0;

      con->response_type = Connection::ResponseType_File;
      con->res_headers.insert(std::make_pair("@response_code", "403"));
      con->req_headers["@requested_resource"] = con->location->get_error_page(403);
      return 1;
    }

    return 0;
  }
}