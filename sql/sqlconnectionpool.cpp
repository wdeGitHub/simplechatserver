#include"sqlconnectionpool.h"
#include <stdexcept>

SqlConnectionPool *SqlConnectionPool::getInstance()
{
    static SqlConnectionPool connpool;
    return &connpool;
}

void SqlConnectionPool::init(std::string ip, std::string user, std::string passwd, std::string db, int Port,int con_num)
{
    conlist.setblock();
    for(int i=0;i<con_num;i++)
    {
        MYSQL *mysql=mysql_init(NULL);//会分配内存
        if(mysql==nullptr)
        {
            throw std::runtime_error("mysql_init failed");
        }
        mysql = mysql_real_connect(mysql, ip.c_str(), user.c_str(), passwd.c_str(), db.c_str(), Port, NULL, 0);
        if(mysql == nullptr)
        {
            throw std::runtime_error("mysql_real_connect failed");
        }
        mysql_set_character_set(mysql, "utf8");
        conlist.push_b(mysql);
    }
}
MYSQL *SqlConnectionPool::GetConnection()
{
    return conlist.pop_b();
}

void SqlConnectionPool::ReleaseConnection(MYSQL *con)
{
    conlist.push_b(con);
}

int SqlConnectionPool::GetFreeConn()
{
    return conlist.size();
}

void SqlConnectionPool::DestroyPool()
{
    while(!conlist.empty())
    {
        MYSQL *con=conlist.pop_b();
        mysql_close(con);
    }
}

SqlConnectionPool::~SqlConnectionPool()
{
    DestroyPool();
}
