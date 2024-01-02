#pragma once
#include "bytearray.h"
namespace WebSrv
{
    class Stream
    {
    public:
        using ptr = std::shared_ptr<Stream>;

        virtual ~Stream() = default;
        /**
         * @brief 读数据
         *
         * @param buffer 接收数据的内存
         * @param len 数据长度
         * @return int 成功返回0
         */
        virtual int read(void *buffer, size_t len) = 0;
        /**
         * @brief 读数据
         *
         * @param ba 接收数据的ByteArray
         * @param len 数据长度
         * @return int
         * @retval >0 接收数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         */
        virtual int read(ByteArray::ptr ba, size_t len) = 0;
        /**
         * @brief 读固定长度的数据
         *
         * @param buffer 接收数据的内存
         * @param len 数据长度
         *          @return int
         * @retval >0 接收数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         */
        virtual int readFixSize(void *buffer, size_t len);

        /**
         * @brief 读固定长度的数据
         *
         * @param ba 接收数据的ByteArray
         * @param len 数据长度
         * @return int
         * @retval >0 接收数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         */
        virtual int readFixSize(ByteArray::ptr ba, size_t len);

        /**
         * @brief 写数据
         *
         * @param buffer 发送数据的内存
         * @param len 发送数据的长度
         * @return int
         * @retval >0 发生数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         *
         */
        virtual int write(const void *buffer, size_t len) = 0;
        /**
         * @brief 写数据
         *
         * @param ba 发送数据的ByteArray
         * @param len 发送数据的长度
         * @return int
         * @retval >0 发生数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         */
        virtual int write(ByteArray::ptr ba, size_t len) = 0;

        /**
         * @brief 写固定长度的数据
         *
         * @param buffer 发送数据的内存
         * @param len 发送数据的长度
         * @return int
         * @retval >0 发生数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         */
        virtual int writeFixSize(const void *buffer, size_t len);
        /**
         * @brief 写固定长度的数据
         *
         * @param ba 发送数据的ByteArray
         * @param len 发送数据的长度
         * @return int
         * @retval >0 发生数据长度
         * @retval =0 流关闭
         * @retval <0 流错误
         */
        virtual int writeFixSize(ByteArray::ptr ba, size_t len);

        /**
         * @brief 关闭socket
         */
        virtual void close() = 0;
    };
} // namespace WebSrv