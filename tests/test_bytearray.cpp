#include "TinyWebServer/bytearray.h"
#include "TinyWebServer/log.h"
#include "TinyWebServer/util.h"
#include "TinyWebServer/macro.h"
#include <iostream>
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");
void test()
{
#define xx(type, len, writeFun, readFun, baseLen)                       \
    {                                                                   \
        std::vector<type> vec;                                          \
        for (int i = 0; i < len; ++i)                                   \
        {                                                               \
            vec.emplace_back(rand());                                   \
        }                                                               \
        WebSrv::ByteArray::ptr byteArr(new WebSrv::ByteArray(baseLen)); \
        for (auto &&i : vec)                                            \
        {                                                               \
            byteArr->writeFun(i);                                       \
        }                                                               \
        byteArr->setPosition(0);                                        \
        for (size_t i = 0; i < vec.size(); ++i)                         \
        {                                                               \
            type value = byteArr->readFun();                            \
            if (value != vec[i])                                        \
            {                                                           \
                SRV_LOG_DEBUG(g_logger) << "data exception";            \
                return;                                                 \
            }                                                           \
        }                                                               \
        if (byteArr->getReadSize() != 0)                                \
        {                                                               \
            SRV_LOG_DEBUG(g_logger) << "byteArr incomplete";            \
            return;                                                     \
        }                                                               \
        SRV_LOG_DEBUG(g_logger) << #writeFun "/" #readFun               \
                                             "(" #type ") len="         \
                                << len                                  \
                                << " baseLen=" << baseLen               \
                                << " size=" << byteArr->getSize();      \
    }
    xx(int8_t, 100, writeFInt8, readFInt8, 1);
    xx(uint8_t, 100, writeFUint8, readFUint8, 1);
    xx(int16_t, 100, writeFInt16, readFInt16, 1);
    xx(uint16_t, 100, writeFUint16, readFUint16, 1);
    xx(int32_t, 100, writeFInt32, readFInt32, 1);
    xx(uint32_t, 100, writeFUint32, readFUint32, 1);
    xx(int64_t, 100, writeFInt64, readFInt64, 1);
    xx(int64_t, 100, writeFUint64, readFUint64, 1);

    xx(int32_t, 100, writeInt32, readInt32, 1);
    xx(uint32_t, 100, writeUint32, readUint32, 1);
    xx(int64_t, 100, writeInt64, readInt64, 1);
    xx(int64_t, 100, writeUint64, readUint64, 1);
#undef xx

#define xx(type, len, writeFun, readFun, baseLen)                                                \
    {                                                                                            \
        std::vector<type> vec;                                                                   \
        for (int i = 0; i < len; ++i)                                                            \
        {                                                                                        \
            vec.emplace_back(rand());                                                            \
        }                                                                                        \
        WebSrv::ByteArray::ptr byteArr(new WebSrv::ByteArray(baseLen));                          \
        for (auto &&i : vec)                                                                     \
        {                                                                                        \
            byteArr->writeFun(i);                                                                \
        }                                                                                        \
        byteArr->setPosition(0);                                                                 \
        for (size_t i = 0; i < vec.size(); ++i)                                                  \
        {                                                                                        \
            type value = byteArr->readFun();                                                     \
            if (value != vec[i])                                                                 \
            {                                                                                    \
                SRV_LOG_DEBUG(g_logger) << "data exception";                                     \
                return;                                                                          \
            }                                                                                    \
        }                                                                                        \
        if (byteArr->getReadSize() != 0)                                                         \
        {                                                                                        \
            SRV_LOG_DEBUG(g_logger) << "byteArr byteArr incomplete";                             \
            return;                                                                              \
        }                                                                                        \
        SRV_LOG_DEBUG(g_logger) << #writeFun "/" #readFun                                        \
                                             "(" #type ") len="                                  \
                                << len                                                           \
                                << " baseLen=" << baseLen                                        \
                                << " size=" << byteArr->getSize();                               \
        byteArr->setPosition(0);                                                                 \
        bool ret = byteArr->writeToFile("./tmp" #type "-" #len "-" #readFun ".dat");             \
        if (!ret)                                                                                \
        {                                                                                        \
            SRV_LOG_DEBUG(g_logger) << "byteArr write to file fail";                             \
            return;                                                                              \
        }                                                                                        \
        WebSrv::ByteArray::ptr byteArr2(new WebSrv::ByteArray(baseLen));                         \
        ret = byteArr2->readFromFile("./tmp" #type "-" #len "-" #readFun ".dat");                \
        if (!ret)                                                                                \
        {                                                                                        \
            SRV_LOG_DEBUG(g_logger) << "byteArr2 read to file fail";                             \
            return;                                                                              \
        }                                                                                        \
        byteArr2->setPosition(0);                                                                \
        WebSrvAssert2(byteArr2->toString() == byteArr->toString(), "write or read to file fail"); \
        if (byteArr2->getPosition() != 0 || byteArr->getPosition() != 0)                         \
        {                                                                                        \
            SRV_LOG_DEBUG(g_logger) << "byteArr2 or byteArr1 incorrect";                         \
            return;                                                                              \
        }                                                                                        \
    }
    xx(int8_t, 100, writeFInt8, readFInt8, 1);
    xx(uint8_t, 100, writeFUint8, readFUint8, 1);
    xx(int16_t, 100, writeFInt16, readFInt16, 1);
    xx(uint16_t, 100, writeFUint16, readFUint16, 1);
    xx(int32_t, 100, writeFInt32, readFInt32, 1);
    xx(uint32_t, 100, writeFUint32, readFUint32, 1);
    xx(int64_t, 100, writeFInt64, readFInt64, 1);
    xx(int64_t, 100, writeFUint64, readFUint64, 1);

    xx(int32_t, 100, writeInt32, readInt32, 1);
    xx(uint32_t, 100, writeUint32, readUint32, 1);
    xx(int64_t, 100, writeInt64, readInt64, 1);
    xx(int64_t, 100, writeUint64, readUint64, 1);
#undef xx
}
#include <util.h>
int main()
{
    test();
}