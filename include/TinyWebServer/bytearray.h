#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <sys/uio.h>

namespace WebSrv
{

    class ByteArray
    {
    public:
        using ptr = std::shared_ptr<ByteArray>;
        /**
         * @brief ByteArray 储存节点
         *
         */
        struct Node
        {
            /**
             * @brief 指定内存块大小构造
             *
             * @param s 内存块字节数
             */
            Node(size_t s);

            Node();

            ~Node();
            // 当前内存块指针
            char *pValue;
            // 下一块内存块地址
            Node *next;
            // 内存块大小
            size_t size;
        };

        /**
         * @brief 指定每个构建的节点内存块大小，构造ByteArray数组
         *
         * @param baseSize 内存块大小
         */
        ByteArray(size_t baseSize = 4096);

        ~ByteArray();

        /**
         * @brief 写入固定长度int8_t类型
         *
         * @param value
         */
        void writeFInt8(int8_t value);
        /**
         * @brief 写入固定长度uint8_t类型
         *
         * @param value
         */
        void writeFUint8(uint8_t value);
        /**
         * @brief 写入固定长度int16_t类型
         *
         * @param value
         */
        void writeFInt16(int16_t value);
        /**
         * @brief 写入固定长度uint16_t类型
         *
         * @param value
         */
        void writeFUint16(uint16_t value);
        /**
         * @brief 写入固定长度int32_t类型
         *
         * @param value
         */
        void writeFInt32(int32_t value);
        /**
         * @brief 写入固定长度uint32_t类型
         *
         * @param value
         */
        void writeFUint32(uint32_t value);
        /**
         * @brief 写入固定长度int64_t类型
         *
         * @param value
         */
        void writeFInt64(int64_t value);
        /**
         * @brief 写入固定长度uint64_t类型
         *
         * @param value
         */
        void writeFUint64(uint64_t value);

        // 非固定长度是可变长度会压缩字节

        /**
         * @brief 写入int32_t类型
         *
         * @param value
         */
        void writeInt32(int32_t value);
        /**
         * @brief 写入uint32_t类型
         *
         * @param value
         */
        void writeUint32(uint32_t value);
        /**
         * @brief 写入int64_t类型
         *
         * @param value
         */
        void writeInt64(int64_t value);
        /**
         * @brief 写入uint64_t类型
         *
         * @param value
         */
        void writeUint64(uint64_t value);
        /**
         * @brief 写入double类型
         *
         * @param value
         */
        void writeDouble(double value);
        /**
         * @brief 写入float类型
         *
         * @param value
         */
        void writeFloat(float value);
        /**
         * @brief 写入std::string 类型，用于uint16_t作为长度类型
         *
         * @param value
         */
        void writeStringFUInt16(const std::string &value);
        /**
         * @brief 写入std::string 类型，用于uint32_t作为长度类型
         *
         * @param value
         */
        void writeStringFUInt32(const std::string &value);
        /**
         * @brief 写入std::string 类型，用于uint64_t作为长度类型
         *
         * @param value
         */
        void writeStringFUInt64(const std::string &value);
        /**
         * @brief 写入std::string 类型，用于VarInt64_t(可变长度)作为长度类型
         *
         * @param value
         */
        void writeStringVInt(const std::string &value);
        /**
         * @brief 写入std::string 类型，无长度
         *
         */
        void writeStringWithoutLength(const std::string &value);

        /**
         * @brief 读取固定长度int8_t类型
         *
         * @return int8_t
         */
        int8_t readFInt8();
        /**
         * @brief 读取固定长度uint8_t类型
         *
         * @return uint8_t
         */
        uint8_t readFUint8();
        /**
         * @brief 读取固定长度int16_t类型
         *
         * @return int16_t
         */
        int16_t readFInt16();
        /**
         * @brief 读取固定长度uint16_t类型
         *
         * @return uint16_t
         */
        uint16_t readFUint16();

        /**
         * @brief 读取固定长度int32_t类型
         *
         * @return int32_t
         */
        int32_t readFInt32();
        /**
         * @brief 读取固定长度uint32_t类型
         *
         * @return uint64_t
         */
        uint64_t readFUint32();
        /**
         * @brief 读取固定长度int64_t类型
         *
         * @return int64_t
         */
        int64_t readFInt64();
        /**
         * @brief 读取固定长度uint64_t类型
         *
         * @return uint64_t
         */
        uint64_t readFUint64();

        // 非固定长度是可变长度(Variable-Length)会压缩字节

