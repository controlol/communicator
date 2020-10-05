#include "database.h"

/*Standaard error functie*/
void printMysqlError(string error) {
    printf("Mysql error: %s\n", error);
}

void (*mysqlErrorCallback)(string) = &printMysqlError;

void setMysqlErrorCallback(void (*callback)(string)) {
    /*Functie pointer voor errors.*/
    mysqlErrorCallback = callback;
}

void printMysqlClientVersion() {
    /*Print mysql versie.*/
    printf("Mysql Client version: %s\n", mysql_get_client_info());
}

MYSQL * connectMysqlDatabase(string address, string user, string pass, string db) {
    MYSQL * connection = mysql_init(NULL);
    /*Initialiseer database socket.*/ 
    if (connection == 0) {
        mysqlErrorCallback(mysql_error(connection));
        return 0;
    }
    /*Verbind met database.*/
    if (mysql_real_connect(connection, address, user, pass, db, 0, NULL, 0) == 0) {
        mysqlErrorCallback(mysql_error(connection));
        mysql_close(connection);
        return 0;
    }
    return connection;
}

MYSQL_RES * mysqlQuery(MYSQL * connection, string query) {
    /*Voer de query uit, als deze gelukt is returned de functie 0, anders 1.*/
    if (mysql_query(connection, query)) {
        mysqlErrorCallback(mysql_error(connection));
    }
    /*sla het resultaat op in een mysql resultaat struct.*/
    MYSQL_RES * result = mysql_store_result(connection);
    return result;
}

void mysqlForeach(MYSQL_RES * result, void (*callback)(MYSQL_ROW)) {
    MYSQL_ROW row;
    /*Voor elke row die is opgehaald, voer de volgende functie uit.*/
    while((row = mysql_fetch_row(result))) {
        callback(row);
    };
}

void closeMysqlSocket(MYSQL * connection) {
    /*Sluit de mysql socket af.*/
    mysql_close(connection);
}