#pragma once
#include <assert.h>
#include <map>
#include <vector>
#include "Connection.hpp"
#include <sys/time.h>
namespace we
{
    class Connection;

    enum WatchType
    {
        None,
        Read,
        Write,
        Error
    };

    class IMultiplexing
    {
    protected:
        std::map<int, Connection *> _fds_data;
    public:
        virtual void add(int, Connection*, WatchType) = 0;
        virtual void remove(int) = 0;
        virtual int wait() = 0 ;
        virtual int wait(long long) = 0;
        virtual int get_next_fd() = 0;
        virtual WatchType is_set(int) const = 0;
        virtual ~IMultiplexing() {};
    };

#define HAVE_SELECT 1
#define HAVE_KQUEUE 1
#define HAVE_EPOLL 1

#if HAVE_KQUEUE

    class MultiplixingKqueue : public IMultiplexing
    {
    public:
        MultiplixingKqueue();
        ~MultiplixingKqueue();

        void add(int, Connection*, WatchType);
        void remove(int);
        int wait();
        int wait(long long);
        WatchType is_set(int);
    };
#endif


#if HAVE_SELECT
    #include <sys/select.h>

    class MultiplixingSelect: public IMultiplexing
    {
        fd_set  _read_set;
        fd_set  _write_set;
        fd_set  _tmp_read_set;
        fd_set  _tmp_write_set;
        int     _max_fd;
        int     _next_fd;
        std::vector<int> _active_fds;
    public:
        MultiplixingSelect();
        ~MultiplixingSelect();

        void add(int, Connection*, WatchType);
        void remove(int);
        int wait();
        int wait(long long);
        int get_next_fd();
        WatchType is_set(int) const;
    };
#endif

}