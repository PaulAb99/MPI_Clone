
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
static int sock = -1;
static int port = -1;
static char host[64];

struct sockaddr_in address;

void mympi_init()
{   
    //envs
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
        port =5000+rank; ;
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

    //socket part
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port);
    inet_ntop(AF_INET, &address.sin_addr, host, INET_ADDRSTRLEN);

    //2fix
    
    // if (bind(sock, (struct sockaddr *)&address, sizeof(address)) == -1)
    // {
    //     perror("bind");
    //     close(sock);
    //     exit(1);
    // }

    // if (listen(sock, 5) == -1)
    // {
    //     perror("listen");
    //     close(sock);
    //     exit(1);
    // }
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
    //2do
}

void mympi_recv(void *buf, int count, int source, int tag)
{
   //2do
}

void mympi_finalize()
{
    if (sock != -1)
    {
        close(sock);
    }
}