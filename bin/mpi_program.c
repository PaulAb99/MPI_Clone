/*
simple mpi test program 
gcc ./bin/mpi_program.c -I./myMPI -L./myMPI -lmympi -o ./bin/mpi_program

test no server 2 term
MYMPI_RANK=0 MYMPI_SIZE=2 MYMPI_PORT=5000 ./bin/mpi_program
MYMPI_RANK=1 MYMPI_SIZE=2 MYMPI_PORT=5000 ./bin/mpi_program


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "mympi.h"

#define MES_BUF 128

int main(int argc, char **argv)
{
    mympi_init();

    int rank = mympi_rank();
    int size = mympi_size();

    char *msg=malloc(MES_BUF);
    
    if(rank==0){
        printf("Rank 0 waiting for %d processes\n",size-1);
        

        for(int src=1;src<size;src++){
            memset(msg,0,MES_BUF);
            mympi_recv(msg,MES_BUF,src);
            printf("Received from rank %d: %s\n",src,msg);
            fflush(stdout);
        }
    }
    else{
        snprintf(msg,MES_BUF,"I am rank %d with PID:%d",rank,getpid());
        mympi_send(msg,strlen(msg),0);
    }

    free(msg);
    mympi_finalize();
    return 0;
}