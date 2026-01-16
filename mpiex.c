/*

./mpiex -hosts 1 45.79.112.203:4242 2 program.exe
./mpiex -processes 3 5000 5 5001 6 5002 7 program.exe

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096
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
    server_addr = malloc(N * sizeof(struct sockaddr_in));
    proc_num_arr = malloc(N * sizeof(int));

    if (strcmp(argv[1], "-hosts") == 0)
    {
        printf("Mpiex Mode: hosts\n");
        remote_host = 1;
    }
    else if (strcmp(argv[1], "-processes") == 0)
    {
        printf("Mpiex Mode: processes\n");
        remote_host = 0;
    }
    else
    {
        fprintf(stderr, "First argument must be -hosts or -processes\n");
        exit(1);
    }

    int port;
    for (int i = 0; i < N; i++) // add data to server struct
    {
        server_addr[i].sin_family = AF_INET;
        proc_num_arr[i] = atoi(argv[4 + i * 2]);
        server_addr[i].sin_addr.s_addr = htonl(INADDR_ANY);
        if (remote_host)
        {
            char *ip_str = strtok(argv[3 + i * 2], ":");
            char *port_str = strtok(NULL, ":");
            port = atoi(port_str);
            server_addr[i].sin_port = htons(port);
            inet_pton(AF_INET, ip_str, &server_addr[i].sin_addr);
                printf("%s:%d will launch %d copies of %s\n", ip_str, port, proc_num_arr[i], argv[argc - 1]);
        }
        else
        {
            port = atoi(argv[3 + i * 2]);
            server_addr[i].sin_port = htons(port);
            inet_pton(AF_INET, "127.0.0.1", &server_addr[i].sin_addr);
                printf("127.0.0.1:%d will launch %d copies of %s\n", port, proc_num_arr[i], argv[argc - 1]);
        }
    }
}

int connect_to_server(int nb)
{
    int sock_fd;
    int server_ip;
    // Create socket
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("Socket creation failed\n");
        return sock_fd;
    }

    char ip_str[16];
    inet_ntop(AF_INET,&server_addr[nb].sin_addr,ip_str,sizeof(ip_str));
    // Connect to server
    if (connect(sock_fd, (struct sockaddr *)&server_addr[nb], sizeof(struct sockaddr)) == -1)
    {   
        
        printf("~~~ Connection to server %s:%d failed ~~~\n", ip_str, ntohs(server_addr[nb].sin_port));
        close(sock_fd);
        return -1;
    }

    printf("~~~ Connection to server %s:%d success ~~~\n", ip_str, ntohs(server_addr[nb].sin_port));
    return sock_fd;
}

int main(int argc, char *argv[])
{
    if (argc < 6 || argc != (atoi(argv[2]) * 2 + 4) || argc >(49150 - 1024)) // 2do more robustly
    {
        fprintf(stderr, "Usage 1: %s -hosts N IP1 N1 IP2 N2 ....  IP_N N_N your_program.exe\n", argv[0]);
        fprintf(stderr, "Usage 2: %s -processes N port_1 N1 port_2 N2 .... port_N N_N your_program.exe\n", argv[0]);
        exit(1);
    }

    parse_cmd_args(argc, argv);

    char *target_program = argv[argc - 1];

    for (int i = 0; i < N; i++)
    {
        int sock_fd;
        char sendbuf[BUFFER_SIZE];
        char recbuf[BUFFER_SIZE];

        if ((sock_fd = connect_to_server(i)) == -1)
        {
            continue;
        }

        snprintf(sendbuf,sizeof(sendbuf),"%s %d\n", target_program, proc_num_arr[i]);
        send(sock_fd, sendbuf, strlen(sendbuf), 0);

        ssize_t bytes = recv(sock_fd, recbuf, BUFFER_SIZE - 1, 0);
        if (bytes <= 0)
        {
            printf("Server closed connection.\n");
            continue;
        }

        recbuf[bytes] = '\0';
        printf("Recieved:\n %s", recbuf);
        close(sock_fd);
    }

    free(server_addr);
    free(proc_num_arr);
    return 0;
}
