#pragma once

namespace we
{
    enum WatchType
    {
        None,
        Read,
        Write,
        Error
    };

    struct IMultiplexing
    {
        virtual void add(int, WatchType) = 0;
        virtual void remove(int) = 0;
        virtual int wait(int) = 0;
        virtual int wait(int, long long) = 0;
        virtual WatchType is_set(int) = 0;
        virtual ~IMultiplexing() {};
    };
}