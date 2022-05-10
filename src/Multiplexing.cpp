#include "Multiplexing.hpp"

namespace we
{
    AMultiplexing::~AMultiplexing()
    {
    }

    BaseConnection *AMultiplexing::get_connection(int fd)
    {
        return this->_fds_data[fd];
    }

#if HAVE_KQUEUE
    MultiplexingKqueue::MultiplexingKqueue()
    {
        if ((this->_kqueue_fd = kqueue()) < 0)
            throw std::runtime_error("Failed to create the kernel event queue");
    }

    MultiplexingKqueue::~MultiplexingKqueue()
    {
        if (this->_kqueue_fd != -1)
        {
            close(this->_kqueue_fd);
            this->_kqueue_fd = -1;
        }
    }

    void MultiplexingKqueue::add(int fd, BaseConnection* data, WatchType type)
    {
        assert(type != None && type != Error);

        if (fd < 0)
            return ;

        switch (type)
        {
        case Read:
            this->updateEvent(fd, EVFILT_READ, EV_ADD | EV_ENABLE);
            break;
        case Write:
            this->updateEvent(fd, EVFILT_WRITE, EV_ADD | EV_ENABLE);
            break;
        default:
            assert(false);
        }

        this->_fds_data[fd] = data;
    }

    void MultiplexingKqueue::remove(int fd)
    {
        this->updateEvent(fd, EVFILT_READ, EV_DELETE);
        this->updateEvent(fd, EVFILT_WRITE, EV_DELETE);
    }

    int MultiplexingKqueue::wait()
    {
        this->_next_fd = 0;
        this->_max_fd = kevent(this->_kqueue_fd, NULL, 0, this->_event_list, QUEUE_SIZE, NULL);
        return this->_max_fd;
    }

    int MultiplexingKqueue::wait(long long ms_wait)
    {
        if (ms_wait == -1)
            return this->wait();

        this->_next_fd = 0;
        struct timespec tv;
        tv.tv_sec = ms_wait / 1000;
        tv.tv_nsec = (ms_wait % 1000) * 1000000;
        this->_max_fd = kevent(this->_kqueue_fd, NULL, 0, this->_event_list, QUEUE_SIZE, &tv);
        return this->_max_fd;
    }

    int MultiplexingKqueue::get_next_fd()
    {
        while (this->_next_fd < this->_max_fd)
            return this->_event_list[this->_next_fd++].ident;
        return -1;
    }

    void MultiplexingKqueue::updateEvent(int ident, short filter, u_short flags)
    {
        struct kevent kev;

        EV_SET(&kev, ident, filter, flags, 0, 0, NULL);
        kevent(this->_kqueue_fd, &kev, 1, NULL, 0, NULL);
    }
#endif

#if HAVE_POLL
    MultiplexingPoll::MultiplexingPoll()
    {
    }

    MultiplexingPoll::~MultiplexingPoll()
    {
    }

    void MultiplexingPoll::add(int fd, BaseConnection* data, WatchType type)
    {
        assert(type != None && type != Error);

        if (fd < 0)
            return;

        struct pollfd tmp;
        memset(&tmp, 0, sizeof(tmp));
        tmp.fd = fd;
        switch (type)
        {
        case Read:
            tmp.events = POLLIN;
            break;
        case Write:
            tmp.events = POLLOUT;
            break;
        default:
            assert(false);
        }
        this->_fd_list.push_back(tmp);
        this->_fds_data[fd] = data;
    }

    void MultiplexingPoll::remove(int fd)
    {
        std::vector<struct pollfd>::iterator it = this->_fd_list.begin();
        while (it != this->_fd_list.end())
        {
            if (it->fd == fd)
                it = this->_fd_list.erase(it);
            else
                ++it;
        }
    }

    int MultiplexingPoll::wait()
    {
        this->_next_fd = 0;
        return poll(this->_fd_list.data(), this->_fd_list.size(), -1);
    }

    /*
    *** If ms_wait is equale to -1 it would mimic the behavior of
    *** wait() "without args" since poll takes int as 3rd argument
    *** and -1 is basically no timeout (If the value of timeout is -1, the poll
    *** blocks indefinitely). For clarification 0 ms_wait means wait for 0 seconds aka
    *** don't wait at all
    */
    int MultiplexingPoll::wait(long long ms_wait)
    {
        this->_next_fd = 0;
        return poll(this->_fd_list.data(), this->_fd_list.size(), ms_wait);
    }

