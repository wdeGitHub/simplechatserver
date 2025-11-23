#ifndef SQL_SQLCONNECTIONPOOL_H
#define SQL_SQLCONNECTIONPOOL_H

#include<iostream>
#include<string>
#include"../safequeue/safequeue.h"
#include"mysql/mysql.h"
class SqlConnectionPool
{
public:
    static SqlConnectionPool*getInstance();
    void init(std::string ip,std::string user,std::string passwd,std::string db,int Port,int con_num);
    MYSQL* GetConnection();
    void ReleaseConnection(MYSQL*con);
    int GetFreeConn();					 //获取连接
	void DestroyPool();					 //销毁所有连接
private:
    SqlConnectionPool()=default;
    ~SqlConnectionPool();
    MYSQL* m_mysql;
    SafeQueue<MYSQL*>conlist;
};
#endif