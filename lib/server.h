#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

/*
Om aan te geven dat er een constante zelf gedefinieerd is.
Kan gebruik gemaakt worden van dit deze preproccesor commandos
*/
#ifdef REDEFINE_BUFFER_SIZE
    #define RECV_BUFFER_SIZE REDEFINE_BUFFER_SIZE
#else
    #define RECV_BUFFER_SIZE 256
#endif

#ifdef REDEFINE_BACKLOG
    #define BACKLOG_SIZE REDEFINE_BACKLOG_SIZE
#else
    #define BACKLOG_SIZE 5
#endif

/*
De server struct.
Alle variabelen belangrijk voor de server en routines zijn hierin geplaatst.
Dit is makkelijk mee te geven bij de subroutines.
*/
struct server_t {
    int fd;
    int clientfd;
    struct sockaddr_in addr;
    struct sockaddr_in client;
    char recvBuffer[RECV_BUFFER_SIZE];
};

struct client_t {
    int fd;
    char recvBuffer[RECV_BUFFER_SIZE];
};

/*Functie definitie voor de server*/
int createServer(const char * address, unsigned int port, void* (*onRequest)(void * server));
