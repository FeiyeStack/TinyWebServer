#include <iostream>
#include "TinyWebServer/db/mysql.h"
#include "TinyWebServer/iomanager.h"
#include "TinyWebServer/log.h"
static WebSrv::Logger::ptr g_logger = SRV_LOGGER_NAME("test");

void run()
{

    std::map<std::string, std::string> params;
    params["host"] = "127.0.0.1";
    params["user"] = "test";
    params["passwd"] = "test123456";
    params["dbname"] = "test";

    WebSrv::MySQL::ptr mysql(new WebSrv::MySQL(params));
    if (!mysql->connect())
    {
        SRV_LOG_ERROR(g_logger) << "connect fail";
        return;
    }

    // 创建表
    std::string createTableSQL = R"(
        CREATE TABLE IF NOT EXISTS user (
            id INT PRIMARY KEY AUTO_INCREMENT,
            name VARCHAR(50) NOT NULL,
            age INT NOT NULL,
            update_time TIMESTAMP
        )
    )";

    if (mysql->execute(createTableSQL) != 0)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to create table: " << mysql->getErrStr();
        return;
    }

    // 插入数据
    std::string insertDataSQL = "INSERT INTO user (name, age, update_time) VALUES (?, ?, ?)";
    if (mysql->execStmt(insertDataSQL.c_str(), "zhang san", 25, WebSrv::MySQLTime(time(nullptr))) != 0)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to insert data: " << mysql->getErrStr();
        return;
    }

    if (mysql->execStmt(insertDataSQL.c_str(), "li si", 17, WebSrv::MySQLTime(time(nullptr))) != 0)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to insert data: " << mysql->getErrStr();
        return;
    }

    if (mysql->execStmt(insertDataSQL.c_str(), "wang wu", 28, WebSrv::MySQLTime(time(nullptr))) != 0)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to insert data: " << mysql->getErrStr();
        return;
    }

    // 查询数据
    std::string queryDataSQL = "SELECT * FROM user WHERE age > ?";
    auto result = mysql->queryStmt(queryDataSQL.c_str(), 20);
    if (!result)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to query data: " << mysql->getErrStr();
        return;
    }

    // 输出查询结果
    while (result->next())
    {
        SRV_LOG_DEBUG(g_logger) << "id: " << result->getInt32(0)
                                << ", name: " << result->getString(1)
                                << ", age: " << result->getInt32(2)
                                << ", update_time: " << WebSrv::timeToStr(result->getTime(3));
    }

    // 更新数据
    std::string updateDataSQL = "UPDATE user SET age = ? WHERE name = ?";
    if (mysql->execStmt(updateDataSQL.c_str(), 30, "zhang san") != 0)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to update data: " << mysql->getErrStr();
        return;
    }

    // 删除数据
    std::string deleteDataSQL = "DELETE FROM user WHERE age < ?";
    if (mysql->execStmt(deleteDataSQL.c_str(), 25) != 0)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to delete data: " << mysql->getErrStr();
        return;
    }

    // 查询数据
    std::string queryAllDataSQL = "SELECT * FROM user";
    auto result2 = mysql->queryStmt(queryAllDataSQL.c_str());
    if (!result)
    {
        SRV_LOG_ERROR(g_logger) << "Failed to query data: " << mysql->getErrStr();
        return;
    }

    // 输出查询结果
    while (result2->next())
    {
        SRV_LOG_DEBUG(g_logger) << "id: " << result2->getInt32(0)
                                << ", name: " << result2->getString(1)
                                << ", age: " << result2->getInt32(2)
                                << ", update_time: " << WebSrv::timeToStr(result2->getTime(3));
    }
}

int main(int argc, char **argv)
{
    WebSrv::IOManager iom(1);
    iom.schedule(run);
    // iom.addTimer(1000, run, true);
    return 0;
}
