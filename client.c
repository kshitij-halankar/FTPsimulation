#include <sys/wait.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <errno.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <limits.h>
#include <libgen.h>
#include <dirent.h>
#include <netdb.h>
#include <libgen.h>
#include <time.h>
#include <pwd.h>

void ftp_stor(char *data_pipe, char *filename, int *pos);
void ftp_appe(char *data_pipe, char *filename, int *pos);
void ftp_retr(char *data_pipe, char *filename);

char *server_pipe_g;
char *client_pipe_g;

void exit_handler()
{
    printf("exiting\n");
    exit(0);
}

int main(int argc, char *argv[])
{
    char ch;
    int pos = 0;
    int mainpipe_fd, server_fd, client_fd;
    pid_t pid = getpid();
    char *server_main_pipe = "../server_pipes/server_main_pipe";
    char *data_pipe = malloc(255 * sizeof(char));

    server_pipe_g = malloc(1024 * sizeof(char));
    client_pipe_g = malloc(1024 * sizeof(char));

    signal(SIGINT, exit_handler);
    signal(SIGTSTP, exit_handler);
    signal(SIGUSR1, exit_handler);

    strcpy(data_pipe, "../server_pipes/");

    char *server_pipe = malloc(1024 * sizeof(char)); // for receiving data form server
    char *client_pipe = malloc(1024 * sizeof(char)); // for sending commands to server
    strcpy(server_pipe, "../server_pipes/serverpipe_");
    strcpy(client_pipe, "../server_pipes/clientpipe_");
    char *client_pid = malloc(6 * sizeof(char));
    sprintf(client_pid, "%d", pid);
    strcat(server_pipe, client_pid);
    strcat(client_pipe, client_pid);
    // printf("server_pipe: %s\n", server_pipe);
    // printf("client_pipe: %s\n", client_pipe);

    strcpy(server_pipe_g, server_pipe);
    strcpy(client_pipe_g, client_pipe);

    // open main server pipe and write pid
    while ((mainpipe_fd = open(server_main_pipe, O_WRONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to main pipe\n");
        sleep(5);
    }
    write(mainpipe_fd, &pid, sizeof(pid_t)); // write pid to main server pipe
    close(mainpipe_fd);

    // waiting for server to create command pipes
    sleep(2);
    // sprintf(fifoName,"/home/halanka/Desktop/asp/ftp/server/fifo%ld", pid);
    // sleep(1);

    // open server pipe
    while ((server_fd = open(server_pipe, O_RDONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to server pipe %d\n", pid);
        sleep(5);
    }
    while (1)
    {
        int j = 0;
        char server_response_data[1024];
        while (read(server_fd, &ch, 1) == 1)
        {
            fprintf(stderr, "%c", ch);
            server_response_data[j] = ch;
            j++;
        }
        close(server_fd);
        char *server_resp = malloc(1024 * sizeof(char));
        strcpy(server_resp, server_response_data);
        // printf("server_resp : %s\n", server_resp);
        if (strcmp(server_resp, "server exit") == 0)
        {
            printf("Client exiting.\n");
            exit(0);
        }
        chmod(client_pipe, 0777);

        while ((client_fd = open(client_pipe, O_WRONLY)) == -1)
        {
            fprintf(stderr, "trying to connect to client pipe %d\n", pid);
            sleep(5);
        }
        printf("ftp> ");
        char command_buf[1024];
        fgets(command_buf, 1024, stdin);
        command_buf[strcspn(command_buf, "\n")] = 0;
        // fprintf(stderr, "typed command: %s\n", command_buf);
        write(client_fd, command_buf, 1024);
        close(client_fd);

        char *client_command = malloc(1024 * sizeof(char));
        strcpy(client_command, command_buf);

        char command_delimiter[] = " ";
        char *command_name = malloc(10 * sizeof(char));
        char *command_parameter = malloc(1024 * sizeof(char));
        char *command_pointer = strtok(client_command, command_delimiter);
        if (command_pointer != NULL)
        {
            command_name = command_pointer;
            // printf("command name: %s\n", command_name);
            command_pointer = strtok(NULL, command_delimiter);
            if (command_pointer != NULL)
            {
                command_parameter = command_pointer;
                // printf("command parameter: %s\n", command_parameter);
            }
        }
        else
        {
            // handling
            command_name = client_command;
            // printf("else: command_name: %s client_command: %s\n", command_name, client_command);
        }

        if (strcmp(client_command, "PORT") == 0)
        {
            strcat(data_pipe, command_parameter);
        }
        else if (strcmp(client_command, "STOR") == 0)
        {
            ftp_stor(data_pipe, command_parameter, &pos);
        }
        else if (strcmp(client_command, "APPE") == 0)
        {
            ftp_stor(data_pipe, command_parameter, &pos);
        }
        else if (strcmp(client_command, "RETR") == 0)
        {
            ftp_retr(data_pipe, command_parameter);
        }
        else if (strcmp(client_command, "REST") == 0)
        {
            pos = atoi(command_parameter);
        }
        // printf("waiting for server response\n");
        while ((server_fd = open(server_pipe, O_RDONLY)) == -1)
        {
            fprintf(stderr, "trying to connect to server pipe %d\n", pid);
            sleep(5);
        }
        int i = 0;
        char server_data[1024];
        while (read(server_fd, &ch, 1) == 1)
        {
            fprintf(stderr, "%c", ch);
            server_data[i] = ch;
            i++;
        }
        char *server_data_str = strdup(server_data);
        // printf("server_data_str: %s", server_data_str);
        server_data_str[strcspn(server_data_str, "\n")] = 0;
        // printf("server_data_str: %s", server_data_str);
        if (strcmp(server_data_str, "221 Service closing control connection.") == 0)
        {
            exit(0);
        }
        close(server_fd);
    }
    // close(fd);
}

void ftp_stor(char *data_pipe, char *filename, int *pos)
{
    int port_fd;
    char ch;
    int file_fd;
    while ((port_fd = open(data_pipe, O_WRONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to data pipe %s\n", data_pipe);
        sleep(5);
    }
    if ((file_fd = open(filename, O_RDONLY)) == -1)
    {
        printf("file problem\n");
    }
    lseek(file_fd, *pos, SEEK_SET);
    *pos = 0;
    while (read(file_fd, &ch, 1) == 1)
    {
        write(port_fd, &ch, 1);
        // fprintf(stderr, "%c", ch);
    }
    close(file_fd);
    close(port_fd);
}

void ftp_appe(char *data_pipe, char *filename, int *pos)
{
    int port_fd;
    char ch;
    int file_fd;
    while ((port_fd = open(data_pipe, O_WRONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to data pipe %s\n", data_pipe);
        sleep(5);
    }
    if ((file_fd = open(filename, O_RDONLY)) == -1)
    {
        printf("file problem\n");
    }
    lseek(file_fd, *pos, SEEK_SET);
    *pos = 0;
    while (read(file_fd, &ch, 1) == 1)
    {
        write(port_fd, &ch, 1);
        // fprintf(stderr, "%c", ch);
    }
    close(file_fd);
    close(port_fd);
}

void ftp_retr(char *data_pipe, char *filename)
{
    int port_fd;
    char ch;
    int file_fd;

    while ((port_fd = open(data_pipe, O_RDONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to data pipe %s\n", data_pipe);
        sleep(5);
    }
    if ((file_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0777)) == -1)
    {
        printf("file problem\n");
    }
    while (read(port_fd, &ch, 1) == 1)
    {
        write(file_fd, &ch, 1);
        // fprintf(stderr, "%c", ch);
    }
    close(file_fd);
    close(port_fd);
}