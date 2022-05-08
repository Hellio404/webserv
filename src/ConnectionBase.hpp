#pragma once

namespace we
{
    class BaseConnection
    {
    public:
        virtual ~BaseConnection();
        virtual void handle_connection() = 0;
    };
}
