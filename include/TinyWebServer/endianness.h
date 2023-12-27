#pragma once

// 小端字节序标识
#define WEB_SRV_LITTLE_ENDIAN 1
// 大端字节序标识
#define WEB_SRV_BIG_ENDIAN 2

#include <cstdint>
#include <byteswap.h>
#include <type_traits>

namespace WebSrv
{

    /**
     * @brief 8字节类型的字节序转换
     *
     * @tparam T
     * @param value
     * @return std::enable_if<sizeof(T)==sizeof(uint64_t),T>::type
     */
    template <typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint64_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_64((uint64_t)value);
    }
    /**
     * @brief 4字节类型的字节序转换
     *
     * @tparam T
     * @param value
     * @return std::enable_if<sizeof(T)==sizeof(uint32_t),T>::type
     */
    template <typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint32_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_32((uint32_t)value);
    }
    /**
     * @brief 2字节类型的字节序转换
     *
     * @tparam T
     * @param value
     * @return std::enable_if<sizeof(T)==sizeof(uint16_t),T>::type
     */
    template <typename T>
    typename std::enable_if<sizeof(T) == sizeof(uint16_t), T>::type
    byteswap(T value)
    {
        return (T)bswap_16((uint16_t)value);
    }

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define WEB_SRV_BYTE_ORDER WEB_SRV_LITTLE_ENDIAN
#else
#define WEB_SRV_BYTE_ORDER WEB_SRV_BIG_ENDIAN
#endif
} // namespace WebSrv
