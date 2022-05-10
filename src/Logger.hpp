#pragma once

#include "Connection.hpp"

#include <arpa/inet.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <chrono>
#include <ctime>

namespace we
{
    class Logger
    {
    private:
        std::ofstream accessLog;
        std::ofstream errorLog;

        bool isAccessLogOpen;
        bool isErrorLogOpen;

    public:
        ~Logger();

        static Logger& get_instance()
        {
            static Logger _instance;
            return _instance;
        }

        void log(Connection *);
        void error(Connection *, const std::string &, const std::string &);

        void setAccessLog(std::string const &);
        void setErrorLog(std::string const &);

        void closeAccessLog();
        void closeErrorLog();

        // Utilities functions
        std::string prettyDateTime(void);

    private:
        Logger();
        Logger(Logger const&);
        Logger &operator=(Logger const&);
    };
}