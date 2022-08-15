#define _XOPEN_SOURCE 500
#include <ftw.h>
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
#include <libgen.h>
#include <time.h>
#include <pwd.h>

static char *pipe_arr[1024];
static int pipe_count = 0;

int child_count = 0;
int parent_pid;
pid_t client_pid_g = 0;
static pid_t client_pid_arr[1024];
static int client_counter = 0;
static char *pipe_path;

void child(pid_t client, char *server_dir);
void ftp_user();
int ftp_cwd(char *path, char *command_output);
void ftp_cdup(char *command_output);
void ftp_rein();
void ftp_quit();
void ftp_port(char *pipeName, char *command_output);
void ftp_retr(char *data_pipe, char *filename, char *command_output, int *filepos);
void ftp_stor(char *data_pipe, char *filename, char *command_output, int *filepos);
void ftp_appe(char *data_pipe, char *filename, char *command_output, int *filepos);
void ftp_rest(char *file_pos, int *pos, char *command_output);
void ftp_rnfr();
void ftp_rnto(char *oldName, char *newname, char *command_output);
void ftp_abor();
void ftp_dele(char *fileName, char *command_output);
int ftp_rmd(char *filepath);
void ftp_mkd(char *dirName, char *command_output);
void ftp_pwd(char *pwd);
void ftp_list(char *fileName, char *command_output);
void ftp_stat(char *command_output, char *client_pid, int command_count, char *command_param);
void ftp_noop(char *ok);
int unlink_rem(const char *filepath, const struct stat *sb, int type_flag, struct FTW *ftw_buf);

void exit_handler()
{
    int m = 0;
    for (m = 0; m < pipe_count; m++)
    {
        // printf("pipes--------- %s\n", pipe_arr[m]);
        unlink(pipe_arr[m]);
    }

    printf("exiting : %d\n", getpid());
    if (parent_pid == getpid())
    {
        int k = 0;
        printf("client_counter: %d\n", client_counter);
        for (k = 0; k < client_counter; k++)
        {
            pid_t client_pid1 = client_pid_arr[k];
            printf("sending quit signal to %d\n", client_pid1);
            kill(client_pid1, SIGUSR1);
        }
        kill(0, SIGINT);
        sleep(1);
        while (1)
        {
            if (child_count == 0)
            {
                exit(0);
            }
        }
    }
    else
    {
        child_count--;
        exit(0);
    }
}

