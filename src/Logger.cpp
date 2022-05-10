#include "Logger.hpp"

namespace we
{
    Logger::Logger() : isAccessLogOpen(false), isErrorLogOpen(false)
    {
    }

    Logger::~Logger()
    {
    }

    void Logger::log(Connection *connection)
    {
        if (this->isAccessLogOpen == false)
            return;

        std::string status_code = "-";
        if (connection->res_headers.count("@response_code"))
            status_code = connection->res_headers.find("@response_code")->second;

        std::string bytes_sent = "-";
        if (connection->res_headers.count("Content-Length"))
            bytes_sent = connection->res_headers.find("Content-Length")->second;

        std::string http_referer = "-";
        if (connection->req_headers.count("Referer"))
            http_referer = connection->req_headers.find("Referer")->second;

        std::string http_user_agent = "-";
        if (connection->req_headers.count("User-Agent"))
            http_user_agent = connection->req_headers.find("User-Agent")->second;

        this->accessLog << connection->client_addr_str
                        << " - - [" << this->prettyDateTime() << "] \""
                        << connection->req_headers["@method"] << " " << connection->req_headers["@path"] << " " << connection->req_headers["@protocol"]
                        << "\" " << status_code << " " << bytes_sent << " \"" << http_referer << "\" \""
                        << http_user_agent << "\"" << std::endl;
    }

    void Logger::error(Connection *connection, const std::string &level, const std::string &message)
    {
        if (this->isErrorLogOpen == false)
            return;

        this->errorLog << this->prettyDateTime() << " [" << level << "]: " << message;
        if (connection != NULL)
        {
            this->errorLog << ", client: \"" << connection->client_addr_str << "\"";
            if (connection->req_headers.count("@method") && connection->req_headers.count("@path") && connection->req_headers.count("@protocol"))
                this->errorLog << ", request: \"" << connection->req_headers["@method"] << " " << connection->req_headers["@path"] << " " << connection->req_headers["@protocol"] << "\"";
            if (connection->res_headers.count("Host"))
                this->errorLog << ", host: \"" << connection->req_headers["Host"] << "\"";
            if (connection->server != NULL)
                this->errorLog << ", server: \"" << connection->server->server_names.front() << "\"";
        }
        this->errorLog << std::endl;
    }

    void Logger::setAccessLog(std::string const &path)
    {
        this->accessLog.open(path.c_str(), std::ios::out | std::ios::app);
        if (this->accessLog.is_open())
            this->isAccessLogOpen = true;
    }

    void Logger::setErrorLog(std::string const &path)
    {
        this->errorLog.open(path.c_str(), std::ios::out | std::ios::app);
        if (this->errorLog.is_open())
            this->isErrorLogOpen = true;
    }

    void Logger::closeAccessLog()
    {
        if (this->isAccessLogOpen)
            this->accessLog.close();
        this->isAccessLogOpen = false;
    }

    void Logger::closeErrorLog()
    {
        if (this->isErrorLogOpen)
            this->errorLog.close();
        this->isErrorLogOpen = false;
    }

    std::string Logger::prettyDateTime()
    {
        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        std::string s(19, '\0');
        std::strftime(&s[0], s.size(), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
        return s;
    }
}