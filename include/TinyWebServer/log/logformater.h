#pragma once
#include <memory.h>
#include <memory>
#include <vector>
#include <string>
#include "logevent.h"
namespace WebSrv
{
     /**
     * @brief 日志格式器
     *
     * 将日志事件字符流格式化，输出对应格式字符流
     */
    class LogFormater
    {
    public:
        using ptr = std::shared_ptr<LogFormater>;

        /**
         * @brief Construct a new Log Format object
         * %m 消息
         * %p 日志级别
         * %r 启动累计毫秒数  需要初始化开始时间INIT_START_TIME()
         * %c 日志名称
         * %t 线程id
         * %F 协程id
         * %n 换行
         * %f 运行代码文件名
         * %T 制表名
         * %l 行号
         * %d 时间 {}内时间字符解析，参std::put_time 默认"%Y-%m-%d %H:%M:%S"
         * %% 百分号
         * %L 输出日志事件的发生位置 file:line(function)
         * 默认格式%d{%Y-%m-%d %H:%M:%S}(%t)(%F)[%p][%c] %f:%l> %m%n
         *
         * @param patten
         */
        LogFormater(const std::string &patten = "");

        /**
         * @brief 解析日志，转成字符串
         *
         * @param event
         * @return std::string
         */
        std::string format(LogEvent::ptr event);

        /**
         * @brief 允许你使用<<输出字符流
         *
         * @param os
         * @param event
         * @return std::ostream&
         */
        std::ostream &format(std::ostream &os, LogEvent::ptr event);

        /**
         * @brief Get the Patten object
         *
         * @return std::string
         */
        std::string getPatten() const
        {
            return _pattern;
        }

        /**
         * @brief 单元格式解析器，解析单个格式
         *
         */
        class FormatItem
        {
        public:
            using ptr = std::shared_ptr<FormatItem>;

            virtual ~FormatItem(){};

            /**
             * @brief 格式引索
             *
             * 从LogEvent获取日志事件内容，根据子类输出对应os流
             * @param os 提供获取os流
             * @param event
             */
            virtual void format(std::ostream &os, LogEvent::ptr event) = 0;
        };

    private:
        /**
         * @brief 初始化FormatItem单元格式解析器,根据格式生成对应的解析器
         *
         */
        void formatItemInit();
        // 字符串格式
        std::string _pattern;
        // 单元解析器集
        std::vector<FormatItem::ptr> _formatItems;
    };

} // namespace WebSrv
