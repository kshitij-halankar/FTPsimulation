#include <wait.h>
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

void child(pid_t client, char *server_dir);
void ftp_user();                               // USER
int ftp_cwd(char *path, char *command_output); // CWD
void ftp_cdup(char *command_output);           // CDUP
void ftp_rein();                               // REIN
void ftp_quit();                               // QUIT
void ftp_port(char *pipeName);
void ftp_retr();
void ftp_stor();
void ftp_appe();
void ftp_rest();
void ftp_rnfr();                                                   // RNFR
void ftp_rnto(char *oldName, char *newname, char *command_output); // RNTO
void ftp_abor();
void ftp_dele(char *fileName, char *command_output); // DELE
void ftp_rmd(char *dirName, char *command_output);   // RMD
void ftp_mkd(char *dirName, char *command_output);   // MKD
void ftp_pwd(char *pwd);                             // PWD
void ftp_list();
void ftp_stat(char *command_output, char *client_pid, int command_count, char *command_param);
void ftp_noop(char *ok); // NOOP

int main(int argc, char *argv[])
{
    int fd, status;
    char ch;
    pid_t client_pid;
    char *server_dir = malloc(PATH_MAX * sizeof(char));
    if (argc == 2)
    {
        strcpy(server_dir, argv[1]);
        // server_dir = argv[1];
    }
    else
    {
        getcwd(server_dir, PATH_MAX);
    }
    char *server_main_pipe = "/home/halanka/Desktop/asp/ftp/server_pipes/server_main_pipe";
    // strcat(server_main_pipe, server_dir);
    // strcat(server_main_pipe,"/server_main_pipe");

    chdir(server_dir);

    printf("server directory: %s\n", server_dir);
    printf("server main pipe: %s\n", server_main_pipe);

    // unlink("/home/halanka/Desktop/asp/server");
    unlink(server_main_pipe);
    if (mkfifo(server_main_pipe, 0777))
    {
        perror("main");
        exit(1);
    }
    chmod(server_main_pipe, 0777);
    while (1)
    {
        fprintf(stderr, "Waiting for a client\n");
        fd = open(server_main_pipe, O_RDONLY);
        fprintf(stderr, "Got a client: ");
        read(fd, &client_pid, sizeof(pid_t));
        fprintf(stderr, "%ld\n", client_pid);
        if (fork() == 0)
        {
            close(fd);
            child(client_pid, server_dir);
        }
        else
        {
            waitpid(0, &status, WNOHANG);
        }
    }
}

