/*
Mpiexec clone for sending tasks to remote/local smpd servers.

Usage:
gcc mpiex.c -o mpiex
preferably n1/nn <=100
./mpiex -hosts 1 45.79.112.203:4242 2 program.exe
./mpiex -processes 3 5000 5 5001 6 5002 7000 program
./mpiex -processes 3 5000 50 5001 10 5002 30 mpi_program
./mpiex -processes 3 5000 -5 5001 6 5002 700 mpi_program
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFFER_SIZE 4096
#define MAX_PORT_NB 30000
int remote_host = 0; // host mode 1/process mode 0
int N;

struct sockaddr_in *server_addr;
int *proc_num_arr;

char *target_program;

void parse_cmd_args(int argc, char **argv)
{

    N = atoi(argv[2]);
    if (N <= 0 || N > (MAX_PORT_NB))
    {
        fprintf(stderr, "N must be a positive number lower than %d so it can be registered\n", MAX_PORT_NB);
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
        // process nb
        int proc_pos = 4 + i * 2;
        proc_num_arr[i] = atoi(argv[proc_pos]);

        // address port

        server_addr[i].sin_family = AF_INET;
        server_addr[i].sin_addr.s_addr = htonl(INADDR_ANY);

        int port_pos = 3 + i * 2;

        if (remote_host)
        {
            char tmp[64];
            strcpy(tmp, argv[port_pos]);
            tmp[sizeof(tmp) - 1] = '\0';

            char *ip_str = strtok(tmp, ":");
            char *port_str = strtok(NULL, " ");

            port = atoi(port_str);
            server_addr[i].sin_port = htons(port);

            inet_pton(AF_INET, ip_str, &server_addr[i].sin_addr);

            printf("%s:%d will launch %d copies of %s\n", ip_str, port, proc_num_arr[i], argv[argc - 1]);
        }
        else
        {
            port = atoi(argv[port_pos]);
            server_addr[i].sin_port = htons(port);

            inet_pton(AF_INET, "127.0.0.1", &server_addr[i].sin_addr);

            printf("127.0.0.1:%d will launch %d copies of %s\n", port, proc_num_arr[i], argv[argc - 1]);
        }
    }
}

int connect_to_server(int nb)
{
    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        printf("socket fail\n");
        return sock_fd;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr[nb].sin_addr, ip_str, sizeof(ip_str));


    if (connect(sock_fd, (struct sockaddr *)&server_addr[nb], sizeof(struct sockaddr_in)) == -1)
    {

        printf("~~~ Connection to server %s:%d failed ~~~\n", ip_str, ntohs(server_addr[nb].sin_port));
        close(sock_fd);
        return -1;
    }
    else
    {
        printf("~~~ Connection to server %s:%d success ~~~\n", ip_str, ntohs(server_addr[nb].sin_port));
    }

    return sock_fd;
}

void *thread_server(void *arg)
{
    int i = *((int *)arg);
    free(arg);

    int sock_fd;
    char sendbuf[BUFFER_SIZE];

    if ((sock_fd = connect_to_server(i)) == -1)
    {
        pthread_exit(NULL);
    }

    // send rec logic
    snprintf(sendbuf, sizeof(sendbuf), "%s %d\n", target_program, proc_num_arr[i]);
    sendbuf[BUFFER_SIZE - 1] = '\0';
    send(sock_fd, sendbuf, strlen(sendbuf), 0);

    char *recbuf = malloc(BUFFER_SIZE);
    int bytes, total = 0;
    int recbuf_size = BUFFER_SIZE;
    while ((bytes = recv(sock_fd, recbuf+total, recbuf_size-total-1, 0)) > 0)
    {
        total += bytes;
        if (total+1 >= recbuf_size)
        {
            recbuf_size += BUFFER_SIZE;
            char *tmp = realloc(recbuf, recbuf_size);
            recbuf = tmp;
        }
        
    }
    if (bytes == -1)
    {
        perror("server closed connection");
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &server_addr[i].sin_addr, ip_str, sizeof(ip_str));
    int port = ntohs(server_addr[i].sin_port);

    recbuf[total] = '\0';
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    printf("Received from server %s:%d the following:\n%s", ip_str, port, recbuf);
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
    free(recbuf);

    close(sock_fd);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    int N_expected = atoi(argv[2]) * 2 + 4;
    if (argc < 6 || argc != N_expected || argc > (MAX_PORT_NB))
    {
        fprintf(stderr, "Usage 1: %s -hosts N IP1 N1 IP2 N2 ....  IP_N N_N your_program.exe\n", argv[0]);
        fprintf(stderr, "Usage 2: %s -processes N port_1 N1 port_2 N2 .... port_N N_N your_program.exe\n", argv[0]);
        exit(1);
    }

    parse_cmd_args(argc, argv);
    target_program = argv[argc - 1];

    // send to smpd servers part
    pthread_t threads[N];

    for (int i = 0; i < N; i++)
    {
        int *arg = malloc(sizeof(*arg));
        *arg = i;
        pthread_create(&threads[i], NULL, thread_server, arg);
    }
    for (int i = 0; i < N; i++)
    {
        pthread_join(threads[i], NULL);
    }

    free(server_addr);
    free(proc_num_arr);
    return 0;
}
