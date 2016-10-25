#include <stdio.h>
#include <sqlite3.h>
#include "finsh.h"
#include <rtthread.h>
#include <sys.h>
#include <dfs.h>
#include <dfs_posix.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for(i=0; i<argc; i++)
	{
		rt_kprintf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	rt_kprintf("\n");
	return 0;
}

int test10_main(void){
  static int step = 0;
  int argc = 4;
  char *argv[4] = {"test10_main", "/test.db", 
            "create table mytable(entry1 int)",
            "drop table mytable"};
  sqlite3 *db;
  char *zErrMsg = 0;
  int rc;

  if( argc<3 ){
    rt_kprintf("Usage: %s DATABASE SQL-STATEMENT\n", argv[0]);
    return(1);
  }
  rt_kprintf("ready open %s\n", argv[1]);
  rc = sqlite3_open(argv[1], &db);

  if( rc ){
    rt_kprintf("Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    return(1);
  }
  else{
    rt_kprintf("open %s success\n", argv[1]);
  }

  if (step == 0) {
      rt_kprintf("SQL exec: %s\n", argv[2]);
      rc = sqlite3_exec(db, argv[2], callback, 0, &zErrMsg);
      if( rc!=SQLITE_OK ){
        rt_kprintf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      }
      else{
        rt_kprintf("SQL exec: %s success\n", argv[2]);
      }
  }
  else {
      rt_kprintf("SQL exec: %s\n", argv[3]);
      rc = sqlite3_exec(db, argv[3], callback, 0, &zErrMsg);
      if( rc!=SQLITE_OK ){
        rt_kprintf("SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
      }
      else{
        rt_kprintf("SQL exec: %s success\n", argv[3]);
      }
  }

  step = !step;
  
  sqlite3_close(db);
  return 0;
}
FINSH_FUNCTION_EXPORT(test10_main, sqlite main)

void sqlite3_test()
{
	const char *dbPath = "/member.db";
	sqlite3 *db;
	char *zErrMsg = 0;
	int rc;
	char *sql_cmd = (char *)rt_malloc(1024);
	
	if(sql_cmd == NULL)
	{
		rt_kprintf("rt_malloc failed\r\n");
		return;
	}
	/* 打开数据库 */
	rt_kprintf("ready open %s\n", dbPath);
	rc = sqlite3_open(dbPath, &db);
	if (rc != SQLITE_OK)
	{
		rt_kprintf("Can't open database: %s\n", sqlite3_errmsg(db));
		goto out;
	}
	else
	{
		rt_kprintf("open %s success\n", dbPath);
	}
	
	/* 建立表格 */
	sprintf(sql_cmd, "CREATE TABLE datapro(package INTEGER,offset INTEGER,lklen INTEGER,base INTEHER,link INTEGER,err INTEGER);");
	rc=sqlite3_exec(db, sql_cmd, 0, 0, &zErrMsg); //建立表datapro
	if (rc == SQLITE_OK) //建表成功
		printf("create the chn_to_eng table successful!\n");
	else
	{
		if (zErrMsg != NULL)
		{
			printf("%s\n",zErrMsg);
			sqlite3_free(zErrMsg) ;		
		}
	}

	/* 添加数据 */
	sprintf(sql_cmd,"INSERT INTO datapro VALUES(%d,%d,%d,%d,%d,%d);",4,2345,268,9,3,3);
	rc=sqlite3_exec(db,sql_cmd, 0, 0, &zErrMsg);
	if (rc == SQLITE_OK) //成功
		printf("insert successful!\n");
	else
	{
		if (zErrMsg != NULL)
		{
			printf("%s\n",zErrMsg);
			sqlite3_free(zErrMsg) ;		
		}
	}
	
out:	
	rt_free(sql_cmd);
	sqlite3_close(db);
}
FINSH_FUNCTION_EXPORT(sqlite3_test, sqlite test)


s8 mul_file_test(void)
{
	const char *dbPath1 = "/test1.txt";
	const char *dbPath2 = "/test2.txt";
	const char *dbPath3 = "/test3.txt";
	const char text1[] = "text-fatfs-test1";
	const char text2[] = "text-fatfs-test2";
	const char text3[] = "text-fatfs-test3";
	int fd1,fd2,fd3;
	/* 打开 */
	fd1 = open(dbPath1, O_RDWR|O_CREAT, 0);
	if (fd1 < 0)
	{
		rt_kprintf("open %s failed\r\n", dbPath1);
		close(fd1);
		return -1;
	}
	/* 打开 */
	fd2 = open(dbPath2, O_RDWR|O_CREAT, 0);
	if (fd2 < 0)
	{
		rt_kprintf("open %s failed\r\n", dbPath2);
		close(fd2);
		return -1;
	}
	/* 打开 */
	fd3 = open(dbPath3, O_RDWR|O_CREAT, 0);
	if (fd3 < 0)
	{
		rt_kprintf("open %s failed\r\n", dbPath3);
		close(fd3);
		return -1;
	}
	
	/* 写入人员总数 */
	if (write(fd1, text1, sizeof(text1)) != sizeof(text1))
	{
		rt_kprintf("write %s failed\r\n", dbPath1);
		close(fd1);
		return -1;
	}
	else
	{
		rt_kprintf("write %s success\r\n", dbPath1);
	}
	/* 写入人员总数 */
	if (write(fd2, text2, sizeof(text2)) != sizeof(text2))
	{
		rt_kprintf("write %s failed\r\n", dbPath2);
		close(fd2);
		return -1;
	}
	else
	{
		rt_kprintf("write %s success\r\n", dbPath2);
	}
	/* 写入人员总数 */
	if (write(fd3, text3, sizeof(text3)) != sizeof(text3))
	{
		rt_kprintf("write %s failed\r\n", dbPath3);
		close(fd3);
		return -1;
	}
	else
	{
		rt_kprintf("write %s success\r\n", dbPath3);
	}
	
	close(fd1);
	close(fd2);
	close(fd3);
	return 0;
}
FINSH_FUNCTION_EXPORT(mul_file_test, mul file test)
