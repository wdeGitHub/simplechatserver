#include"sqlconnectionpool.h"
#include<iostream>

using namespace std;
int main()
{
    SqlConnectionPool *pool=SqlConnectionPool::getInstance();
    pool->init("localhost","root","@Yd66666","SCSdata",0,8);
    for(int i=0;i<pool->GetFreeConn();i++)
    {
        cout<<i;
MYSQL *mysql=pool->GetConnection();

    char *psql = "select * from user";
	int ret = mysql_query(mysql, psql);
    //
     MYSQL_RES* res = mysql_store_result(mysql);
     //
     int num = mysql_num_fields(res);

    // 6. 得到所有列的名字, 并且输出
    MYSQL_FIELD * fields = mysql_fetch_fields(res);
    for(int i=0; i<num; ++i)
    {
        printf("%s\t\t", fields[i].name);
    }
    printf("\n");
    MYSQL_ROW row;
    while( (row = mysql_fetch_row(res)) != NULL)
    {
        // 将当前行中的每一列信息读出
        for(int i=0; i<num; ++i)
        {
            printf("%s\t\t", row[i]);
        }
        printf("\n");
    }
    mysql_free_result(res);
    pool->ReleaseConnection(mysql);
    }
    pool->DestroyPool();
    // MYSQL *mysql=mysql_init(NULL);
    // if(mysql==nullptr)
    // {
    //     int ret=mysql_errno(mysql);
    //     return ret;
    // }
    // mysql = mysql_real_connect(mysql, "localhost", "root", "@Yd66666", "SCSdata", 0, NULL, 0);
    // //数据库连接池的本质要求就是返回一个MYSQL类,
    // mysql_set_character_set(mysql, "utf8");
    // //
    // char *psql = "select * from user";
	// int ret = mysql_query(mysql, psql);
    // //
    //  MYSQL_RES* res = mysql_store_result(mysql);
    //  //
    //  int num = mysql_num_fields(res);

    // // 6. 得到所有列的名字, 并且输出
    // MYSQL_FIELD * fields = mysql_fetch_fields(res);
    // for(int i=0; i<num; ++i)
    // {
    //     printf("%s\t\t", fields[i].name);
    // }
    // printf("\n");
    // MYSQL_ROW row;
    // while( (row = mysql_fetch_row(res)) != NULL)
    // {
    //     // 将当前行中的每一列信息读出
    //     for(int i=0; i<num; ++i)
    //     {
    //         printf("%s\t\t", row[i]);
    //     }
    //     printf("\n");
    // }

    // // 8. 释放资源 - 结果集
    // mysql_free_result(res);

    // // 9. 关闭连接
    // mysql_close(mysql);
    return 0;
}
