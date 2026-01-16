# MPI_Clone
A simplified MPI infrastructure on sockets consisting of:
smpd-> a process manager that can be deployed on multiple hosts recieving requests to launch programs
mpiex-> client program that transmits requests to the smpd server
