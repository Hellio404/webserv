#pragma once

#include "Config.hpp"

#include <sys/stat.h>
#include <fstream>
#include <string>

namespace we
{
    struct FileStatus
    {
        typedef enum
        {
            FILE_OK = 0,
            FILE_NOT_FOUND,
            FILE_NOT_REGULAR,
            FILE_NOT_READABLE,
            FILE_IS_DIRECTORY,
        } Type;
    };

    FileStatus::Type    check_file_validity(const std::string &);
    bool                check_path_by_location(const std::string &, const LocationBlock &);
    ssize_t             location_filename_cmp(const std::string &, const std::string &);
    std::string         get_file_fullpath(const std::string &, const std::string &, ssize_t);
}