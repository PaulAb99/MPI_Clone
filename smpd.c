/*

Example: TCP Sockets - Iterative Server
Echo Server: replies back every received message
Iterative Server: handles one client connection at a time

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

void handle_client(int new_sd);

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Usage: give <port number> where server will start listening\n");
        exit(1);
    }

    int sd, new_sd;
    struct sockaddr_in server, client;
    int client_len = sizeof(client);

    int port = atoi(argv[1]);

    // Create server socket (listening socket)
    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // Bind address/port
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    // bind address of server socket
    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("bind");
        close(sd);
        exit(1);
    }

    // Listen for clients, set backlog=5
    if (listen(sd, 5) == -1)
    {
        perror("listen");
        close(sd);
        exit(1);
    }

    printf("Server started on port %d\n", port);

    // Main loop: accept clients sequentially
    while (1)
    {

        // Accept a client, get new_sd the communication socket
        new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
        if (new_sd == -1)
        {
            perror("accept");
            continue;
        }

        // only for printing: convert binary address in string ddd.ddd.ddd.ddd
        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ip_str, INET_ADDRSTRLEN);

        printf("Accepted new client from %s\n", ip_str);
        handle_client(new_sd);
    }
    close(sd);
    return 0;
}

void handle_client(int new_sd)
{
    char buffer[BUFFER_SIZE];

    // Read lines and echo back
    // Use new_sd the communication socket to exchange data with client
    while (1)
    {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes = recv(new_sd, buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
        {
            break; // client closed or error
        }

        printf("Received: %s\n", buffer);

        send(new_sd, buffer, bytes, 0);
    }

    close(new_sd);
    printf("Client disconnected.\n");
}