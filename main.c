#define REDEFINE_BUFFER_SIZE 1024
#include "lib/database.h"
#include "lib/server.h"

/*
Compile 'n run using
gcc main.c lib/database.c lib/server.c -ansi -Wall -Wextra -Wconversion $(mariadb_config --libs) -o main && ./program
*/

int clientfd;
void printMessage(MYSQL_ROW row);
void *onRequest(void * server);
void onError(const char * error);
void aarde(struct client_t * client, MYSQL * conn);
void lucht(struct client_t * client, MYSQL * conn);
void water(struct client_t * client, MYSQL * conn);
void vuur(struct client_t * client, MYSQL * conn);

int main() {
    setMysqlErrorCallback(&onError);
    /*conn = connectMysqlDatabase("localhost", "root", "root", "domotica");*/
    /*if (conn != 0) {*/
        createServer("0.0.0.0", 1337, &onRequest);
    /*    closeMysqlSocket(conn);
    }
    mysql_close(conn);*/
    return 0;
}

int g = 0;

void onError(const char * error) {
    printf("%s", error);
}

void *onRequest(void * args) {
    struct client_t *client = args;
    printf("Request: %d ::: ", g++);
    MYSQL * conn = connectMysqlDatabase("localhost", "root", "root", "domotica");
    if (!conn) {
        printf("Database connection failed!\n");
        pthread_detach(pthread_self());
        return NULL;
    }

    switch(client->recvBuffer[0]) {
        case '0':
            aarde(client, conn);
        break;
        case '1':
            lucht(client, conn);
        break;
        case '2':
            water(client, conn);
        break;
        case '3':
            vuur(client, conn);
        break;
    }
    mysql_close(conn);
    close(client->fd);
    free(client);
    pthread_detach(pthread_self());
    return NULL;
}

void aarde(struct client_t * client, MYSQL * conn) {
    printf("Request from device 0\n");

    char msg[100] = "";
    MYSQL_RES * result;
    MYSQL_ROW row;
    char sql[400] = "";

    int powerState = client->recvBuffer[1]-48;

    /* sla de powerstate van de server op in de database */
    sprintf(sql, "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'LEDpower';");
    result = mysqlQuery(conn, sql);

    /* als de tabel niet bestaat moet deze aangemaakt, anders wordt de powerstate vergeleken met die in de db en zo nodig geupdatet */
    if (mysql_num_rows(result) > 0) { /*  */
        sprintf(sql, "SELECT aan FROM LEDpower ORDER BY updatedTime DESC LIMIT 1;");
        result = mysqlQuery(conn, sql);
        while ((row = mysql_fetch_row(result))) {
            int lastState = atoi(row[0]);
            if (lastState != powerState) {
                sprintf(sql, "INSERT INTO LEDpower (aan) VALUES ('%d');", powerState);
                mysqlQuery(conn, sql);
                printf("Updated state\n");
            }
        }
    } else {
        sprintf(sql, "CREATE TABLE LEDpower (aan int(1) NOT NULL, updatedTime TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP);");
        mysqlQuery(conn, sql);
        sprintf(sql, "INSERT INTO LEDpower (aan) VALUES ('%d');", powerState);
        mysqlQuery(conn, sql);
    }

    /* haal alle ingestelde waardes uit de database en geef deze geformatteerd door aan de arduino */
    sprintf(sql, "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'LEDcontrol';");
    result = mysqlQuery(conn, sql);

    if (mysql_num_rows(result) > 0) {
        sprintf(sql, "SELECT * FROM LEDcontrol");
        result = mysqlQuery(conn, sql);
        if (mysql_num_rows(result) > 0) {
            row = mysql_fetch_row(result);
            sprintf(msg, "%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n", row[1], row[2], row[3], row[4], row[5], row[6], row[7], row[8], row[9], row[10], row[11], row[12], row[13]);
        }
    }
    send(client->fd, msg, strlen(msg), 0);
}

