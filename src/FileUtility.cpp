#include "FileUtility.hpp"

namespace we
{
    FileStatus::Type    check_file_validity(const std::string &file_path)
    {
        struct stat file_stat;

        // Check if file exists
        if (stat(file_path.c_str(), &file_stat))
            return FileStatus::FILE_NOT_FOUND;

        // check if it's a directory
        if (S_ISDIR(file_stat.st_mode))
            return FileStatus::FILE_IS_DIRECTORY;

        // check if it is a regular file
        if (!S_ISREG(file_stat.st_mode))
            return FileStatus::FILE_NOT_REGULAR;

        // check if it is readable by the current user (or globally)
        if (!(file_stat.st_mode & S_IRUSR)
            && !(file_stat.st_mode & S_IRGRP)
            && !(file_stat.st_mode & S_IROTH))
            return FileStatus::FILE_NOT_READABLE;

        return FileStatus::FILE_OK;
    }

    bool        check_path_by_location(const std::string &path, const LocationBlock &location)
    {
        if (strcasecmp(path.c_str(), location.pattern.c_str()) >= 0)
            return true;
        return false;
    }

    std::string get_file_fullpath(const std::string &root, const std::string &path)
    {
        std::string fullpath = root;

        // remove trailing '/'
        if (fullpath.size() >= 1 && fullpath[fullpath.size() - 1] == '/')
            fullpath.erase(fullpath.size() - 1);

        return fullpath + path;
    }
}