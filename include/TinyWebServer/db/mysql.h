#pragma once

#include <mysql/mysql.h>
#include <memory>
#include <functional>
#include <map>
#include <vector>
#include <mutex>
#include "db.h"
#include <list>

namespace WebSrv {

bool mysql_time_to_time_t(const MYSQL_TIME& mt, time_t& ts);
bool time_t_to_mysql_time(const time_t& ts, MYSQL_TIME& mt);

class MySQL;
class MySQLStmt;
struct MySQLTime {
    MySQLTime(time_t t)
        :ts(t) { 
            time_t_to_mysql_time(ts,mt);
        }
    time_t ts;
    MYSQL_TIME mt;
};



class MySQLRes : public ISQLData {
public:
    using ptr=std::shared_ptr<MySQLRes>;
    using data_cb=std::function<bool(MYSQL_ROW row
                ,int field_count, int row_no)>;
    MySQLRes(MYSQL_RES* res, int eno, const char* estr);

    MYSQL_RES* get() const { return m_data.get();}

    int getErrno() const { return m_errno;}
    const std::string& getErrStr() const { return m_errstr;}

    bool foreach(data_cb cb);

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;

    bool isNull(int idx) override;
    int8_t getInt8(int idx) override;
    uint8_t getUint8(int idx) override;
    int16_t getInt16(int idx) override;
    uint16_t getUint16(int idx) override;
    int32_t getInt32(int idx) override;
    uint32_t getUint32(int idx) override;
    int64_t getInt64(int idx) override;
    uint64_t getUint64(int idx) override;
    float getFloat(int idx) override;
    double getDouble(int idx) override;
    std::string getString(int idx) override;
    std::string getBlob(int idx) override;
    time_t getTime(int idx) override;
    bool next() override;
private:
    int m_errno;
    std::string m_errstr;
    MYSQL_ROW m_cur;
    unsigned long* m_curLength;
    std::shared_ptr<MYSQL_RES> m_data;
};

class MySQLStmtRes : public ISQLData {
friend class MySQLStmt;
public:

        using ptr=std::shared_ptr<MySQLStmtRes>;

    static MySQLStmtRes::ptr Create(std::shared_ptr<MySQLStmt> stmt);
    ~MySQLStmtRes();

    int getErrno() const { return m_errno;}
    const std::string& getErrStr() const { return m_errstr;}

    int getDataCount() override;
    int getColumnCount() override;
    int getColumnBytes(int idx) override;
    int getColumnType(int idx) override;
    std::string getColumnName(int idx) override;

    bool isNull(int idx) override;
    int8_t getInt8(int idx) override;
    uint8_t getUint8(int idx) override;
    int16_t getInt16(int idx) override;
    uint16_t getUint16(int idx) override;
    int32_t getInt32(int idx) override;
    uint32_t getUint32(int idx) override;
    int64_t getInt64(int idx) override;
    uint64_t getUint64(int idx) override;
    float getFloat(int idx) override;
    double getDouble(int idx) override;
    std::string getString(int idx) override;
    std::string getBlob(int idx) override;
    time_t getTime(int idx) override;
    bool next() override;
private:
    MySQLStmtRes(std::shared_ptr<MySQLStmt> stmt, int eno, const std::string& estr);
    struct Data {
        Data();
        ~Data();
        void alloc(size_t size);