void lucht(struct client_t * client, MYSQL * conn) {
    printf("Request from Knocklock\n");

    char msg[100] = "";
    MYSQL_RES * result;
    MYSQL_ROW row;
    char sql[400] = "";

	msg[0] = '0';
	char knockCode[32] = {0};
	strcpy(knockCode, &client->recvBuffer[1]);
	printf("%s\n", knockCode);

    sprintf(sql, "SELECT * FROM knockers;");
    result = mysqlQuery(conn, sql);

    /*SQL Gedeelte*/
    if (mysql_num_rows(result) > 0) {
		while((row = mysql_fetch_row(result))) {
		    if (strcmp(row[2], knockCode) == 0) {
    			msg[0] = '1';
    			sprintf(sql, "SELECT * FROM knockactivity;");
    			mysqlQuery(conn, sql);
    			if (mysql_num_rows(result) > 0) {
    			    sprintf(sql, "INSERT INTO knockactivity (knock_id) VALUES (%s)", row[0]);
    			    mysqlQuery(conn, sql);
    			}
    			else {
    			    sprintf(sql, "CREATE TABLE knockactivity (id int(11) AUTO_INCREMENT PRIMARY KEY, knock_id int(11), time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP);");
    			    mysqlQuery(conn, sql);
    			}
		    }
		}
    }
	else {
		/*Als table niet bestaat*/
		sprintf(sql, "CREATE TABLE knockers (id int(11) AUTO_INCREMENT PRIMARY KEY, naam VARCHAR(32), code VARCHAR(32));");
		mysqlQuery(conn, sql);
	}

	send(client->fd, msg, strlen(msg), 0);
}

void water(struct client_t * client, MYSQL * conn) {
	printf("Request from plant watering system\n");

	char msg[100] = "";
	MYSQL_RES * result;
	MYSQL_ROW row;
	char sql[400] = "";

	char moisturelevel[32] = {0};

	strcpy(moisturelevel, &client->recvBuffer[1]);
	printf("moisture level: %s\n", moisturelevel);

	sprintf(sql, "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'preferredwater';");
	result = mysqlQuery(conn, sql);

	if (mysql_num_rows(result) > 0) {
        	sprintf(sql, "SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'measuredwater';");
		result = mysqlQuery(conn, sql);

     		if (mysql_num_rows(result) > 0) {
            		sprintf(sql, "INSERT INTO measuredwater (level) VALUES (%s);", moisturelevel);
			mysqlQuery(conn, sql);

			sprintf(sql, "SELECT * FROM preferredwater");
			result = mysqlQuery(conn, sql);
			if (mysql_num_rows(result) > 0) {
				row = mysql_fetch_row(result);
				sprintf(msg, "%s", row[0]);
				printf("return value %s\n", msg);
				send(client->fd, msg, strlen(msg), 0);
			}
		}
		else {
			sprintf(sql, "CREATE TABLE measuredwater (id int AUTO_INCREMENT PRIMARY KEY, level INT UNSIGNED, time TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP)");
			mysqlQuery(conn, sql);
            		printf("Inserting %s to level\n", moisturelevel);
            		sprintf(sql, "INSERT INTO measuredwater (level) VALUES (%s);", moisturelevel);
            		mysqlQuery(conn, sql);
		}
	}
	else {
  		sprintf(sql, "CREATE TABLE preferredwater (preferredlevel INT UNSIGNED);");
  		mysqlQuery(conn, sql);
		sprintf(sql, "INSERT INTO preferredwater (preferredlevel) VALUES (400);");
		mysqlQuery(conn, sql);
    }
}

