#include "Multiplexing.hpp"

namespace we
{
#if HAVE_KQUEUE
    MultiplixingKqueue::MultiplixingKqueue()
    {
        if ((this->_kqueue_fd = kqueue()) < 0)
            throw std::runtime_error("Failed to create the kernel event queue");
    }

    MultiplixingKqueue::~MultiplixingKqueue()
    {
        if (this->_kqueue_fd != -1)
        {
            close(this->_kqueue_fd);
            this->_kqueue_fd = -1;
        }
    }

    void MultiplixingKqueue::add(int fd, Connection* data, WatchType type)
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
        }

        this->_fds_data[fd] = data;
    }

    void MultiplixingKqueue::remove(int fd)
    {
        this->updateEvent(fd, EVFILT_READ, EV_DELETE);
        this->updateEvent(fd, EVFILT_WRITE, EV_DELETE);
    }

    int MultiplixingKqueue::wait()
    {
        this->_next_fd = 0;
        this->_max_fd = kevent(this->_kqueue_fd, NULL, 0, this->_event_list, QUEUE_SIZE, NULL);
        return this->_max_fd;
    }

    int MultiplixingKqueue::wait(long long ms_wait)
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

    int MultiplixingKqueue::get_next_fd()
    {
        while (this->_next_fd < this->_max_fd)
            return this->_event_list[this->_next_fd++].ident;
        return -1;
    }

    Connection *MultiplixingKqueue::get_connection(int fd)
    {
        return this->_fds_data[fd];
    }

    void MultiplixingKqueue::updateEvent(int ident, short filter, u_short flags)
    {
        struct kevent kev;

        EV_SET(&kev, ident, filter, flags, 0, 0, NULL);
        kevent(this->_kqueue_fd, &kev, 1, NULL, 0, NULL);
    }
#endif

#if HAVE_SELECT
    MultiplixingSelect::MultiplixingSelect()
    {
        FD_ZERO(&this->_read_set);
        FD_ZERO(&this->_write_set);
        this->_max_fd = 0;
    }

    MultiplixingSelect::~MultiplixingSelect()
    {
    }

    void MultiplixingSelect::add(int fd, Connection* data, WatchType type)
    {
        // TODO: check if there is enough room for the new fd
        // TODO: check if fd < 0
        assert(type != None && type != Error);
        switch (type)
        {
        case Read:
            FD_SET(fd, &this->_read_set);
            break;
        case Write:
            FD_SET(fd, &this->_write_set);
            break;
        }
        if (fd > this->_max_fd)
            this->_max_fd = fd;
        this->_fds_data[fd] = data;
    }

    void MultiplixingSelect::remove(int fd)
    {
        FD_CLR(fd, &this->_read_set);
        FD_CLR(fd, &this->_write_set);
    }

    int MultiplixingSelect::wait()
    {
        this->_next_fd = 0;
        this->_tmp_read_set = this->_read_set;
        this->_tmp_write_set = this->_write_set;
        return select(this->_max_fd + 1, &this->_tmp_read_set, &this->_tmp_write_set, NULL, NULL);
    }

    int MultiplixingSelect::wait(long long ms_wait)
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

    int MultiplixingSelect::get_next_fd()
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

    Connection *MultiplixingSelect::get_connection(int fd)
    {
        return this->_fds_data[fd];
    }
#endif
}