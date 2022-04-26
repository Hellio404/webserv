#include "Handler.hpp"
#include "Config.hpp"
#include "Connection.hpp"
#include "Date.hpp"

#include <iomanip>

namespace we
{

    static unsigned long long range_utoa(std::string::const_iterator begin, std::string::const_iterator end, bool &valid)
    {
        unsigned long long ret = 0;
        for (std::string::const_iterator it = begin; it != end; ++it)
        {
            if (!isdigit(*it))
                throw std::runtime_error("Invalid range");
            unsigned long long prev_val = ret;
            ret *= 10;
            ret += *it - '0';
            if (ret < prev_val)
            {
                valid = false;
                return 0;
            }
        }
        valid = true;
        return ret;
    }

    typedef unsigned long long ull;
    static bool handle_range(std::string const &range, std::vector<std::pair<ull, ull> > &vranges, ull max_size)
    {

        std::set<std::pair<ull, ull> > ranges;

        if (range.size() > 1024 || range.find("bytes=") != 0)
            return false;
        std::string::const_iterator it = range.begin() + 6;
        try
        {
            while (it != range.end())
            {
                bool valid = true;
                bool reverse = false;
                std::pair<ull, ull> r;
                std::string::const_iterator it_start = it;

                while (it != range.end() && *it != '-')
                    ++it;
                if (it == it_start)
                {
                    r.first = 0;
                    reverse = true;
                }
                else
                    r.first = range_utoa(it_start, it, valid);
                if (*it == '-')
                    ++it;
                else if (it != range.end())
                    return false;
                it_start = it;
                while (it != range.end() && *it != ',')
                    ++it;
                
                if (it == it_start && !reverse)
                    r.second = max_size - 1;
                else if (!reverse)
                    r.second = std::min(range_utoa(it_start, it, valid), max_size - 1);
                else if (reverse)
                {
                    ull rg = range_utoa(it_start, it, valid);
                    if (rg > max_size - 1 || !max_size)
                        valid = false;
                    r.first = max_size - rg - 1;
                    r.second = max_size - 1;
                }
                else
                    valid = false;
                if (*it == ',')
                    ++it;
                else if (it != range.end())
                    return false;
                if (r.first >= r.second || !valid)
                    continue;

                ranges.insert(r);
                if (ranges.size() > 255)
                    return false;
            }
        }
        catch (...)
        {
            return false;
        }
        if (ranges.empty())
            return false;

        vranges.push_back(*ranges.begin());
        std::set<std::pair<ull, ull> >::const_iterator it_r = ranges.begin();
        std::vector<std::pair<ull, ull> >::const_iterator pit_r;
        ++it_r;
        for (; it_r != ranges.end(); ++it_r)
        {
            pit_r = vranges.end() - 1;
            if ((pit_r->second >= it_r->first || pit_r->second + 80 >= it_r->first) && pit_r->second >= it_r->second)
                continue;
            else if (pit_r->second >= it_r->first || pit_r->second + 80 >= it_r->first)
            {
                std::pair<ull, ull> new_r;
                new_r.first = pit_r->first;
                new_r.second = it_r->second;
                vranges.pop_back();
                vranges.push_back(new_r);
            }
            else
                vranges.push_back(*it_r);
        }
        return true;
    }

    int file_handler(Connection *con)
    {
        std::string &requested_resource = con->req_headers["@requested_resource"];

        if (con->response_type != Connection::ResponseType_None)
            return 1;
        struct stat st;

        if (stat(requested_resource.c_str(), &st) != 0 || S_ISDIR(st.st_mode))
            return 0;

        int fd = open(requested_resource.c_str(), O_RDONLY);
        if (fd != -1)
        {
            close(fd);
            std::string content_length = we::to_string(st.st_size);
            con->content_length = st.st_size;
            con->etag = "\"" + we::to_string(st.st_mtime) + "-" + content_length + "-" + we::to_string(st.st_size) + "\"";
            con->last_modified = st.st_mtime * 1000;
            con->mime_type = "application/octet-stream"; // TODO: detect mime type

            if (con->req_headers.count("Range") && con->req_headers["Range"] != "")
            {
                if (handle_range(con->req_headers["Range"], con->ranges, st.st_size))
                {
                    con->response_type = Connection::ResponseType_RangeFile;
                    con->res_headers.insert(std::make_pair("@response_code", "206"));
                    con->res_headers.insert(std::make_pair("@handler", "file_handler/range"));
                    con->res_headers.insert(std::make_pair("@file", requested_resource));
                    return 1;
                }
                else
                {
                    con->response_type = Connection::ResponseType_File;
                    con->res_headers.insert(std::make_pair("@response_code", "416"));
                    con->res_headers.insert(std::make_pair("@handler", "file_handler/range"));
                    con->res_headers.insert(std::make_pair("@file", requested_resource));
                    con->req_headers["@requested_resource"] = con->location->get_error_page(416); // TODO: recalculate length
                    return 1;
                }
            }

            con->response_type = Connection::ResponseType_File;
            con->res_headers.insert(std::make_pair("@response_code", "200"));
            con->res_headers.insert(std::make_pair("@content-length", content_length));
            con->res_headers.insert(std::make_pair("@handler", "file_handler"));
            con->res_headers.insert(std::make_pair("@file", requested_resource));

            return 1;
        }
        return 0;
    }
}