    int MultiplexingPoll::get_next_fd()
    {
        while (this->_next_fd < this->_fd_list.size())
        {
            if (this->_fd_list[this->_next_fd].revents & POLLIN ||
                this->_fd_list[this->_next_fd].revents & POLLOUT)
                return this->_fd_list[this->_next_fd++].fd;
            else
                this->_next_fd++;
        }
        return -1;
    }
#endif

#if HAVE_SELECT
    MultiplexingSelect::MultiplexingSelect()
    {
        FD_ZERO(&this->_read_set);
        FD_ZERO(&this->_write_set);
        this->_max_fd = 10;
    }

    MultiplexingSelect::~MultiplexingSelect()
    {
    }

    void MultiplexingSelect::add(int fd, BaseConnection* data, WatchType type)
    {
        assert(type != None && type != Error);
        switch (type)
        {
        case Read:
            FD_SET(fd, &this->_read_set);
            break;
        case Write:
            FD_SET(fd, &this->_write_set);
            break;
        default:
            assert(false);
        }
        if (fd > this->_max_fd)
            this->_max_fd = fd;
        this->_fds_data[fd] = data;
    }

    void MultiplexingSelect::remove(int fd)
    {
        FD_CLR(fd, &this->_read_set);
        FD_CLR(fd, &this->_write_set);
    }

    int MultiplexingSelect::wait()
    {
        this->_next_fd = 0;
        this->_tmp_read_set = this->_read_set;
        this->_tmp_write_set = this->_write_set;
        return select(this->_max_fd + 1, &this->_tmp_read_set, &this->_tmp_write_set, NULL, NULL);
    }

    int MultiplexingSelect::wait(long long ms_wait)
    {
        if (ms_wait == -1)
            return this->wait();

        this->_next_fd = 0;
        this->_tmp_read_set = this->_read_set;
        this->_tmp_write_set = this->_write_set;
        struct timeval tv;
        tv.tv_sec = ms_wait / 1000;
        tv.tv_usec = (ms_wait % 1000) * 1000;

        return select(this->_max_fd + 1, &this->_tmp_read_set, &this->_tmp_write_set, NULL, &tv);
    }

    int MultiplexingSelect::get_next_fd()
    {
        while (this->_next_fd <= this->_max_fd)
        {
            if (FD_ISSET(this->_next_fd, &this->_tmp_read_set) || FD_ISSET(this->_next_fd, &this->_tmp_write_set))
                return this->_next_fd++;
            else
                this->_next_fd++;
        }
        return -1;
    }
#endif

    std::string get_instance_name()
    {
        #if defined(HAVE_EPOLL)
            return "epoll";
        #elif defined(HAVE_KQUEUE)
            return "kqueue";
        #elif defined(HAVE_POLL)
            return "poll";
        #elif defined(HAVE_SELECT)
            return "select";
        #else
            return "unknown";
        #endif
    }

    AMultiplexing *get_instance(int type)
    {
        switch (type)
        {
        case Config::MulKqueue:
        #if defined(HAVE_KQUEUE)
            return new MultiplexingKqueue();
        #else
            break;
        #endif
        case Config::MulPoll:
        #if defined(HAVE_POLL)
            return new MultiplexingPoll();
        #else
            break;
        #endif
        case Config::MulEpoll:
        #if defined(HAVE_EPOLL)
            return new MultiplexingEpoll();
        #else
            break;
        #endif
        case Config::MulSelect:
        #if defined(HAVE_SELECT)
            return new MultiplexingSelect();
        #else
            break;
        #endif
        default:
            break;
        }

    if (type == Config::MulNone)
        std::cerr << "no 'use_events' option specified in config file, defaulting to " << get_instance_name() << std::endl;
    else
        std::cerr << "unsupported 'use_events' option specified in config file, defaulting to " << get_instance_name() << std::endl;

    #if defined(HAVE_EPOLL)
        return new MultiplexingEpoll();
    #elif defined(HAVE_KQUEUE)
        return new MultiplexingKqueue();
    #elif defined(HAVE_EPOLL)
        return new MultiplexingEpoll();
    #elif defined(HAVE_POLL)
        return new MultiplexingPoll();
    #elif defined(HAVE_SELECT)
        return new MultiplexingSelect();
    #else
        return NULL;
    #endif
    }
}