int main(int argc, char *argv[])
{
    parent_pid = getpid();
    printf("parent: %d\n", parent_pid);
    int fd, status;
    char ch;
    pid_t client_pid;
    char *server_dir = malloc(PATH_MAX * sizeof(char));

    pipe_path = malloc(255 * sizeof(char));

    signal(SIGINT, exit_handler);
    signal(SIGTSTP, exit_handler);

    if (argc == 2)
    {
        strcpy(server_dir, argv[1]);
    }
    else
    {
        getcwd(server_dir, PATH_MAX);
    }

    getcwd(pipe_path, 255);
    strcat(pipe_path, "/../server_pipes/");
    char *server_main_pipe = malloc(1024 * sizeof(char));
    strcpy(server_main_pipe, pipe_path);
    strcat(server_main_pipe, "server_main_pipe");

    chdir(server_dir);

    printf("server directory: %s\n", server_dir);
    printf("server main pipe: %s\n", server_main_pipe);

    unlink(server_main_pipe);
    if (mkfifo(server_main_pipe, 0777))
    {
        perror("main");
        exit(1);
    }
    chmod(server_main_pipe, 0777);
    pipe_arr[pipe_count] = malloc(1024 * sizeof(char));
    strcpy(pipe_arr[pipe_count], pipe_path);
    strcat(pipe_arr[pipe_count], "server_main_pipe");
    // strcpy(server_main_pipe, pipe_arr[pipe_count]);
    pipe_count++;

    // strcpy(server_main_pipe_g, server_main_pipe);
    while (1)
    {
        fprintf(stderr, "Waiting for a client\n");
        fd = open(server_main_pipe, O_RDONLY);
        fprintf(stderr, "Got a client: ");
        read(fd, &client_pid, sizeof(pid_t));
        fprintf(stderr, "%ld\n", client_pid);
        client_pid_arr[client_counter] = client_pid;
        client_counter++;
        // printf("fork client_counter: %d\n", client_counter);
        // printf("fork client_pid_arr: %d\n", client_pid_arr[0]);
        if (fork() == 0)
        {
            child_count++;
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
    char *data_pipe = malloc(255 * sizeof(char));
    int data_port = 0;
    int filepos = 0;

    strcpy(server_pipe, pipe_path);
    strcat(server_pipe, "serverpipe_");

    strcpy(client_pipe, pipe_path);
    strcat(client_pipe, "clientpipe_");

    strcpy(data_pipe, pipe_path);

    char *client_pid = malloc(6 * sizeof(char));
    client_pid_g = pid;
    printf("Client pid _ g: %d\n", client_pid_g);
    sprintf(client_pid, "%d", pid);
    strcat(server_pipe, client_pid);
    strcat(client_pipe, client_pid);
    printf("server_pipe: %s\n", server_pipe);
    printf("client_pipe: %s\n", client_pipe);
    printf("data_pipe: %s\n", data_pipe);

    char newline = '\n';
    int mainpipe_fd, server_fd, client_fd, i;

    // creating command pipes
    mkfifo(server_pipe, 0777);
    chmod(server_pipe, 0777);
    pipe_arr[pipe_count] = malloc(1024 * sizeof(char));
    strcpy(pipe_arr[pipe_count], pipe_path);
    strcat(pipe_arr[pipe_count], "serverpipe_");
    strcat(pipe_arr[pipe_count], client_pid);
    pipe_count++;

    mkfifo(client_pipe, 0777);
    chmod(client_pipe, 0777);
    pipe_arr[pipe_count] = malloc(1024 * sizeof(char));
    strcpy(pipe_arr[pipe_count], pipe_path);
    strcat(pipe_arr[pipe_count], "clientpipe_");
    strcat(pipe_arr[pipe_count], client_pid);
    pipe_count++;

    // strcpy(server_pipe_g, server_pipe);
    // strcpy(client_pipe_g, client_pipe);

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
        client_command[strcspn(client_command, "\n")] = 0;
        printf("after clean: %s\n", client_command);

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
            command_parameter = "";
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
                if (data_port == 1)
                {
                    while (1)
                    {
                        if (data_port == 1)
                        {
                            unlink(data_pipe);
                            data_port = 0;
                            break;
                        }
                        else
                        {
                            sleep(5);
                        }
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
                if (data_port == 1)
                {
                    while (1)
                    {
                        if (data_port == 1)
                        {
                            // if data connection pipes are open close them
                            unlink(data_pipe);
                            data_port = 0;
                            break;
                        }
                        else
                        {
                            sleep(5);
                        }
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
                if (data_port == 0)
                {
                    strcat(data_pipe, command_parameter);
                    ftp_port(data_pipe, command_output);
                    // strcpy(data_pipe_g, data_pipe);
                    data_port = 1;
                }
                else
                {
                    strcpy(command_output, "500 data connection already opened.");
                }
            }
            else if (strcmp(command_name, "RETR") == 0)
            {
                if (data_port == 1)
                {
                    ftp_retr(data_pipe, command_parameter, command_output, &filepos);
                }
                else
                {
                    strcpy(command_output, "500 no data connection present.");
                }
            }
            else if (strcmp(command_name, "STOR") == 0)
            {
                if (data_port == 1)
                {
                    ftp_stor(data_pipe, command_parameter, command_output, &filepos);
                }
                else
                {
                    strcpy(command_output, "500 no data connection present.");
                }
            }
            else if (strcmp(command_name, "APPE") == 0)
            {
                if (data_port == 1)
                {
                    ftp_appe(data_pipe, command_parameter, command_output, &filepos);
                }
                else
                {
                    strcpy(command_output, "500 no data connection present.");
                }
            }
            else if (strcmp(command_name, "REST") == 0)
            {
                if (data_port == 1)
                {
                    ftp_rest(command_parameter, &filepos, command_output);
                    printf("filepos: %d\n", filepos);
                }
                else
                {
                    strcpy(command_output, "500 no data connection present.");
                }
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
                if (data_port == 1)
                {
                    // close data connection
                    unlink(data_pipe);
                    data_port = 0;
                    strcpy(command_output, "226 Closing data connection.");
                }
                else
                {
                    strcpy(command_output, "500 no data connection present.");
                }
            }
            else if (strcmp(command_name, "DELE") == 0)
            {
                ftp_dele(command_parameter, command_output);
            }
            else if (strcmp(command_name, "RMD") == 0)
            {
                ftp_rmd(command_parameter);
                strcpy(command_output, "250 Requested file action okay, completed.");
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
                // printf("command_parameter: %s\n", command_parameter);
                if (data_port == 1)
                {
                    ftp_list(command_parameter, command_output);
                }
                else
                {
                    strcpy(command_output, "500 no data connection present.");
                }
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
        printf("Writing server response\n");
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

int unlink_rem(const char *filepath, const struct stat *sb, int type_flag, struct FTW *ftw_buf)
{
    int rem = remove(filepath);
    if (rem)
    {
        perror(filepath);
    }
    return rem;
}

int ftp_rmd(char *filepath)
{
    return nftw(filepath, unlink_rem, 64, FTW_DEPTH | FTW_PHYS);
}

// int ftp_rmd(const char *path, char *command_output)
// {
//     DIR *d = opendir(path);
//     size_t path_len = strlen(path);
//     int r = -1;
//     if (d)
//     {
//         struct dirent *p;
//         r = 0;
//         while (!r && (p = readdir(d)))
//         {
//             int r2 = -1;
//             char *buf;
//             size_t len;
//             if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
//                 continue;
//             len = path_len + strlen(p->d_name) + 2;
//             buf = malloc(len);
//             if (buf)
//             {
//                 struct stat statbuf;
//                 snprintf(buf, len, "%s/%s", path, p->d_name);
//                 if (!stat(buf, &statbuf))
//                 {
//                     if (S_ISDIR(statbuf.st_mode))
//                         r2 = ftp_rmd(buf, command_output);
//                     else
//                         r2 = unlink(buf);
//                 }
//                 free(buf);
//             }
//             r = r2;
//         }
//         closedir(d);
//     }
//     if (!r)
//     {
//         r = rmdir(path);
//     }
//     strcpy(command_output, "250 Requested file action okay, completed.");
// }

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

void ftp_stat(char *command_output, char *client_pid, int command_count, char *command_param)
{
    // char *host = malloc(16 * sizeof(char));
    if ((command_param != NULL) && (command_param[0] == '\0'))
    {
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

        sprintf(hostname, "211 Server FTP talking to host %s, port \n", host);
        char *user = malloc(1024 * sizeof(char));
        getcwd(curr_server_dir, 255);
        sprintf(user, "211 User: %s  Working directory: %s\n", client_pid, curr_server_dir);
        sprintf(cmd_count, "211 Command count is %d\n", command_count);

        strcpy(command_output, hostname);                                                               // 211 Server FTP talking to host #server_host, port <>
        strcat(command_output, user);                                                                   // 211 User: #client_pid  Working directory: #cwd
        strcat(command_output, "211 The control connection established using pipes (bidirectional)\n"); // 211 The control connection using pipes (bidirectional)
        strcat(command_output, "211 There is no current data connection\n");                            // 211 There is no current data connection / Data connection established using pipes (#pipenames)
        strcat(command_output, "211 using Mode: Stream, Structure: File\n");                            // 211 using Mode Stream, Structure File
        strcat(command_output, cmd_count);                                                              // 211 Command count is ##cc
        strcat(command_output, server_pid);                                                             // 211 Process id is ##server_pid
        strcat(command_output, "211 Authentication type: None\n");                                      // 211 Authentication type: None
        strcat(command_output, "211 *** end of status ***");                                            // 211 *** end of status ***
    }
    else
    {
        DIR *dp;
        struct dirent *dirp;
        struct stat sb;
        char *input = strdup(command_param);
        strcpy(input, command_param);
        if (stat(input, &sb) == 0 && S_ISDIR(sb.st_mode))
        {
            // printf("directory\n");
            dp = opendir(command_param);
            while ((dirp = readdir(dp)) != NULL)
            {
                char *dir_file = dirp->d_name;
                struct stat dir_sb;
                stat(dir_file, &dir_sb);
                char *file_permission = malloc(12 * sizeof(char));
                strcpy(file_permission, (S_ISDIR(dir_sb.st_mode)) ? "d" : "-");
                strcat(file_permission, (S_ISDIR(dir_sb.st_mode)) ? "d" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IRUSR) ? "r" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IWUSR) ? "w" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IXUSR) ? "x" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IRGRP) ? "r" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IWGRP) ? "w" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IXGRP) ? "x" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IROTH) ? "r" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IWOTH) ? "w" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IXOTH) ? "x" : "-");
                struct passwd *pws;
                pws = getpwuid(dir_sb.st_uid);
                char *uname = pws->pw_name;
                char *date_time = ctime(&dir_sb.st_mtime);
                date_time[strcspn(date_time, "\n")] = 0;
                char *file_details = malloc(100 * sizeof(char));
                sprintf(file_details, "%s %d %s %d %s %s\n", file_permission, sb.st_nlink, uname, sb.st_size, date_time, dir_file);
                strcat(command_output, file_details);
            }
            closedir(dp);
        }
        else
        {
            // printf("file\n");
            char *file_permission = malloc(12 * sizeof(char));
            strcpy(file_permission, (S_ISDIR(sb.st_mode)) ? "d" : "-");
            strcat(file_permission, (S_ISDIR(sb.st_mode)) ? "d" : "-");
            strcat(file_permission, (sb.st_mode & S_IRUSR) ? "r" : "-");
            strcat(file_permission, (sb.st_mode & S_IWUSR) ? "w" : "-");
            strcat(file_permission, (sb.st_mode & S_IXUSR) ? "x" : "-");
            strcat(file_permission, (sb.st_mode & S_IRGRP) ? "r" : "-");
            strcat(file_permission, (sb.st_mode & S_IWGRP) ? "w" : "-");
            strcat(file_permission, (sb.st_mode & S_IXGRP) ? "x" : "-");
            strcat(file_permission, (sb.st_mode & S_IROTH) ? "r" : "-");
            strcat(file_permission, (sb.st_mode & S_IWOTH) ? "w" : "-");
            strcat(file_permission, (sb.st_mode & S_IXOTH) ? "x" : "-");
            struct passwd *pws;
            pws = getpwuid(sb.st_uid);
            char *uname = pws->pw_name;
            char *date_time = ctime(&sb.st_mtime);
            date_time[strcspn(date_time, "\n")] = 0;
            char *file_details = malloc(100 * sizeof(char));
            sprintf(file_details, "%s %d %s %d %s %s\n", file_permission, sb.st_nlink, uname, sb.st_size, date_time, input);
            strcpy(command_output, file_details);
        }
    }
}

