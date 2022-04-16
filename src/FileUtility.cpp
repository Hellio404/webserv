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

    ssize_t     location_filename_cmp(const std::string &s1, const std::string &s2)
    {
        char  c1, c2;
        ssize_t n = (s1.size() <= s2.size()) ? s1.size() : s2.size();

        for (ssize_t i = 0;; ++i)
        {
            c1 = s1[i];
            c2 = s2[i];

            if (c1 == c2)
            {
                if (c1 == '\0') // end of s1
                    return 0;
                n--;
                continue;
            }

            /* we need '/' to be the lowest character */
            if (c1 == 0 || c2 == 0)
                return c1 - c2;

            c1 = (c1 == '/') ? 0 : c1;
            c2 = (c2 == '/') ? 0 : c2;

            return c1 - c2;
        }

        return 0;
    }

    bool        check_path_by_location(const std::string &path, const LocationBlock &location)
    {
        ssize_t cmp = location_filename_cmp(path, location.pattern);
        if (cmp == 0)
            return true; // the path matches the location pattern
        else if (cmp > 0 && path[location.pattern.size() - 1] == '/')
            return true; // the location is a subpath of the path

        return false;
    }

    std::string get_file_fullpath(const std::string &root, const std::string &path, ssize_t offset)
    {
        std::string fullpath = root;

        // remove trailing '/'
        if (fullpath.size() > 1 && fullpath[fullpath.size() - 1] == '/')
            fullpath.erase(fullpath.size() - 1);
        if (offset > 0)
            fullpath += path.substr(offset);
        else
            fullpath += path;

        return fullpath;
    }
}