void vuur(struct client_t * client, MYSQL * conn) {
    printf("Request from NFC device\n");

    printf("Dit is de buffer: %s\n", client->recvBuffer);

    MYSQL_RES * result;
    MYSQL_ROW row;
    char sql[400] = "";

	char versturen[50];
  	char data[300];
    strcpy(data,client->recvBuffer);/*Haal de 3 van de rest van de cijfers af, die worden gestuurd van de arduino*/
    sprintf(sql,"SELECT * FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_NAME = 'persoon';");
    mysql_query(conn, sql);
    result = mysql_store_result(conn);
    if (mysql_num_rows(result) > 0) { /*Als de result meer dan 0 regels heeft bestaat de tabel */
        sprintf(sql,"SELECT * FROM persoon WHERE ID LIKE '%s';", &data[1]);
        printf("DE SQL: %s\n", sql);
        mysql_query(conn, sql);
        result = mysql_store_result(conn);
        int auth = 0; /*Wordt 1 als de persoon in de db staat*/
        if(mysql_num_rows(result) > 0){/*Als de result meer dan 0 regels heeft bestaat het ID in de tabel*/
            auth = 1;
        }
        else{
            printf("Persoon staat niet in db\n");
        }
        if(auth == 1){/*Persoon staat in de database*/
            sprintf(sql, "SELECT locatie FROM persoon WHERE ID LIKE '%s';", &data[1]);/*Als locatie 1 is, is de persoon al binnen, als locatie 0 is, is de persoon nog niet binnen*/
            mysql_query(conn, sql);
            result = mysql_store_result(conn);
            row = mysql_fetch_row(result);
            unsigned int i;
            int p = 0;
            char val[20] = "";
            char q[2];
                for(i = 0; i < mysql_num_fields(result); i++)
                {
                    sprintf(q,"%s ",row[i] );
                }
            if(*q == '1'){/*Persoon is binnen dus als persoon weer zijn pas aanbied, moet er "tot ziens" uitgeprint worden op het arduino scherm*/
                sprintf(sql, "UPDATE persoon SET Locatie = 0 WHERE ID LIKE '%s';", &data[1]);
                mysql_query(conn, sql);
                sprintf(sql,"SELECT voornaam,achternaam FROM persoon WHERE ID LIKE '%s';",&data[1] );
                mysql_query(conn, sql);
                result = mysql_store_result(conn);
		row = mysql_fetch_row(result);
                for(i = 0; i < mysql_num_fields(result); i++)
                {
                    p += sprintf(&val[p],"%s ",row[i] );
                }
                printf("%s\n", val);
                sprintf(versturen,"GTot ziens!%s         ",val);/*Text die moet worden uitgeprint op het scherm in een char array stoppen, zodat het makkelijk te versturen is*/
                send(client->fd, versturen, sizeof(versturen), 0);
            }
            else if(*q == '0'){/*Persoon is nog niet binnen dus als persoon weer zijn pas aanbied, moet er "welkom" uitgeprint worden op het arduino scherm*/
                sprintf(sql, "UPDATE persoon SET Locatie= 1 WHERE ID LIKE '%s';",&data[1]);
		         mysql_query(conn, sql);
		        sprintf(sql,"SELECT voornaam,achternaam FROM persoon WHERE ID LIKE '%s';",&data[1] );
                mysql_query(conn, sql);
                result = mysql_store_result(conn);
		row = mysql_fetch_row(result);
                for(i = 0; i < mysql_num_fields(result); i++)
                {
                    p += sprintf(&val[p],"%s ",row[i] );
                }
                printf("%s\n", val);
                sprintf(versturen,"GWelkom:%s      ",val);/*Text die moet worden uitgeprint op het scherm in een char array stoppen, zodat het makkelijk te versturen is*/  
                send(client->fd, versturen, sizeof(versturen), 0);
            }
        }
        else{
            sprintf(versturen,"RPas is niet geautoriseerd!      ");
            printf("%s\n", versturen);
            send(client->fd, versturen, sizeof(versturen), 0);
        }
    }
    else{ /* Maak de tabel als deze nog niet bestaat*/
        sprintf(sql, "CREATE TABLE persoon(ID int NOT NULL,Locatie int NOT NULL, voornaam varchar(25) NOT NULL, achternaam varchar(25) NOT NULL, date TIMESTAMP DEFAULT now() ON UPDATE now());");
        mysqlQuery(conn, sql);
    }
}
