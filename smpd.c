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
#include <wait.h>
#define BUFFER_SIZE 4096
#define TEMP_BUFFER_SIZE 256

int sd_global = -1;
int port;

void run_task(char *recBuf, char *sendBuf)
{
    char *exec = strtok((char *)recBuf, " ");
    int copies = atoi(strtok(NULL, " "));
    if (!exec)
    {
        return;
    }
    if (copies <= 0 || copies > 100)
    {
        snprintf(sendBuf, BUFFER_SIZE, "Invalid nb of copies (%d) should be [1-100]\n", copies);
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

    pid_t pids[copies];
    for (int i = 0; i < copies; i++)
    {
        pid_t pid = fork();
        if (pid == 0)
        {

            dup2(pipefd[1], STDOUT_FILENO);
            dup2(pipefd[1], STDERR_FILENO);
            close(pipefd[0]);
            close(pipefd[1]);

            char env_buf[16];
            snprintf(env_buf, sizeof(env_buf), "%d", i);
            setenv("MYMPI_RANK", env_buf, 1);

            snprintf(env_buf, sizeof(env_buf), "%d", copies);
            setenv("MYMPI_SIZE", env_buf, 1);

            snprintf(env_buf, sizeof(env_buf), "%d", port + 10000);
            setenv("MYMPI_PORT", env_buf, 1);
            setenv("MYMPI_HOST", "127.0.0.1", 1);

            execlp(path, path, NULL);

            perror("execl");
            exit(1);
        }
        else if (pid > 0)
        {
            pids[i] = pid;
        }
        else
        {
            perror("fork");
            close(pipefd[0]);
            close(pipefd[1]);
            return;
        }
    }

    close(pipefd[1]);
    int total = 0;
    int bytes;
    int sendBuf_size = BUFFER_SIZE;
    char temp[TEMP_BUFFER_SIZE];

    while ((bytes = read(pipefd[0], &temp, TEMP_BUFFER_SIZE)) > 0)
    {
        if (total + bytes >= sendBuf_size)
        {
            sendBuf_size += BUFFER_SIZE;
            sendBuf = realloc(sendBuf, sendBuf_size);
        }
        memcpy(sendBuf + total, temp, bytes);
        total += bytes;
    }
    sendBuf[total] = '\0';
    close(pipefd[0]);

    for (int i = 0; i < copies; i++)
    {
        waitpid(pids[i], NULL, 0);
    }
}

void handle_client(int new_sd)
{
    char rec_buffer[BUFFER_SIZE];

    while (1)
    {
        memset(rec_buffer, 0, BUFFER_SIZE);
        char *send_buffer = malloc(BUFFER_SIZE);
        ssize_t bytes = recv(new_sd, rec_buffer, BUFFER_SIZE - 1, 0);

        if (bytes <= 0)
        {
            break;
        }
        printf("Received:%s\n", rec_buffer);

        run_task(rec_buffer, send_buffer);

        if (strlen(send_buffer) == 0)
        {
            snprintf(send_buffer, 64, "No output from task.\n");
        }
        send(new_sd, send_buffer, strlen(send_buffer), 0);
        free(send_buffer);
        close(new_sd);
    }
    

    printf("Client disconnected.\n");
    printf("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");
}

void clean()
{
    if (sd_global != -1)
    {
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
    port = atoi(argv[1]);

    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror("socket");
        exit(1);
    }

    sd_global = sd;
    atexit(clean);
    signal(SIGINT, exit);

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
