#pragma once
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <vector>
#include <map>

#include "Connection.hpp"

#define HAVE_SELECT 1
#define HAVE_KQUEUE 1
#define HAVE_POLL 1

#if HAVE_KQUEUE
    #include <sys/types.h>
    #include <sys/event.h>

    #define QUEUE_SIZE 1024
#endif

#if HAVE_POLL
    #include <poll.h>
#endif

#if HAVE_SELECT
    #include <sys/select.h>
#endif

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

#if HAVE_KQUEUE
    class MultiplixingKqueue: public IMultiplexing
    {
        int             _max_fd; // nbr of fd in the kqueue event list
        int             _next_fd; // cursor on the kqueue event list
        int             _kqueue_fd; // Kernel event queue file descriptor
        struct kevent   _event_list[QUEUE_SIZE]; // Events that have triggered a filter in the kqueue (max QUEUE_SIZE at a time)

        void updateEvent(int ident, short filter, u_short flags);

    public:
        MultiplixingKqueue();
        ~MultiplixingKqueue();

        void add(int, Connection*, WatchType);
        void remove(int);
        int wait();
        int wait(long long);
        int get_next_fd();
        WatchType is_set(int) const;
    };
#endif

#if HAVE_POLL
    class MultiplixingPoll: public IMultiplexing
    {
    public:
        MultiplixingPoll();
        ~MultiplixingPoll();

        void add(int, Connection*, WatchType);
        void remove(int);
        int wait();
        int wait(long long);
        int get_next_fd();
        WatchType is_set(int) const;
    };
#endif

#if HAVE_SELECT
    class MultiplixingSelect: public IMultiplexing
    {
        fd_set  _read_set;
        fd_set  _write_set;
        fd_set  _tmp_read_set;
        fd_set  _tmp_write_set;
        int     _max_fd;
        int     _next_fd;

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