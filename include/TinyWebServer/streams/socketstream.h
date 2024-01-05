#pragma once

#include "stream.h"
#include "socket.h"
namespace WebSrv
{
    /**
     * @brief socket流
     *
     */
    class SocketStream : public Stream
    {
    public:
        using ptr = std::shared_ptr<SocketStream>;
        /**
         * @brief Construct a new Socket Stream object
         *
         * @param sock
         * @param owner 是否主控
         */
        SocketStream(Socket::ptr socket, bool owner = true);

        ~SocketStream();

        /**
         * @brief 读数据
         *
         * @param buffer 接收数据的内存
         * @param len 数据长度
         * @return int 成功返回0
         */
        int read(void *buffer, size_t len) override;
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
        int read(ByteArray::ptr ba, size_t len) override;

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
        int write(const void *buffer, size_t len) override;
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
        int write(ByteArray::ptr ba, size_t len) override;

        /**
         * @brief 关闭socket
         */
        void close() override;
        /**
         * @brief 是否连接
         *
         * @return true
         * @return false
         */
        bool isConnected() const;
        /**
         * @brief 远程地址
         *
         * @return Address::ptr
         */
        Address::ptr getRemoteAddress();
        /**
         * @brief 本地地址
         *
         * @return Address::ptr
         */
        Address::ptr getLocalAddress();
        /**
         * @brief 远程地址字符串类型
         *
         * @return std::string
         */
        std::string getRemoteAddressString();
        /**
         * @brief 本地地址字符串类型
         *
         * @return std::string
         */
        std::string getLocalAddressString();
        /**
         * @brief 返回错误码
         *
         * @return int
         */
        int getError() const;

        /**
         * @brief 返回Socket类
         */
        Socket::ptr getSocket() const { return _socket; }

    protected:
        Socket::ptr _socket;
        /// @brief 是否主控
        bool _owner;
    };
} // namespace WebSrv