void ftp_port(char *pipeName, char *command_output)
{
    int port_fd;
    // creating command pipes
    mkfifo(pipeName, 0777);
    chmod(pipeName, 0777);
    pipe_arr[pipe_count] = malloc(1024 * sizeof(char));
    // strcpy(pipe_arr[pipe_count], pipeName);
    strcpy(pipe_arr[pipe_count], pipe_path);
    strcat(pipe_arr[pipe_count], pipeName);
    pipe_count++;
    strcpy(command_output, "225 Data connection open; no transfer in progress.");
}

void ftp_stor(char *data_pipe, char *filename, char *command_output, int *filepos)
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
    // printf("filepos: %d\n", filepos);
    lseek(port_fd, *filepos, SEEK_SET);
    *filepos = 0;
    while (read(port_fd, &ch, 1) == 1)
    {
        write(file_fd, &ch, 1);
        // fprintf(stderr, "%c", ch);
    }
    close(file_fd);
    close(port_fd);
    strcpy(command_output, "250 Requested file action okay, completed.");
}

void ftp_appe(char *data_pipe, char *filename, char *command_output, int *filepos)
{
    int port_fd;
    char ch;
    int file_fd;
    while ((port_fd = open(data_pipe, O_RDONLY)) == -1)
    {
        fprintf(stderr, "trying to connect to data pipe %s\n", data_pipe);
        sleep(5);
    }
    if ((file_fd = open(filename, O_WRONLY | O_APPEND)) == -1)
    {
        printf("file problem\n");
    }
    lseek(port_fd, *filepos, SEEK_SET);
    *filepos = 0;
    while (read(port_fd, &ch, 1) == 1)
    {
        write(file_fd, &ch, 1);
        // fprintf(stderr, "%c", ch);
    }
    close(file_fd);
    close(port_fd);
    strcpy(command_output, "250 Requested file action okay, completed.");
}

