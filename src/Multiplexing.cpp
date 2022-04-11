#include "Multiplexing.hpp"

namespace we
{






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

        WatchType MultiplixingSelect::is_set(int fd) const
        {
            if (FD_ISSET(fd, &this->_tmp_read_set))
                return Read;
            if (FD_ISSET(fd, &this->_tmp_write_set))
                return Write;
            return None;
        }
#endif
}