/*
simple mpi test program 
gcc ./bin/mpi_program.c -I./myMPI -L./myMPI -lmympi -o ./bin/mpi_program

*/

#include <stdio.h>
#include <stdlib.h>

#include "mympi.h"

int main(int argc, char **argv)
{
    mympi_init();

    int rank = mympi_rank();
    int size = mympi_size();

    printf("I am process %d of %d\n", rank, size);

    mympi_finalize();
    return 0;
}