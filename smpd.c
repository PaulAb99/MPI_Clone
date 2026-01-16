/*

smpd-> handles one client connection at a time sent by mpiex.c

usage:
gcc smpd.c -o smpd
./smpd <port number>

*/

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#define BUFFER_SIZE 4096
#define TEMP_BUFFER_SIZE 256

int sd_global=-1;

void run_task(char *recBuf, char *sendBuf)
{
    char *exec = strtok((char *)recBuf, " ");
    int copies = atoi(strtok(NULL, " "));
    if(!exec||!copies){
        return;
    }
    char path[256];
    snprintf(path, sizeof(path), "./bin/%s", exec);
    printf("Executing %d copies of %s \n", copies, path);
    int pipefd[2];
    if (pipe(pipefd) == -1)
    {
        perror("pipe");
        return;
    }

    
    for (int i = 0; i < copies; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {

            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            execlp(path, path, NULL);

            close(pipefd[0]);
            close(pipefd[1]);
            perror("execl");
            exit(1);
        }
    }
    close(pipefd[1]);
    char temp[TEMP_BUFFER_SIZE];
    int n;
    while ((n=read(pipefd[0], &temp, TEMP_BUFFER_SIZE)) > 0)
    {   
        temp[n] = '\0';
        int remaining = BUFFER_SIZE - strlen(sendBuf) - 1;
        strncat(sendBuf, temp, remaining);
    }

    close(pipefd[0]);
    

    for (int i = 0; i < copies; i++)
    {
        wait(NULL);
    }
}

void handle_client(int new_sd)
{
    char rec_buffer[BUFFER_SIZE];
    char send_buffer[BUFFER_SIZE];

    while (1)
    {
        memset(rec_buffer, 0, BUFFER_SIZE);
        memset(send_buffer, 0, BUFFER_SIZE);
        ssize_t bytes = recv(new_sd, rec_buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
        {
            break; 
        }
        printf("Received:%s\n", rec_buffer);

        run_task(rec_buffer, send_buffer);

        if(strlen(send_buffer) == 0){
            snprintf(send_buffer, sizeof(send_buffer), "No output from task.\n");
        }
        send(new_sd, send_buffer, strlen(send_buffer), 0);
    }

    close(new_sd);
    printf("Client disconnected.\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void clean(){
    if(sd_global!=-1){
        close(sd_global);
    }
    printf("Server shutting down.\n");

}

int main(int argc, char *argv[])
{

    if (argc != 2)
    {
        fprintf(stderr, "Usage: give <port number> where server will start listening\n");
        exit(1);
    }

    int sd, new_sd;
    struct sockaddr_in server, client;
    int client_len = sizeof(client);
    int port = atoi(argv[1]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    sd_global=sd;
    atexit(clean);
    signal(SIGINT,exit);

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);

    if (bind(sd, (struct sockaddr *)&server, sizeof(server)) == -1)
    {
        perror("bind");
        close(sd);
        exit(1);
    }

    if (listen(sd, 5) == -1)
    {
        perror("listen");
        close(sd);
        exit(1);
    }

    printf("Server started on port %d\n", port);

    while (1)
    {

        new_sd = accept(sd, (struct sockaddr *)&client, &client_len);
        if (new_sd == -1)
        {
            perror("accept");
            continue;
        }

        char ip_str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client.sin_addr, ip_str, INET_ADDRSTRLEN);
        printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
        printf("Accepted new client from %s\n", ip_str);
        handle_client(new_sd);
    }
    close(sd);
    return 0;
}
