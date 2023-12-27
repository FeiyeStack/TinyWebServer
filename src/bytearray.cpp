#include "bytearray.h"
#include "endianness.h"
#include <stdexcept>
#include <fstream>
#include <cmath>
#include "log.h"
namespace WebSrv
{
    static Logger::ptr g_logger=SRV_LOGGER_NAME("system" );
    ByteArray::Node::Node(size_t s)
        : pValue(new char[s]), next(nullptr), size(s)
    {
    }

    ByteArray::Node::Node()
        : pValue(nullptr), next(nullptr), size(0)
    {
    }

    ByteArray::Node::~Node()
    {
        if (pValue)
        {
            delete[] pValue;
        }
    }

    ByteArray::ByteArray(size_t baseSize)
        : _baseSize(baseSize), _position(0), _capacity(baseSize), _size(0), _endian(WEB_SRV_BIG_ENDIAN), _root(new Node(baseSize)), _cur(_root)
    {
    }

    ByteArray::~ByteArray()
    {
        while (_root)
        {
            _cur = _root;
            _root = _root->next;
            delete _cur;
        }
    }

    void ByteArray::writeFInt8(int8_t value)
    {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFUint8(uint8_t value)
    {
        write(&value, sizeof(value));
    }

    void ByteArray::writeFInt16(int16_t value)
    {
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFUint16(uint16_t value)
    {
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFInt32(int32_t value)
    {
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFUint32(uint32_t value)
    {
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFInt64(int64_t value)
    {
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    void ByteArray::writeFUint64(uint64_t value)
    {
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            value = byteswap(value);
        }
        write(&value, sizeof(value));
    }

    // 将有符号转为无符号可以提高压缩率
    static uint32_t EncodeZigzag32(const int32_t &v)
    {
        if (v < 0)
        {
            return ((uint32_t)(-v)) * 2 - 1; //-1 负数标识（奇数）
        }
        else
        {
            return v * 2;
        }
    }

    static uint64_t EncodeZigzag64(const int64_t &v)
    {
        if (v < 0)
        {
            return ((uint64_t)(-v)) * 2 - 1; //-1 负数标识（奇数，末位为1）
        }
        else
        {
            return v * 2;
        }
    }

    static int32_t DecodeZigzag32(const uint32_t &v)
    {
        return (v >> 1) ^ -(v & 1); // 负数时，&1为0 异或结果为其反码--负数
    }

    static int64_t DecodeZigzag64(const uint64_t &v)
    {
        return (v >> 1) ^ -(v & 1);
    }

    void ByteArray::writeInt32(int32_t value)
    {
        writeUint32(EncodeZigzag32(value));
    }

    void ByteArray::writeUint32(uint32_t value)
    {
        uint8_t tmp[5];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeInt64(int64_t value)
    {
        writeUint64(EncodeZigzag64(value));
    }

    void ByteArray::writeUint64(uint64_t value)
    {
        uint8_t tmp[10];
        uint8_t i = 0;
        while (value >= 0x80)
        {
            tmp[i++] = (value & 0x7F) | 0x80;
            value >>= 7;
        }
        tmp[i++] = value;
        write(tmp, i);
    }

    void ByteArray::writeDouble(double value)
    {
        uint64_t v;
        memcpy(&v, &value, sizeof(value));
        writeFUint64(v);
    }

    void ByteArray::writeFloat(float value)
    {
        uint32_t v;
        memcpy(&v, &value, sizeof(value));
        writeFUint32(v);
    }

    void ByteArray::writeStringFUInt16(const std::string &value)
    {
        writeFInt16(value.size());
        write(value.c_str(), value.size());
    }

    void ByteArray::writeStringFUInt32(const std::string &value)
    {
        writeFInt32(value.size());
            write(value.c_str(), value.size());
    }

    void ByteArray::writeStringFUInt64(const std::string &value)
    {
        writeFInt64(value.size());
            write(value.c_str(), value.size());
    }

    void ByteArray::writeStringVInt(const std::string &value)
    {
        writeUint64(value.size());
            write(value.c_str(), value.size());
    }

    void ByteArray::writeStringWithoutLength(const std::string &value)
    {
        write(value.c_str(), value.size());
    }

    int8_t ByteArray::readFInt8()
    {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }

    uint8_t ByteArray::readFUint8()
    {
        int8_t v;
        read(&v, sizeof(v));
        return v;
    }

    int16_t ByteArray::readFInt16()
    {
        int16_t v;
        read(&v, sizeof(v));
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            return byteswap(v);
        }
        return v;
    }

    uint16_t ByteArray::readFUint16()
    {
        uint16_t v;
        read(&v, sizeof(v));
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            return byteswap(v);
        }
        return v;
    }

    int32_t ByteArray::readFInt32()
    {
        int32_t v;
        read(&v, sizeof(v));
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            return byteswap(v);
        }
        return v;
    }

    uint64_t ByteArray::readFUint32()
    {
        uint32_t v;
        read(&v, sizeof(v));
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            return byteswap(v);
        }
        return v;
    }

    int64_t ByteArray::readFInt64()
    {
        int64_t v;
        read(&v, sizeof(v));
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            return byteswap(v);
        }
        return v;
    }

    uint64_t ByteArray::readFUint64()
    {
        uint64_t v;
        read(&v, sizeof(v));
        if (_endian != WEB_SRV_BYTE_ORDER)
        {
            return byteswap(v);
        }
        return v;
    }

    int32_t ByteArray::readInt32()
    {
        return DecodeZigzag32(readUint32());
    }

    uint32_t ByteArray::readUint32()
    {
        uint32_t result = 0;
        for (int i = 0; i < 32; i += 7)
        {
            uint8_t b = readFUint8();
            if (b < 0x80)
            {
                result |= ((uint32_t)b) << i;
                break;
            }
            result |= (((uint32_t)(b & 0x7F)) << i);
        }
        return result;
    }

    int64_t ByteArray::readInt64()
    {
        return DecodeZigzag64(readUint64());
    }

    uint64_t ByteArray::readUint64()
    {
        uint64_t result = 0;
        for (int i = 0; i < 64; i += 7)
        {
            uint8_t b = readFUint8();
            if (b < 0x80)
            {
                result |= ((uint32_t)b) << i;
                break;
            }
            result |= (((uint32_t)(b & 0x7F)) << i);
        }
        return result;
    }

    double ByteArray::readDouble()
    {
        uint64_t v = readFUint64();
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    float ByteArray::readFloat()
    {
        uint32_t v = readFUint64();
        double value;
        memcpy(&value, &v, sizeof(v));
        return value;
    }

    std::string ByteArray::readStringFInt16()
    {
        uint16_t len = readFUint16();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringFInt32()
    {
        uint32_t len = readFUint32();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringFInt64()
    {
        uint64_t len = readFUint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    std::string ByteArray::readStringVInt()
    {
        uint64_t len = readUint64();
        std::string buff;
        buff.resize(len);
        read(&buff[0], len);
        return buff;
    }

    void ByteArray::clear()
    {
        _position = _size = 0;
        _capacity = _baseSize;
        Node *temp = _root->next;
        while (temp)
        {
            _cur = temp;
            temp = temp->next;
            delete _cur;
        }
        _cur = _root;
        _root->next = nullptr;
    }

    void ByteArray::write(const void *buff, size_t size)
    {
        if (size == 0)
        {
            return;
        }
        // 判断是否要扩充
        addCapacity(size);

        size_t npos = _position % _baseSize; // 当前区块位置，确保不会越界
        size_t ncap = _cur->size - npos;     // 当前内存块剩余空间
        size_t bpos = 0;                     // 已存入空间大小
        while (size > 0)
        {
            if (ncap >= size)
            { // 大于存储空间时，直接存入当前内存块
                memcpy(_cur->pValue + npos, (const char *)buff + bpos, size);
                // 填充满的话移动到下一块内存空间
                if (_cur->size == npos + size)
                {
                    _cur = _cur->next;
                }
                _position += size;
                bpos += size;
                size = 0;
            }
            else
            { // 需要跨区块存入
                memcpy(_cur->pValue + npos, (const char *)buff + bpos, ncap);
                _position += ncap;
                bpos += ncap;
                size -= ncap;
                _cur = _cur->next;
                ncap = _cur->size;
                npos = 0;
            }
        }
        if (_position > _size)
        {
            _size = _position;
        }
    }

    void ByteArray::read(void *buff, size_t size)
    {
        if (getReadSize() < size)
        {
            throw std::out_of_range("The readable data at the current location is either too long" 
            "or not as much data is readable");
        }
        size_t npos = _position % _baseSize; // 当前区块位置，确保不会越界
        size_t ncap = _cur->size - npos;     // 当前内存块剩余空间
        size_t bpos = 0;                     // 已读取空间大小
        while (size > 0)
        {
            if (ncap >= size)
            { // 大于存储空间时，直接从当前内存块读取
                memcpy((char *)buff + bpos, _cur->pValue + npos, size);
                // 读完的话移动到下一块内存空间
                if (_cur->size == npos + size)
                {
                    _cur = _cur->next;
                }
                _position += size;
                bpos += size;
                size = 0;
            }
            else
            { // 需要跨区块读取
                memcpy((char *)buff + bpos, _cur->pValue + npos, ncap);
                _position += ncap;
                bpos += ncap;
                size -= ncap;
                _cur = _cur->next;
                ncap = _cur->size;
                npos = 0;
            }
        }
    }

    void ByteArray::read(void *buff, size_t size, size_t position) const
    {
        if ((size - position) < size)
        {
            throw std::out_of_range("The readable data at the current location is either too long"
            "or not as much data is readable");
        }
        size_t npos = position % _baseSize; // 当前区块位置，确保不会越界
        size_t ncap = _cur->size - npos;    // 当前内存块剩余空间
        size_t bpos = 0;                    // 已读取空间大小
        size_t count = position / _baseSize; // 距离目标位置的间隔区块
        Node *cur = _root;
        while (count)
        {
            cur = cur->next;
            --count;
        }
        while (size > 0)
        {
            if (ncap >= size)
            { // 大于存储空间时，直接从当前内存块读取
                memcpy((char *)buff + bpos, cur->pValue + npos, size);
                // 读完的话移动到下一块内存空间
                if (cur->size == npos + size)
                {
                    cur = cur->next;
                }
                position += size;
                bpos += size;
                size = 0;
            }
            else
            { // 需要跨区块读取
                memcpy((char *)buff + bpos, cur->pValue + npos, ncap);
                position += ncap;
                bpos += ncap;
                size -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
        }
    }

    void ByteArray::setPosition(size_t pos)
    {
        if (pos > _capacity)
        {
            throw std::out_of_range("Setting position exceeds the current space limit");
        }
        _position = pos;
        if (_position > _size)
        {
            _size = _position;
        }
        // 移动到对应区块
        _cur = _root;
        while (pos >= _cur->size)
        {
            pos -= _cur->size;
            _cur = _cur->next;
        }
    }

    size_t ByteArray::getPosition() const
    {
        return _position;
    }

    bool ByteArray::writeToFile(const std::string &filename)
    {
        std::ofstream ofs;
        ofs.open(filename, std::ios::trunc | std::ios::binary); // 以截断的形式添加（覆盖）
        if (!ofs)
        {
            SRV_LOG_ERROR(g_logger) << __func__ << "(" << filename << ") error, error = " << errno << "errstr=" << strerror(errno);
            return false;
        }
        int64_t readSize = getReadSize();
        int64_t pos = _position;
        Node *cur = _cur;
        while (readSize > 0)
        {
            int distance = pos % _baseSize;
            int64_t len = (readSize > (int64_t)_baseSize ? _baseSize : readSize) - distance;
            ofs.write(cur->pValue + distance, len);
            cur = cur->next;
            pos += len;
            readSize -= len;
        }
        return true;
    }

    bool ByteArray::readFromFile(const std::string &filename)
    {
        std::ifstream ifs;
        ifs.open(filename, std::ios::binary);
        if (!ifs)
        {
            SRV_LOG_ERROR(g_logger) << __func__ << "(" << filename << ") error, error = " << errno << "errstr=" << strerror(errno);
            return false;
        }
        std::shared_ptr<char> buff(new char[_baseSize], [](char *ptr)
                                   { delete[] ptr; });
        while (!ifs.eof())
        {
            ifs.read(buff.get(), _baseSize);
            write(buff.get(), ifs.gcount());
        }
        return true;
    }

    size_t ByteArray::getBaseSize() const
    {
        return _baseSize;
    }

    size_t ByteArray::getReadSize() const
    {
        return _size - _position;
    }

    size_t ByteArray::getSize() const
    {
        return _size;
    }

    bool ByteArray::isLittleEndian() const
    {
        return _endian == WEB_SRV_LITTLE_ENDIAN;
    }

    void ByteArray::setIsLittleEndian(bool val)
    {
        if (val)
        {
            _endian = WEB_SRV_LITTLE_ENDIAN;
        }
        else
        {
            _endian = WEB_SRV_BIG_ENDIAN;
        }
    }

    std::string ByteArray::toString() const
    {
        std::string str;
        str.resize(getReadSize());
        if (str.empty())
        { // 没可读大小
            return str;
        }
        read(&str[0], str.size(),_position);
        return str;
    }

    std::string ByteArray::toHexString() const
    {
        std::string str = toString();
        if (str.empty())
        {
            return str;
        }
        std::stringstream ss;
        for (size_t i = 0; i < str.size(); ++i)
        {
            if (i > 0 && i % 32 == 0)
            {
                ss << std::endl;
            }
            ss << std::setw(2) << std::setfill('0') << std::hex << (int)(uint8_t)str[i] << " ";
        }
        return ss.str();
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
        {
            return 0;
        }
        uint64_t size = len;
        size_t npos = _position % _baseSize; // 当前区块位置，确保不会越界
        size_t ncap = _cur->size - npos;     // 当前内存块剩余空间
        Node *cur = _cur;
        iovec buff;
        while (len > 0)
        {
            if (ncap >= len)
            { // 大于存储空间时，直接从当前内存块获取
                buff.iov_base = cur->pValue + npos;
                buff.iov_len = len;
                len = 0;
            }
            else
            { // 需要跨区块获取
                buff.iov_base = cur->pValue + npos;
                buff.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.emplace_back(buff);
        }
        return size;
    }

    uint64_t ByteArray::getReadBuffers(std::vector<iovec> &buffers, uint64_t len, uint64_t position) const
    {
        len = len > getReadSize() ? getReadSize() : len;
        if (len == 0)
        {
        }
        uint64_t size = len;
        size_t npos = position % _baseSize;  // 当前区块位置，确保不会越界
        size_t count = position / _baseSize; // 距离目标位置的间隔区块
        Node *cur = _root;
        while (count)
        {
            cur = cur->next;
            --count;
        }

        size_t ncap = _cur->size - npos; // 当前内存块剩余空间
        iovec buff;
        while (len > 0)
        {
            if (ncap >= len)
            { // 大于存储空间时，直接从当前内存块获取
                buff.iov_base = cur->pValue + npos;
                buff.iov_len = len;
                len = 0;
            }
            else
            { // 需要跨区块获取
                buff.iov_base = cur->pValue + npos;
                buff.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.emplace_back(buff);
        }
        return size;
        return 0;
    }

    uint64_t ByteArray::getWriteBuffers(std::vector<iovec> &buffers, uint64_t len)
    {
        if (len == 0)
        {
            return 0;
        }
        addCapacity(len);
        uint64_t size = len;
        size_t npos = _position % _baseSize; // 当前区块位置，确保不会越界
        size_t ncap = _cur->size - npos;     // 当前内存块剩余空间
        Node *cur = _cur;
        iovec buff;
        while (len > 0)
        {
            if (ncap >= len)
            { // 大于存储空间时，直接从当前内存块获取
                buff.iov_base = cur->pValue + npos;
                buff.iov_len = len;
                
                len = 0;
            }
            else
            { // 需要跨区块获取
                buff.iov_base = cur->pValue + npos;
                buff.iov_len = ncap;
                len -= ncap;
                cur = cur->next;
                ncap = cur->size;
                npos = 0;
            }
            buffers.emplace_back(buff);
        }
        return size;
    }

    void ByteArray::addCapacity(size_t size)
    {
        if (size == 0)
        {
            return;
        }
        size_t oldCap = getCapacity();
        if (oldCap >= size)
        {
            return;
        }
        size = size - oldCap;
        size_t count = ceil(1.0 * size / _baseSize); // ceil 函数返回不小于输入参数的最小整数(因为是链表，就按需求开辟空间)
        Node *temp = _root;
        // 移动到末尾
        while (temp->next)
        {
            temp = temp->next;
        }
        Node *first = nullptr;
        for (int i = 0; i < count; i++)
        {
            temp->next = new Node(_baseSize);
            if (!first)
            {
                first = temp->next;
            }
            temp = temp->next;
            _capacity += _baseSize;
        }
        // 旧空间满了的话，先行移动下一节点
        if (oldCap == 0)
        {
            _cur = first;
        }
    }

    size_t ByteArray::getCapacity() const
    {
        return _capacity - _position;
    }

} // namespace WebSrv
