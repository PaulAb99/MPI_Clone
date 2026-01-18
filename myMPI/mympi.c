
/*
the mpi functions
gcc -c ./myMPI/mympi.c -o ./myMPI/mympi.o
ar rcs ./myMPI/libmympi.a ./myMPI/mympi.o
*/
#include "mympi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUUFER_SIZE 256

// init vals
static int rank = -1;
static int size = -1;
static int serv_sock = -1;
static int port = -1;
static char host[64];
static int *child_arr = NULL;
static int child_sock = -1;

void mympi_init()
{
    // envs
    char *rankEnv = getenv("MYMPI_RANK");
    if (rankEnv)
    {
        rank = atoi(rankEnv);
    }

    char *sizeEnv = getenv("MYMPI_SIZE");
    if (sizeEnv)
    {
        size = atoi(sizeEnv);
    }

    char *portEnv = getenv("MYMPI_PORT");
    if (portEnv)
    {
        port = atoi(portEnv);
    }
    else{
        port = -1;
    }

    char *hostEnv = getenv("MYMPI_HOST");
    if (hostEnv)
    {
        strncpy(host, hostEnv, sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0';
    }
    else
    {
        strncpy(host, "127.0.0.1", sizeof(host) - 1);
    }

    if (rank == -1 || size == -1)
    {
        printf("env vars should be set before launch\n");
        exit(1);
    }


    // socket part split
    if (rank == 0)
    {
        if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

        // set socket behavior to be reused
        int sock_opt = 1; // en
        setsockopt(serv_sock, SOL_SOCKET, SO_REUSEADDR, &sock_opt, sizeof(int));

        if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        {
            printf("rank %d cant bind %s:%d\n", rank, host, port);
            perror("bind");
            close(serv_sock);
            exit(1);
        }

        // eph socket
        // socklen_t addr_len = sizeof(serv_addr);
        // if (getsockname(serv_sock, (struct sockaddr *)&serv_addr, &addr_len) == -1)
        // {
        //     perror("getsockname");
        //     close(serv_sock);
        //     exit(1);
        // }
        // port = ntohs(serv_addr.sin_port);
        // printf("rank 0 is on eph port %d\n", port);
        // char port_buf[16];
        // snprintf(port_buf, sizeof(port_buf), "%d", port);
        // setenv("MYMPI_PORT", port_buf, 1);

        if (listen(serv_sock, 5) == -1)
        {
            perror("listen");
            close(serv_sock);
            exit(1);
        }

        child_arr = malloc(sizeof(int) * size);
        child_arr[0] = -1;

        for (int i = 1; i < size; i++)
        {
            int sock;
            if ((sock = accept(serv_sock, NULL, NULL)) == -1)
            {
                perror("accept");
                close(serv_sock);
                exit(1);
            }
            int child_rank;
            int bytes;

            if ((bytes = recv(sock, &child_rank, sizeof(int), 0)) != sizeof(int))
            {
                perror("recv");
                close(sock);
                close(serv_sock);
                exit(1);
            }

            if (child_rank < 1 || child_rank >= size)
            {
                fprintf(stderr, "invalid child %d\n", child_rank);
                close(sock);
                continue;
            }
            child_arr[child_rank] = sock;
        }
    }

    else // childs
    {
        if ((child_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket");
            exit(1);
        }

        struct sockaddr_in child_addr;
        memset(&child_addr, 0, sizeof(child_addr));

        child_addr.sin_family = AF_INET;

        // eph port 2 childs
        // char *portEnv = getenv("MYMPI_PORT");
        // if (!portEnv)
        // {
        //     printf("no port env var for kids found\n");
        //     exit(1);
        // }

        // port = atoi(portEnv);
        child_addr.sin_port = htons(port);

        inet_pton(AF_INET, host, &child_addr.sin_addr);

        int iter = 100;
        while (connect(child_sock, (struct sockaddr *)&child_addr, sizeof(child_addr)) == -1 && iter > 0)
        {
            usleep(100000);
            iter--;
        }
        if (iter == 0)
        {
            perror("connect");
            close(child_sock);
            exit(1);
        }
        send(child_sock, &rank, sizeof(int), 0);
    }
}

int mympi_rank()
{
    return rank;
}

int mympi_size()
{
    return size;
}

void mympi_send(const void *sBuf, int count, int dst)
{

    if (rank == 0)
    {
        send(child_arr[dst], sBuf, count, 0);
    }
    else
    {
        send(child_sock, sBuf, count, 0);
    }
}

void mympi_recv(void *rBuf, int count, int src)
{

    if (rank == 0)
    {
        recv(child_arr[src], rBuf, count, 0);
    }
    else
    {
        recv(child_sock, rBuf, count, 0);
    }
}

void mympi_finalize()
{
    if (serv_sock != -1)
    {
        close(serv_sock);
    }

    if (child_sock != -1)
    {
        close(child_sock);
    }
    if (child_arr)
    {
        free(child_arr);
    }
}