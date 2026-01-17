
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

struct sockaddr_in serv_addr;

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
        port = atoi(portEnv) + rank;
    }
    else
    {
        port = 5000 + rank;
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

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(port);
    inet_ntop(AF_INET, &serv_addr.sin_addr, host, INET_ADDRSTRLEN);
   
    // socket part split
    if (rank == 0)
    {
        if ((serv_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket");
            exit(1);
        }

        if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        {
            perror("bind");
            close(serv_sock);
            exit(1);
        }

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
            if (child_arr[i] = accept(serv_sock, NULL, NULL) == -1)
            {
                perror("accept");
                close(serv_sock);
                exit(1);
            }
        }
    }

    else // childs
    {
        if ((child_sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
            perror("socket");
            exit(1);
        }

        int iter=10;
        while(connect(child_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1  && iter>0)
        {
            usleep(100000);
            iter--;
        }
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

void mympi_send(const void *buf, int count, int dest, int tag)
{
    // 2do
}

void mympi_recv(void *buf, int count, int source, int tag)
{
    // 2do
}

void mympi_finalize()
{
    if (serv_sock != -1)
    {
        close(serv_sock);
    }

    if(child_sock != -1)
    {
        close(child_sock);
    }
}