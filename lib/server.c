#include "server.h"

int createServer(const char * address, unsigned int port, void* (*onRequest)(void * server)) {
    /*Server struct, hierin staan alle variabelen die bij de server horen.*/
    struct server_t server;
    /*Lengte van de client als deze verbind.*/
    unsigned int clientLen = sizeof(struct sockaddr_in);

    /*Maak een socket file descriptor aan, dit dient om reageren op de client.*/
    server.fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server.fd < 0) {
        printf("Could not create socket file descriptor. (%d)\n", errno);
        return -1;
    }

    /*Zet de socket attributen zoals protocol, port en adres*/
    server.addr.sin_family = AF_INET;
    server.addr.sin_port = htons((uint16_t)port);
    server.addr.sin_addr.s_addr = inet_addr(address);

    /*Bind de file descriptor*/
    if (bind(server.fd, (struct sockaddr *) &server.addr, sizeof(server.addr)) < 0) {
        printf("Could not bind socket. (%d)\n", errno);
        close(server.fd);
        return -1;
    }

    /*Start luisteren voor verbindingen.*/
    if (listen(server.fd, BACKLOG_SIZE) < 0) {
        printf("Could not start listening. (%d)\n", errno);
        close(server.fd);
        return -1;
    }

    /*
    Hier staat de main server loop.
    Hierin word gewacht op een client, als dit gebeurd dan;
        1) De recv buffer word schoongemaakt.
        2) De verbinding word geaccepteerd.
        3) Een forked proces word opgestart waarin de verbinding word behandeld.
        4) Een subroutine word opgestart waarin het request word behandeld.
        5) De verbinding met de client word gesloten en de fork stopt.
    */
    printf("Started server succesfully, listening on port (%d)...\n", port);
    /*Eeuwige loop, liefst is dit proces altijd online.*/

    while(1) {
        server.clientfd = accept(server.fd, (struct sockaddr *) &server.client, &clientLen);
        if (server.clientfd < 0) {
            printf("Failed to accept connection. (%d)", errno);
            close(server.fd);
            return -1;
        }
        else if (server.clientfd > 0) {
            pthread_t tid;
            memset(server.recvBuffer, 0, RECV_BUFFER_SIZE);
            recv(server.clientfd, server.recvBuffer, RECV_BUFFER_SIZE, 0);

            struct client_t *client_data = (struct client_t*)malloc(sizeof(struct client_t));

            if (client_data == NULL) {
                printf("Error! couldn't allocate memory for struct");
                return 1;
            }

            client_data->fd = server.clientfd;
            strcpy(client_data->recvBuffer, server.recvBuffer);

            pthread_create(&tid, NULL, onRequest, (void*)client_data);
        }
    }
    close(server.fd);
}