void child(pid_t pid, char *server_dir)
{
    int command_counter = 0;
    int response_code = 0;
    int login = 0;
    int data_connection = 0;
    char *prev_cmd = malloc(10 * sizeof(char));
    char *rename_file_name = malloc(1024 * sizeof(char));
    char *server_pipe = malloc(1024 * sizeof(char)); // for sending data to client
    char *client_pipe = malloc(1024 * sizeof(char)); // for reading commands from client
    strcpy(server_pipe, "/home/halanka/Desktop/asp/ftp/server_pipes/serverpipe_");
    strcpy(client_pipe, "/home/halanka/Desktop/asp/ftp/server_pipes/clientpipe_");
    char *client_pid = malloc(6 * sizeof(char));
    sprintf(client_pid, "%d", pid);
    strcat(server_pipe, client_pid);
    strcat(client_pipe, client_pid);
    printf("server_pipe: %s\n", server_pipe);
    printf("client_pipe: %s\n", client_pipe);

    char newline = '\n';
    int mainpipe_fd, server_fd, client_fd, i;

    // creating command pipes
    mkfifo(server_pipe, 0777);
    chmod(server_pipe, 0777);
    mkfifo(client_pipe, 0777);
    chmod(client_pipe, 0777);
    while ((server_fd = open(server_pipe, O_WRONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to server pipe %d\n", pid);
        sleep(5);
    }

    write(server_fd, "command pipes created", 22);
    response_code = 230;
    write(server_fd, &newline, 1);
    close(server_fd);

    // reading command from client
    while (1)
    {
        sleep(2);
        // fprintf(stderr, "waiting for client command\n");
        while ((client_fd = open(client_pipe, O_RDONLY)) == -1)
        {
            fprintf(stderr, "trying to connect to client pipe %d\n", pid);
            sleep(5);
        }
        printf("client command: ");
        char ch;
        char *client_command = malloc(1024 * sizeof(char));
        // char *prev_cmd = malloc(10 * sizeof(char));
        // char client_command[1024], ch;
        int r;
        while ((r = read(client_fd, client_command, 1024)) > 0)
        {
            printf("%s\n", client_command);
        }

        // if(strcmp(client_command, "QUIT") == 0) {
        //     close(server_fd);
        //     close(client_fd);
        //     exit(0);
        // }

        // working on command
        char command_delimiter[] = " ";
        char *command_name = malloc(10 * sizeof(char));
        char *command_parameter = malloc(1024 * sizeof(char));
        char *command_pointer = strtok(client_command, command_delimiter);
        char *command_output = malloc(10000 * sizeof(char));
        if (command_pointer != NULL)
        {
            command_name = command_pointer;
            printf("command name: %s\n", command_name);
            command_pointer = strtok(NULL, command_delimiter);
            if (command_pointer != NULL)
            {
                command_parameter = command_pointer;
                printf("command parameter: %s\n", command_parameter);
            }
        }
        else
        {
            // handling
            command_name = client_command;
            printf("else: command_name: %s client_command: %s\n", command_name, client_command);
        }

        // check if USER is first command
        if (login == 0 && strcmp(command_name, "USER") != 0)
        {
            // strcpy(command_output, "Please run USER command to login and establish connection.");
            strcpy(command_output, "530 Not logged in.");
        }
        else
        {
            if (strcmp(command_name, "USER") == 0)
            {
                if (response_code == 230)
                {
                    login = 1;
                    strcpy(command_output, "230 User logged in, proceed.");
                }
                else
                {
                    strcpy(command_output, "530 Not logged in.");
                }
            }
            else if (strcmp(command_name, "CWD") == 0)
            {
                // printf("param length: %d\n", strlen(command_parameter));
                ftp_cwd(command_parameter, command_output);
                printf("%s\n", command_output);
            }
            else if (strcmp(command_name, "CDUP") == 0)
            {
                ftp_cdup(command_output);
                // printf("%s\n", command_output);
            }
            else if (strcmp(command_name, "REIN") == 0)
            {
                while ((server_fd = open(server_pipe, O_WRONLY)) == -1)
                {
                    fprintf(stderr, "trying to connect to server pipe %d\n", pid);
                    sleep(5);
                }
                // printf("command_output: %s\n", command_output);
                strcpy(command_output, "120	Service ready in 1 minutes.");
                write(server_fd, command_output, strlen(command_output));
                write(server_fd, &newline, 1);
                while (1)
                {
                    if (data_connection == 0)
                    {
                        break;
                    }
                    else
                    {
                        sleep(5);
                    }
                }
                login = 0;           // logout
                command_counter = 0; // reset command counter
                chdir(server_dir);   // reset server directory
                close(server_fd);
                strcpy(command_output, "220	Service ready for new user.");
            }
            else if (strcmp(command_name, "QUIT") == 0)
            {
                // wait for any open data transfers
                while (1)
                {
                    if (data_connection == 0)
                    {
                        // if data connection pipes are open close them
                        break;
                    }
                    else
                    {
                        sleep(5);
                    }
                }
                // send quit message to client
                while ((server_fd = open(server_pipe, O_WRONLY)) == -1)
                {
                    fprintf(stderr, "trying to connect to server pipe %d\n", pid);
                    sleep(5);
                }
                strcpy(command_output, "221 Service closing control connection.");
                write(server_fd, command_output, strlen(command_output));
                write(server_fd, &newline, 1);

                // close named pipes
                close(server_fd);
                close(client_fd);
                unlink(server_pipe);
                unlink(client_pipe);

                // exit child
                exit(0);
            }
            else if (strcmp(command_name, "PORT") == 0)
            {
            }
            else if (strcmp(command_name, "RETR") == 0)
            {
            }
            else if (strcmp(command_name, "STOR") == 0)
            {
            }
            else if (strcmp(command_name, "APPE") == 0)
            {
            }
            else if (strcmp(command_name, "REST") == 0)
            {
            }
            else if (strcmp(command_name, "RNFR") == 0)
            {
                strcpy(rename_file_name, command_parameter);
                // printf("rnfr prev_cmd %s\n", prev_cmd);
                // printf("rnfr rename_file_name %s\n", rename_file_name);
            }
            else if (strcmp(command_name, "RNTO") == 0)
            {
                // printf("prev_cmd %s\n", prev_cmd);
                if (strcmp(prev_cmd, "RNFR") == 0)
                {
                    ftp_rnto(rename_file_name, command_parameter, command_output);
                }
                else
                {
                    strcpy(command_output, "503	Bad sequence of commands.");
                }
            }
            else if (strcmp(command_name, "ABOR") == 0)
            {
            }
            else if (strcmp(command_name, "DELE") == 0)
            {
                ftp_dele(command_parameter, command_output);
            }
            else if (strcmp(command_name, "RMD") == 0)
            {
                ftp_rmd(command_parameter, command_output);
            }
            else if (strcmp(command_name, "MKD") == 0)
            {
                ftp_mkd(command_parameter, command_output);
            }
            else if (strcmp(command_name, "PWD") == 0)
            {
                char path[1024];
                ftp_pwd(path);
                printf("current path: %s\n", path);
                // strcpy(command_output, "current path: ");
                strcpy(command_output, path);
                printf("%s\n", command_output);
            }
            else if (strcmp(command_name, "LIST") == 0)
            {
            }
            else if (strcmp(command_name, "STAT") == 0)
            {

                ftp_stat(command_output, client_pid, command_counter, command_parameter);
                printf("%s\n", command_output);
            }
            else if (strcmp(command_name, "NOOP") == 0)
            {
                char ok[10];
                ftp_noop(ok);
                strcpy(command_output, ok);
                printf("%s\n", command_output);
            }
            else
            {
                // invalid command
                strcpy(command_output, "502	Command not implemented.");
                strcat(command_output, command_name);
            }
        }

        while ((server_fd = open(server_pipe, O_WRONLY)) == -1)
        {
            fprintf(stderr, "trying to connect to server pipe %d\n", pid);
            sleep(5);
        }
        printf("command_output: %s\n", command_output);
        write(server_fd, command_output, strlen(command_output));
        write(server_fd, &newline, 1);
        close(server_fd);
        command_counter++;
        strcpy(prev_cmd, command_name);
        // printf("x prev_cmd %s\n", prev_cmd);
    }
}

int ftp_cwd(char *path, char *command_output)
{
    DIR *dir_path = opendir(path);
    if (dir_path)
    {
        chdir(path);
        closedir(dir_path);
        strcpy(command_output, "250	Requested file action okay, completed.");
        // char *new_path = malloc(255 * sizeof(char));
        // getcwd(new_path, 255);
        // strcat(command_output, new_path);
        // printf("command_output: %s\n", command_output);
    }
    else
    {
        strcpy(command_output, "501	Syntax error in parameters or arguments.");
    }
}

void ftp_cdup(char *command_output)
{
    chdir("..");
    // strcpy(command_output, "200 The requested action has been successfully completed.");
    char *new_path = malloc(255 * sizeof(char));
    getcwd(new_path, 255);
    strcpy(command_output, new_path);
}
void ftp_pwd(char *pwd)
{
    getcwd(pwd, 1024);
}

void ftp_noop(char *ok)
{
    strcpy(ok, "OK");
}

void ftp_dele(char *fileName, char *command_output)
{
    char *input_file = strdup(fileName);
    char *fname = basename(input_file);
    // printf("filename: %s \n", fname);
    if (remove(fname) == 0)
    {
        strcpy(command_output, "250	Requested file action okay, completed.");
        // sprintf(command_output, "File %s deleted successfully", fname);
    }
    else
    {
        strcpy(command_output, "450	Requested file action not taken.");
        // sprintf(command_output, "Unable to delete the file %s", fname);
    }
}

void ftp_rmd(char *dirName, char *command_output)
{
    char *dir = dirName;
    int ret = rmdir(dir);
    if (ret == 0)
    {
        sprintf(command_output, "Directory %s removed successfully", dirName);
    }
    else
    {
        sprintf(command_output, "Unable to remove directory %s", dirName);
    }
}

void ftp_mkd(char *dirName, char *command_output)
{
    int check;
    char *dirname = dirName;
    check = mkdir(dirname, 0777);
    if (!check)
    {
        // char *new_path = malloc(255 * sizeof(char));
        // getcwd(new_path, 255);
        // strcpy(command_output, new_path);
        char buf[255];
        char *res = realpath(dirName, buf);
        if (res)
        {
            strcpy(command_output, buf);
        }
        strcat(command_output, " created.");
        // sprintf(command_output, "Directory created");
    }
    else
    {
        strcpy(command_output, "501	Syntax error in parameters or arguments.");
        // sprintf(command_output, "Unable to create directory");
    }
}

void ftp_rnto(char *oldName, char *newname, char *command_output)
{
    char *ts1 = strdup(oldName);
    char *old_name = basename(ts1);
    char *ts2 = strdup(newname);
    char *new_name = basename(ts2);
    int result = rename(old_name, new_name);
    if (result == 0)
    {
        strcpy(command_output, "250	Requested file action okay, completed.");
        // sprintf(command_output, "The file %s is renamed successfully to %s", oldName, newname);
    }
    else
    {
        strcpy(command_output, "553	Requested action not taken. File name not allowed.");
        // sprintf(command_output, "The file %s could not be renamed to %s", oldName, newname);
    }
}

void ftp_port(char *pipeName)
{
    char *myfifo = pipeName;
    mkfifo(myfifo, 0666);
}

void ftp_stat(char *command_output, char *client_pid, int command_count, char *command_param)
{
    // char *host = malloc(16 * sizeof(char));
    char hostbuf[255];
    char host[1024];
    host[1023] = '\0';
    char *server_pid = malloc(30 * sizeof(char));
    char *hostname = malloc(1024 * sizeof(char));
    char *curr_server_dir = malloc(255 * sizeof(char));
    char *cmd_count = malloc(100 * sizeof(char));
    pid_t serpid = getpid();
    sprintf(server_pid, "211 Process id is %d\n", serpid);

    gethostname(host, 1023);
    // printf("Hostname: %s\n", host);

    sprintf(hostname, "211 Server FTP talking to host %s, port \n", host);
    char *user = malloc(1024 * sizeof(char));
    getcwd(curr_server_dir, 255);
    sprintf(user, "211 User: %s  Working directory: %s\n", client_pid, curr_server_dir);
    sprintf(cmd_count, "211 Command count is %d\n", command_count);

    strcpy(command_output, hostname); // 211 Server FTP talking to host #server_host, port <>
    strcat(command_output, user); // 211 User: #client_pid  Working directory: #cwd
    strcat(command_output, "211 The control connection established using pipes (bidirectional)\n"); // 211 The control connection using pipes (bidirectional)
    strcat(command_output, "211 There is no current data connection\n"); // 211 There is no current data connection / Data connection established using pipes (#pipenames)
    strcat(command_output, "211 using Mode: Stream, Structure: File\n"); // 211 using Mode Stream, Structure File
    strcat(command_output, cmd_count); // 211 Command count is ##cc
    strcat(command_output, server_pid); // 211 Process id is ##server_pid
    strcat(command_output, "211 Authentication type: None\n"); // 211 Authentication type: None
    strcat(command_output, "211 *** end of status ***"); // 211 *** end of status ***
}

