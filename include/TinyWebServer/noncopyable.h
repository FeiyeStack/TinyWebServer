#pragma once

namespace WebSrv
{
    // 禁用拷贝、赋值
    class NonCopyable
    {
    public:
        NonCopyable() = default;
        ~NonCopyable() = default;
        NonCopyable(NonCopyable &&) = default;
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable &operator=(const NonCopyable &) = delete;
        NonCopyable &operator=(NonCopyable &&) = default;

    };
} // namespace WebSrv
