#include "stream.h"

namespace WebSrv
{
    int Stream::readFixSize(void *buffer, size_t len)
    {
        size_t offset = 0;
        size_t left = len;
        while (left > 0)
        {
            int length = read((char *)buffer + offset, left);
            if (length <= 0)
            {
                return length;
            }
            offset += length;
            left -= length;
        }
        return len;
    }

    int Stream::readFixSize(ByteArray::ptr ba, size_t len)
    {
        size_t left = len;
        while (left > 0)
        {
            int length = read(ba, left);
            if (length <= 0)
            {
                return length;
            }
            left -= length;
        }
        return len;
    }

    int Stream::writeFixSize(const void *buffer, size_t len)
    {
        size_t offset = 0;
        size_t left = len;
        while (left > 0)
        {
            int length = write((const char *)buffer + offset, left);
            if (length <= 0)
            {
                return length;
            }
            offset += length;
            left -= length;
        }
        return len;
    }
    int Stream::writeFixSize(ByteArray::ptr ba, size_t len)
    {
        size_t left = len;
        while (left > 0)
        {
            int length = write(ba, left);
            if (length <= 0)
            {
                return length;
            }
            left -= length;
        }
        return len;
    }
} // namespace name