        bool is_null;
        bool error;
        enum_field_types type;
        unsigned long length;
        int32_t data_length;
        char* data;
    };
private:
    int m_errno;
    std::string m_errstr;
    std::shared_ptr<MySQLStmt> m_stmt;
    std::vector<MYSQL_BIND> m_binds;
    std::vector<Data> m_datas;
};

class MySQLManager;
class MySQL : public IDB
              ,public std::enable_shared_from_this<MySQL> {
friend class MySQLManager;
public:
    using ptr=std::shared_ptr<MySQL>;

    MySQL(const std::map<std::string, std::string>& args);

    bool connect();
    bool ping();

    virtual int execute(const char* format, ...) override;
    int execute(const char* format, va_list ap);
    virtual int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    std::shared_ptr<MySQL> getMySQL();
    std::shared_ptr<MYSQL> getRaw();

    uint64_t getAffectedRows();

    ISQLData::ptr query(const char* format, ...) override;
    ISQLData::ptr query(const char* format, va_list ap); 
    ISQLData::ptr query(const std::string& sql) override;

    ITransaction::ptr openTransaction(bool auto_commit) override;
    IStmt::ptr prepare(const std::string& sql) override;

    template<typename... Args>
    int execStmt(const char* stmt, Args&&... args);

    template<class... Args>
    ISQLData::ptr queryStmt(const char* stmt, Args&&... args);

    const char* cmd();

    bool use(const std::string& dbname);
    int getErrno() override;
    std::string getErrStr() override;
    uint64_t getInsertId();
private:
    bool isNeedCheck();
private:
    std::map<std::string, std::string> m_params;
    std::shared_ptr<MYSQL> m_mysql;

    std::string m_cmd;
    std::string m_dbname;

    uint64_t m_lastUsedTime;
    bool m_hasError;
    int32_t m_poolSize;
};

class MySQLTransaction : public ITransaction {
public:
    using ptr=std::shared_ptr<MySQLTransaction>;

    static MySQLTransaction::ptr Create(MySQL::ptr mysql, bool auto_commit);
    ~MySQLTransaction();

    bool begin() override;
    bool commit() override;
    bool rollback() override;

    virtual int execute(const char* format, ...) override;
    int execute(const char* format, va_list ap);
    virtual int execute(const std::string& sql) override;
    int64_t getLastInsertId() override;
    std::shared_ptr<MySQL> getMySQL();

    bool isAutoCommit() const { return m_autoCommit;}
    bool isFinished() const { return m_isFinished;}
    bool isError() const { return m_hasError;}
private:
    MySQLTransaction(MySQL::ptr mysql, bool auto_commit);
private:
    MySQL::ptr m_mysql;
    bool m_autoCommit;
    bool m_isFinished;
    bool m_hasError;
};

class MySQLStmt : public IStmt
                  ,public std::enable_shared_from_this<MySQLStmt> {
public:
    using ptr=std::shared_ptr<MySQLStmt>;
    static MySQLStmt::ptr Create(MySQL::ptr db, const std::string& stmt);

    ~MySQLStmt();
    int bind(int idx, const int8_t& value);
    int bind(int idx, const uint8_t& value);
    int bind(int idx, const int16_t& value);
    int bind(int idx, const uint16_t& value);
    int bind(int idx, const int32_t& value);
    int bind(int idx, const uint32_t& value);
    int bind(int idx, const int64_t& value);
    int bind(int idx, const uint64_t& value);
    int bind(int idx, const float& value);
    int bind(int idx, const double& value);
    int bind(int idx, const std::string& value);
    int bind(int idx, const char* value);
    int bind(int idx, const void* value, int len);
    int bind(int idx, const MySQLTime& value);

    //for null type
    int bind(int idx);

    int bindInt8(int idx, const int8_t& value) override;
    int bindUint8(int idx, const uint8_t& value) override;
    int bindInt16(int idx, const int16_t& value) override;
    int bindUint16(int idx, const uint16_t& value) override;
    int bindInt32(int idx, const int32_t& value) override;
    int bindUint32(int idx, const uint32_t& value) override;
    int bindInt64(int idx, const int64_t& value) override;
    int bindUint64(int idx, const uint64_t& value) override;
    int bindFloat(int idx, const float& value) override;
    int bindDouble(int idx, const double& value) override;
    int bindString(int idx, const char* value) override;
    int bindString(int idx, const std::string& value) override;
    int bindBlob(int idx, const void* value, int64_t size) override;
    int bindBlob(int idx, const std::string& value) override;
    int bindTime(int idx, const time_t& value) override;
    int bindMySQLTime(int idx, const MySQLTime& value);

    int bindNull(int idx) override;

    int getErrno() override;
    std::string getErrStr() override;

    int execute() override;
    int64_t getLastInsertId() override;
    ISQLData::ptr query() override;

    MYSQL_STMT* getRaw() const { return m_stmt;}
private:
    MySQLStmt(MySQL::ptr db, MYSQL_STMT* stmt);
private:
    MySQL::ptr m_mysql;
    MYSQL_STMT* m_stmt;
    std::vector<MYSQL_BIND> m_binds;
};

class MySQLManager {
public:

