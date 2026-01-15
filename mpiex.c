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
int mode_host = 0; // host mode 0/process mode 1
int N;

typedef struct host_or_proc
{
    char ip[64];
    int port;
    int num;
} server_data;
server_data *servers;
    
int parse_cmd_args(int argc, char **argv)
{

    N = atoi(argv[2]);
    servers = malloc(N * sizeof(server_data));
 
    if (strcmp(argv[1], "-hosts") == 0)
    {
        mode_host = 1;
    }
    else if (strcmp(argv[1], "-processes") == 0)
    {
        mode_host = 0;
    }
    else
    {
        fprintf(stderr, "First argument must be -hosts or -processes\n");
        exit(1);
    }

    if (mode_host)
    {
        for (int i = 0; i < N; i++)
        {
            strcpy(servers[i].ip, argv[3 + i * 2]);
            servers[i].port=6000;
            servers[i].num = atoi(argv[4 + i * 2]);
            printf("Host %s will run %d processes on port %d\n",servers[i].ip,servers[i].num,servers[i].port);
        }
    }
    else{
        for (int i = 0; i < N; i++)
        {   
            strcpy(servers[i].ip, "127.0.0.1");
            servers[i].port=6000+i;
            servers[i].num = atoi(argv[4 + i * 2]);
            printf("Port %d will run %d processes on localhost\n",servers[i].port,servers[i].num);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 6 || argc != (atoi(argv[2]) * 2 + 4)) // 2do more robustly
    {
        fprintf(stderr, "Usage 1: %s -hosts N IP1 N1 IP2 N2 ....  IP_N N_N your_program.exe\n", argv[0]);
        fprintf(stderr, "Usage 2: %s -processes N port_1 N1 port_2 N2 .... port_N N_N your_program.exe\n", argv[0]);
        exit(1);
    }

    parse_cmd_args(argc, argv);

    char *server_ip = argv[1];
    int port = atoi(argv[2]);

    int sock_fd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

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
    free(servers);
    return 0;
}