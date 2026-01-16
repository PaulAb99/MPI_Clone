/*

Example: TCP Sockets - Client
Echo Client: sends lines to an Echo Server.

Can be used as a client to EchoServer and EchoServerConcurrent.

First start EchoServer:
EchoServer 6789
Then Start EchoClient:
EchoClient 127.0.0.1 6789

Public Echo Server that can be used for testing: tcpbin.com
Use with:
EchoClient 45.79.112.203 4242

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024
#define PORT_MPIEX 1024
int remote_host = 0; // host mode 1/process mode 0
int N;

struct sockaddr_in *server_addr;
int *proc_num_arr;

void parse_cmd_args(int argc, char **argv)
{

    N = atoi(argv[2]);
    if (N <= 0 || N > (49150 - 1024))
    {
        fprintf(stderr, "N must be a positive number lower than %d so it can be registered\n", 49150 - 1024);
        exit(1);
    }
    server_addr = malloc(N * sizeof(sockaddr_in));
    proc_arr = malloc(N * sizeof(int));

    if (strcmp(argv[1], "-hosts") == 0)
    {
        printf("Client Mode: hosts\n");
        remote_host = 1;
    }
    else if (strcmp(argv[1], "-processes") == 0)
    {
        printf("Client Mode: processes\n");
        remote_host = 0;
    }
    else
    {
        fprintf(stderr, "First argument must be -hosts or -processes\n");
        exit(1);
    }

    for (int i = 0; i < N; i++) // add data to server struct
    {
        server_addr[i].sin_family = AF_INET;
        proc_num_arr[i] = atoi(argv[4 + i * 2]);
        server.sin_addr.s_addr = htonl(INADDR_ANY);
        if (remote_host)
        {
            server_addr.sin_port = htons(5000);
            strcpy(server_addr[i].sin_addr, argv[3 + i * 2]);
            printf("Host %s will run %d processes on port %d\n", servers[i].ip, servers[i].num, servers[i].port);
        }
        else
        {
            server_addr.sin_port = htons(1025 + i);
            strcpy(server_addr[i].sin_addr, "127.0.0.1");
            printf("Localhost will run %d processes on port %d\n", servers[i].sin, servers[i].num);
        }
    }
}

// debug mode print to file a report with mutex

int main(int argc, char *argv[])
{
    if (argc < 6 || argc != (atoi(argv[2]) * 2 + 4) || argc >) // 2do more robustly
    {
        fprintf(stderr, "Usage 1: %s -hosts N IP1 N1 IP2 N2 ....  IP_N N_N your_program.exe\n", argv[0]);
        fprintf(stderr, "Usage 2: %s -processes N port_1 N1 port_2 N2 .... port_N N_N your_program.exe\n", argv[0]);
        exit(1);
    }

    parse_cmd_args(argc, argv);

    int sock_fd;
    struct sockaddr_in server_addr; /// fac parse simplu cu select si dupa un upgrade de queue
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    // Server IP was given as string; Convert IP string to binary
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0)
    {
        fprintf(stderr, "Expect server IP in format ddd.ddd.ddd.ddd\n");
        close(sock_fd);
        exit(1);
    }

    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("connect");
        close(sock_fd);
        exit(1);
    }

    printf("Connected to server\n");

    while (1) // repeatedly read lines
    {
        printf("Enter message: ");
        if (!fgets(buffer, BUFFER_SIZE, stdin))
        {
            break; // EOF or error
        }

        // Send message
        send(sock_fd, buffer, strlen(buffer), 0);

        // Receive echo
        ssize_t bytes = recv(sock_fd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
        {
            printf("Server closed connection.\n");
            break;
        }

        buffer[bytes] = '\0';
        printf("Echoed: %s", buffer);
    }

    close(sock_fd);
    free(server_addr);
    free(proc_num_arr);
    return 0;
}