
#include "Chunker.hpp"

namespace we
{
    ChunckReader::ChunckReader(std::fstream &f, size_t buffer_size) : file(f), data_buffer_size(buffer_size)
    {
        data_buffer.reserve(data_buffer_size);
        wait_for_size = true;
        need_digit = true;
        skip_crlf = false;
        skip_lf = false;
        skip_to_crlf = false;
        skip_to_lf = false;
        file_size = 0;
    }

    bool ChunckReader::add_to_chunk(const char *&buffer, const char *end)
    {
        while (buffer != end)
        {
            if (!wait_for_size && need_digit)
                throw we::HTTPStatusException(400, "Bad request");
            if (wait_for_size)
                add_to_chunksize(buffer, end);
            else if (next_chunk_size > 0)
                add_to_data_buffer(buffer, end);
            else if (trailer_part_ended(buffer, end))
                return true;
        }

        return false;
    }

    bool ChunckReader::trailer_part_ended(const char *&buffer, const char *end)
    {
        data_buffer.append(buffer, end);
        buffer = end;
        std::string::iterator it = data_buffer.begin();

        for (; it < data_buffer.end(); it++)
        {
            if (*it == '\r' && *(it + 1) == '\n' && *(it + 2) == '\r' && *(it + 3) == '\n')
            {
                data_buffer.assign(it + 4, data_buffer.end());
                return true;
            }
            if (*it == '\r' && *(it + 1) == '\n' && *(it + 2) == '\n')
            {
                data_buffer.assign(it + 3, data_buffer.end());
                return true;
            }
            if (*it == '\n' && *(it + 1) == '\r' && *(it + 2) == '\n')
            {
                data_buffer.assign(it + 3, data_buffer.end());
                return true;
            }
            if (*it == '\n' && *(it + 1) == '\n')
            {
                data_buffer.assign(it + 2, data_buffer.end());
                return true;
            }
        }
        if (data_buffer.size() > 3)
            data_buffer.assign(data_buffer.end() - 3, data_buffer.end());
        return false;
    }

    std::string ChunckReader::get_leftover_data()
    {
        std::string ret = data_buffer;
        data_buffer.clear();
        return ret;
    }

    void ChunckReader::add_to_data_buffer(const char *&buffer, const char *end)
    {
        size_t to_copy = std::min(next_chunk_size, static_cast<size_t>(end - buffer));
        data_buffer.append(buffer, to_copy);
        buffer += to_copy;
        next_chunk_size -= to_copy;

        if (next_chunk_size == 0)
        {
            skip_crlf = true;
            wait_for_size = true;
            need_digit = true;
        }
        if (next_chunk_size == 0 || data_buffer.size() >= data_buffer_size)
        {
            this->file_size += data_buffer.size();
            file.write(data_buffer.c_str(), data_buffer.size());
            data_buffer.clear();
        }
    }

    void ChunckReader::add_to_chunksize(const char *&buffer, const char *end)
    {
        /*
            this is nonesence, but we need to skip the CRLF after the reading a chunk
        */
        if (skip_crlf && *buffer == '\r')
        {
            buffer++;
            skip_crlf = false;
            skip_lf = true;
        }
        if (buffer == end)
            return;
        if ((skip_crlf || skip_lf) && *buffer == '\n')
        {
            buffer++;
            skip_lf = false;
            skip_crlf = false;
        }
        if (skip_crlf || skip_lf)
            throw we::HTTPStatusException(400, "Bad request");
        const char *newline = reinterpret_cast<const char *>(memchr(buffer, '\n', end - buffer));
        if (!newline && !skip_to_lf)
        {
            if (!skip_to_crlf)
                this->get_next_chunked_size(buffer, end);
            // change the buffer to the end of the buffer
            buffer = end;
        }
        else if (skip_to_lf)
        {
            if (newline)
            {
                buffer = newline + 1;
                skip_to_lf = false;
                skip_to_crlf = false;
                wait_for_size = false;
            }
            else
                throw we::HTTPStatusException(400, "Bad request");
        }
        else
        {
            wait_for_size = false;
            if (!skip_to_crlf)
                this->get_next_chunked_size(buffer, newline);
            skip_to_crlf = false;
            // change buffer to point to the next line
            buffer = newline + 1;
            if (next_chunk_size == 0)
                data_buffer = "\n";
        }
    }

    void ChunckReader::get_next_chunked_size(const char *it, const char *end)
    {
        while (it != end && *it != ';' && *it != '\r')
        {
            if (!isxdigit(*it))
                throw we::HTTPStatusException(400, "Bad request");
            need_digit = false;
            this->next_chunk_size *= 16;
            this->next_chunk_size += char_to_hex(*it);
            if (next_chunk_size > max_chunk_size)
                throw we::HTTPStatusException(400, "Bad request");
            it++;
        }
        if (it != end && *it == '\r')
            skip_to_lf = true;
        else if (it != end)
            skip_to_crlf = true;
    }

    size_t  ChunckReader::get_file_size() const
    {
        return this->file_size;
    }

    ChunckReader::~ChunckReader() {}
} // namespace we
