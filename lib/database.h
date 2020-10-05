#pragma once
/*
Library voor MySQL.
Paperdev - Jorn Veken - 2020.
*/

#include <stdio.h>
#include <stdlib.h>
#include <mariadb/mysql.h>

typedef const char * string;

void printMysqlError(string error);

void setMysqlErrorCallback(void (*callback)(string));

void printMysqlClientVersion();

MYSQL * connectMysqlDatabase(string address, string user, string pass, string db);

MYSQL_RES * mysqlQuery(MYSQL * connection, string query);

void mysqlForeach(MYSQL_RES * result, void (*callback)(MYSQL_ROW));

void closeMysqlSocket(MYSQL * connection);