    MySQLManager();
    ~MySQLManager();

    MySQL::ptr get(const std::string& name);
    void registerMySQL(const std::string& name, const std::map<std::string, std::string>& params);

    void checkConnection(int sec = 30);

    uint32_t getMaxConn() const { return m_maxConn;}
    void setMaxConn(uint32_t v) { m_maxConn = v;}

    int execute(const std::string& name, const char* format, ...);
    int execute(const std::string& name, const char* format, va_list ap);
    int execute(const std::string& name, const std::string& sql);

    ISQLData::ptr query(const std::string& name, const char* format, ...);
    ISQLData::ptr query(const std::string& name, const char* format, va_list ap); 
    ISQLData::ptr query(const std::string& name, const std::string& sql);

    MySQLTransaction::ptr openTransaction(const std::string& name, bool auto_commit);

    static MySQLManager* getMySQLManager();
private:
    void freeMySQL(const std::string& name, MySQL* m);
private:
    uint32_t m_maxConn;
    std::mutex m_mutex;
    std::map<std::string, std::list<MySQL*> > m_conns;
    std::map<std::string, std::map<std::string, std::string> > m_dbDefines;
};

class MySQLUtil {
public:
    static ISQLData::ptr Query(const std::string& name, const char* format, ...);
    static ISQLData::ptr Query(const std::string& name, const char* format,va_list ap); 
    static ISQLData::ptr Query(const std::string& name, const std::string& sql);

    static ISQLData::ptr TryQuery(const std::string& name, uint32_t count, const char* format, ...);
    static ISQLData::ptr TryQuery(const std::string& name, uint32_t count, const std::string& sql);

    static int Execute(const std::string& name, const char* format, ...);
    static int Execute(const std::string& name, const char* format, va_list ap); 
    static int Execute(const std::string& name, const std::string& sql);

    static int TryExecute(const std::string& name, uint32_t count, const char* format, ...);
    static int TryExecute(const std::string& name, uint32_t count, const char* format, va_list ap); 
    static int TryExecute(const std::string& name, uint32_t count, const std::string& sql);
};

namespace
{
    template <size_t N, typename... Args>
    struct MySQLBinder;

    template <size_t N, typename T, typename... Tail>
    struct MySQLBinder<N, T, Tail...>
    {
        static int Bind(MySQLStmt::ptr stmt, T value, Tail &...tail)
        {
            int rt = stmt->bind(N, value);
            if (rt != 0)
            {
                return rt;
            }
            return MySQLBinder<N + 1, Tail...>::Bind(stmt, tail...);
        }
    };

    template <size_t N>
    struct MySQLBinder<N>
    {
        static int Bind(MySQLStmt::ptr stmt)
        {
            return 0;
        }
    };



    // template <size_t N>
    // struct MySQLBinder<N, const char *>
    // {
    //     static int Bind(MySQLStmt::ptr stmt, const char *value)
    //     {
    //         return stmt->bind(N, value);
    //     }
    // };

    template <typename... Args>
    int bindX(MySQLStmt::ptr stmt, Args &&...args)
    {
        return MySQLBinder<1, Args...>::Bind(stmt, std::forward<Args>(args)...);
    }
}

template<typename... Args>
int MySQL::execStmt(const char* stmt, Args&&... args) {
    auto st = MySQLStmt::Create(shared_from_this(), stmt);
    if(!st) {
        return -1;
    }
    int rt = bindX(st, args...);
    if(rt != 0) {
        return rt;
    }
    return st->execute();
}

template<class... Args>
ISQLData::ptr MySQL::queryStmt(const char* stmt, Args&&... args) {
    auto st = MySQLStmt::Create(shared_from_this(), stmt);
    if(!st) {
        return nullptr;
    }
    int rt = bindX(st, args...);
    if(rt != 0) {
        return nullptr;
    }
    return st->query();
}

}

