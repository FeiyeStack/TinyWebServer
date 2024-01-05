#include <iostream>
#include "TinyWebServer/db/mysql.h"
#include "TinyWebServer/iomanager.h"

void run() {

    std::map<std::string, std::string> params;
    params["host"] = "127.0.0.1";
    params["user"] = "test";
    params["passwd"] = "123456";
    params["dbname"] = "test";

    WebSrv::MySQL::ptr mysql(new WebSrv::MySQL(params));
    if (!mysql->connect())
    {
        std::cout << "connect fail" << std::endl;
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
        std::cout << "Failed to create table: " << mysql->getErrStr() << std::endl;
        return;
    }

    // 插入数据
    std::string insertDataSQL = "INSERT INTO user (name, age, update_time) VALUES (?, ?, ?)";
    if (mysql->execStmt(insertDataSQL.c_str(), "John Doe", 25, WebSrv::MySQLTime(time(nullptr))) != 0)
    {
        std::cout << "Failed to insert data: " << mysql->getErrStr() << std::endl;
        return;
    }

    // 查询数据
    std::string queryDataSQL = "SELECT * FROM user WHERE age > ?";
    auto result = mysql->queryStmt(queryDataSQL.c_str(), 20);
    if (!result)
    {
        std::cout << "Failed to query data: " << mysql->getErrStr() << std::endl;
        return;
    }

    // 输出查询结果
    while (result->next())
    {
        std::cout << "id: " << result->getInt64(0)
                  << ", name: " << result->getString(1)
                  << ", age: " << result->getInt32(2)
                  << ", update_time: " << result->getTime(3) << std::endl;
    }

    // 更新数据
    std::string updateDataSQL = "UPDATE user SET age = ? WHERE name = ?";
    if (mysql->execStmt(updateDataSQL.c_str(), 30, "John Doe") != 0)
    {
        std::cout << "Failed to update data: " << mysql->getErrStr() << std::endl;
        return;
    }

    // 删除数据
    std::string deleteDataSQL = "DELETE FROM user WHERE age < ?";
    if (mysql->execStmt(deleteDataSQL.c_str(), 25) != 0)
    {
        std::cout << "Failed to delete data: " << mysql->getErrStr() << std::endl;
        return;
    }
}

int main(int argc, char** argv) {
    WebSrv::IOManager iom(1);
    iom.schedule(run);
    //iom.addTimer(1000, run, true);
    return 0;
}