void ftp_retr(char *data_pipe, char *filename, char *command_output, int *filepos)
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
    lseek(file_fd, *filepos, SEEK_SET);
    *filepos = 0;
    while (read(file_fd, &ch, 1) == 1)
    {
        write(port_fd, &ch, 1);
        // fprintf(stderr, "%c", ch);
    }
    close(file_fd);
    close(port_fd);
    strcpy(command_output, "250 Requested file action okay, completed.");
}

void ftp_list(char *fileName, char *command_output)
{
    DIR *dp;
    struct dirent *dirp;
    struct stat sb;

    if ((fileName != NULL) && (fileName[0] == '\0'))
    // if (argc == 1)

    {
        dp = opendir("./");
        while ((dirp = readdir(dp)) != NULL)
        {
            char *dir_file = dirp->d_name;
            struct stat dir_sb;
            stat(dir_file, &dir_sb);
            char *file_permission = malloc(12 * sizeof(char));
            strcpy(file_permission, (S_ISDIR(dir_sb.st_mode)) ? "d" : "-");
            strcat(file_permission, (S_ISDIR(dir_sb.st_mode)) ? "d" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IRUSR) ? "r" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IWUSR) ? "w" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IXUSR) ? "x" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IRGRP) ? "r" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IWGRP) ? "w" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IXGRP) ? "x" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IROTH) ? "r" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IWOTH) ? "w" : "-");
            strcat(file_permission, (dir_sb.st_mode & S_IXOTH) ? "x" : "-");
            struct passwd *pws;
            pws = getpwuid(dir_sb.st_uid);
            char *uname = pws->pw_name;
            char *date_time = ctime(&dir_sb.st_mtime);
            date_time[strcspn(date_time, "\n")] = 0;
            char *file_details = malloc(100 * sizeof(char));
            sprintf(file_details, "%s %d %s %d %s %s\n", file_permission, sb.st_nlink, uname, sb.st_size, date_time, dir_file);
            strcat(command_output, file_details);
        }
        closedir(dp);
    }
    else
    {
        char *input = strdup(fileName);
        strcpy(input, fileName);
        if (stat(input, &sb) == 0 && S_ISDIR(sb.st_mode))
        {
            // printf("directory\n");
            dp = opendir(fileName);
            while ((dirp = readdir(dp)) != NULL)
            {
                char *dir_file = dirp->d_name;
                struct stat dir_sb;
                stat(dir_file, &dir_sb);
                char *file_permission = malloc(12 * sizeof(char));
                strcpy(file_permission, (S_ISDIR(dir_sb.st_mode)) ? "d" : "-");
                strcat(file_permission, (S_ISDIR(dir_sb.st_mode)) ? "d" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IRUSR) ? "r" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IWUSR) ? "w" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IXUSR) ? "x" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IRGRP) ? "r" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IWGRP) ? "w" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IXGRP) ? "x" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IROTH) ? "r" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IWOTH) ? "w" : "-");
                strcat(file_permission, (dir_sb.st_mode & S_IXOTH) ? "x" : "-");
                struct passwd *pws;
                pws = getpwuid(dir_sb.st_uid);
                char *uname = pws->pw_name;
                char *date_time = ctime(&dir_sb.st_mtime);
                date_time[strcspn(date_time, "\n")] = 0;
                char *file_details = malloc(100 * sizeof(char));
                sprintf(file_details, "%s %d %s %d %s %s\n", file_permission, sb.st_nlink, uname, sb.st_size, date_time, dir_file);
                strcat(command_output, file_details);
            }
            closedir(dp);
        }
        else
        {
            // printf("file\n");
            char *file_permission = malloc(12 * sizeof(char));
            strcpy(file_permission, (S_ISDIR(sb.st_mode)) ? "d" : "-");
            strcat(file_permission, (S_ISDIR(sb.st_mode)) ? "d" : "-");
            strcat(file_permission, (sb.st_mode & S_IRUSR) ? "r" : "-");
            strcat(file_permission, (sb.st_mode & S_IWUSR) ? "w" : "-");
            strcat(file_permission, (sb.st_mode & S_IXUSR) ? "x" : "-");
            strcat(file_permission, (sb.st_mode & S_IRGRP) ? "r" : "-");
            strcat(file_permission, (sb.st_mode & S_IWGRP) ? "w" : "-");
            strcat(file_permission, (sb.st_mode & S_IXGRP) ? "x" : "-");
            strcat(file_permission, (sb.st_mode & S_IROTH) ? "r" : "-");
            strcat(file_permission, (sb.st_mode & S_IWOTH) ? "w" : "-");
            strcat(file_permission, (sb.st_mode & S_IXOTH) ? "x" : "-");
            struct passwd *pws;
            pws = getpwuid(sb.st_uid);
            char *uname = pws->pw_name;
            char *date_time = ctime(&sb.st_mtime);
            date_time[strcspn(date_time, "\n")] = 0;
            char *file_details = malloc(100 * sizeof(char));
            sprintf(file_details, "%s %d %s %d %s %s\n", file_permission, sb.st_nlink, uname, sb.st_size, date_time, input);
            strcpy(command_output, file_details);
        }
    }
}

void ftp_rest(char *file_pos, int *pos, char *command_output)
{
    // printf("file_pos: %s\n", file_pos);
    if ((*pos = atoi(file_pos)) == 0)
    {
        strcpy(command_output, "501 Syntax error in parameters or arguments.");
        // printf("command_output: %s\n", command_output);
    }
    // printf("pos: %d\n", &pos);
}