        /**
         * @brief 读取VarInt32_t类型
         *
         * @return int32_t
         */
        int32_t readInt32();
        /**
         * @brief 读取VarUint32_t类型
         *
         * @return uint32_t
         */
        uint32_t readUint32();
        /**
         * @brief 读取VarInt64_t类型
         *
         * @return int64_t
         */
        int64_t readInt64();
        /**
         * @brief 读取VarUint64_t类型
         *
         * @return uint64_t
         */
        uint64_t readUint64();
        /**
         * @brief 读取double类型
         *
         * @return double
         */
        double readDouble();
        /**
         * @brief 读取float类型
         *
         * @return float
         */
        float readFloat();
        /**
         * @brief 读取std::string 类型，用于uint16_t作为长度类型
         *
         * @return std::string
         */
        std::string readStringFInt16();
        /**
         * @brief 读取std::string 类型，用于uint32_t作为长度类型
         *
         * @return std::string
         */
        std::string readStringFInt32();
        /**
         * @brief 读取std::string 类型，用于uint64_t作为长度类型
         *
         * @return std::string
         */
        std::string readStringFInt64();
        /**
         * @brief 读取std::string 类型，用于VarInt64_t(可变长度)作为长度类型
         *
         * @return std::string
         */
        std::string readStringVInt();
        /**
         * @brief 清空数据
         *
         */
        void clear();

        /**
         * @brief 写入size长度的数据
         *
         * @param buff
         * @param size
         */
        void write(const void *buff, size_t size);

        /**
         * @brief 读取size长度的数据
         *
         * @param buff
         * @param size
         * @throw getpPosition() <size 说明可读数据不够长，抛出 std::out_of_range
         */
        void read(void *buff, size_t size);
        /**
         * @brief 从指定位置读取数据(不会改变之前读过的位置)
         *
         * @param buff
         * @param size
         * @param position
         * @throw (_size- position)<size 说明可读数据不够长，抛出 std::out_of_range
         */
        void read(void *buff, size_t size, size_t position) const; 
        /**
         * @brief 设置byteArray当前位置
         *
         * @param pos
         * @throw _position 不能大于容器大小 _capacity 否则抛出 std::out_of_range
         */
        void setPosition(size_t pos);

        /**
         * @brief 返回当前的位置
         *
         * @return size_t
         */
        size_t getPosition() const;
        /**
         * @brief 将ByteArray数据写入文件中
         *
         * @param filename
         * @return true
         * @return false
         */
        bool writeToFile(const std::string &filename);
        /**
         * @brief 从文件读取数据
         *
         * @param filename
         * @return true
         * @return false
         */
        bool readFromFile(const std::string &filename);
        /**
         * @brief 当前内存块大小
         *
         * @return size_t
         */
        size_t getBaseSize() const;
        /**
         * @brief 当前可读大小
         *
         * @return size_t
         */
        size_t getReadSize() const;
        /**
         * @brief 当前数据长度
         *
         * @return size_t
         */
        size_t getSize() const;
        /**
         * @brief 储存的数据是否是小端
         *
         * @return true
         * @return false
         */
        bool isLittleEndian() const;
        /**
         * @brief 设置是否为小端数据（存入数据前使用）
         *
         * @param val
         */
        void setIsLittleEndian(bool val);
        /**
         * @brief 将当前位置到末尾的数据转换成std::string
         *
         * @return std::string
         */
        std::string toString() const;
        /**
         * @brief 将当前位置到末尾的数据转换成16进制的std::string
         *
         * @return std::string
         */
        std::string toHexString() const;
        /**
         * @brief 获取可读取的缓存,将数据保存到WSABUF数组中（从当前位置开始）（缓存内存区块）
         *
         * @param buffers
         * @param len 读取数据长度，超过可读取长度时，等于可读取长度
         * @return uint64_t 读取大小
         */
        uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len = ~0ull) const;
        /**
         * @brief 获取可读取的缓存，将数据保存到WSABUF数组中，从指定位置开始（缓存内存区块）
         *
         * @param buffers
         * @param len 读取数据长度，超过可读取长度时，等于可读取长度
         * @return uint64_t 读取大小
         */
        uint64_t getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const;

        /**
         * @brief 获取可写入缓存，将数据保存到WSABUF数组中 （缓存内存区块）
         *
         * @param buffers
         * @param len
         * @return uint64_t
         */
        uint64_t getWriteBuffers(std::vector<iovec> &buffers, uint64_t len);

    private:
        /**
         * @brief 扩容ByteArray大小（达到上限时)
         *
         */
        void addCapacity(size_t size);
        /**
         * @brief 当前可写入大小
         *
         * @return size_t
         */
        size_t getCapacity() const;

    private:
        // 内存块大小
        size_t _baseSize;
        // 当前操作位置
        size_t _position; // 操作位置大于_size时操作为尾部=_size
        // 当前的容器总量
        size_t _capacity;
        // 当前数据大小
        size_t _size;
        // 容器数据字节序,默认大端
        int8_t _endian;
        // 第一个内存块指针
        Node *_root;
        // 当前操作的内存块指针
        Node *_cur;
    };
} // namespace WebSrv
