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

void child(pid_t client);
void ftp_user();
void ftp_cwd(char *path);
void ftp_cdup();
void ftp_rein();
void ftp_quit();
void ftp_port(char *pipeName);
void ftp_retr();
void ftp_stor();
void ftp_appe();
void ftp_rest();
void ftp_rnfr();
void ftp_rnto();
void ftp_abor();
void ftp_dele();
void ftp_rmd();
void ftp_mkd();
void ftp_pwd();
void ftp_list();
void ftp_stat();
void ftp_noop();

int main(int argc, char *argv[]){
    int fd, status;
    char ch;
    pid_t pid;
    char *server_dir = argv[1];
    char *server_main_pipe = "/home/halanka/Desktop/asp/ftp/server/server_main_pipe";
    // strcat(server_main_pipe, server_dir);
    // strcat(server_main_pipe,"/server_main_pipe");

    printf("server directory: %s\n",server_dir);
    printf("server main pipe: %s\n",server_main_pipe);

    // unlink("/home/halanka/Desktop/asp/server");
    unlink(server_main_pipe);
    if(mkfifo(server_main_pipe, 0777)){
        perror("main");
        exit(1);
    }
    chmod(server_main_pipe, 0777);
    while(1){
        fprintf(stderr, "Waiting for a client\n");
        fd = open(server_main_pipe, O_RDONLY);
        fprintf(stderr, "Got a client: ");
        read(fd, &pid, sizeof(pid_t));
        fprintf(stderr, "%ld\n", pid);
        if(fork() == 0){
            close(fd);
            child(pid);
        }else{
            waitpid(0, &status, WNOHANG);
        }
    }
}

void child(pid_t pid){

    char *server_pipe = malloc(1024 * sizeof(char)); // for sending data to client
    char *client_pipe = malloc(1024 * sizeof(char)); // for reading commands from client
    strcpy(server_pipe, "/home/halanka/Desktop/asp/ftp/server/serverpipe_");
    strcpy(client_pipe, "/home/halanka/Desktop/asp/ftp/server/clientpipe_");
    char *client_pid = malloc(6*sizeof(char));
    sprintf(client_pid, "%d", pid);
    strcat(server_pipe, client_pid);
    strcat(client_pipe,client_pid);
    printf("server_pipe: %s\n",server_pipe);
    printf("client_pipe: %s\n",client_pipe);

    char newline='\n';
    int mainpipe_fd, server_fd, client_fd, i;

    //creating command pipes
    mkfifo(server_pipe, 0777);
    chmod(server_pipe, 0777);
    mkfifo(client_pipe, 0777);
    chmod(client_pipe, 0777);
    while((server_fd = open(server_pipe, O_WRONLY))==-1){
        fprintf(stderr, "trying to connect to server pipe %d\n", pid);
        sleep(5);
    }

    write(server_fd, "command pipes created", 22);
    write(server_fd, &newline, 1);
    close(server_fd);

    // reading command from client
    while(1){
        sleep(2);
        // fprintf(stderr, "waiting for client command\n");
        while((client_fd = open(client_pipe, O_RDONLY))==-1){
            fprintf(stderr, "trying to connect to client pipe %d\n", pid);
            sleep(5);
        }
        printf("client command: ");
        char client_command[1024], ch;
        int r;
        while((r = read(client_fd, client_command, 1024)) > 0){
            printf("%s\n",client_command);
        }

        // if(strcmp(client_command, "QUIT") == 0) {
        //     close(server_fd);
        //     close(client_fd);
        //     exit(0);
        // }

        // working on command
        char command_delimiter[] = " ";
        char *command_name;
        char *command_parameter;
        char *command_pointer = strtok(client_command, command_delimiter);
        if(command_pointer != NULL) {
            command_name = command_pointer;
            printf("command name: %s\n", command_name);
            command_pointer = strtok(NULL, command_delimiter);
            if(command_pointer != NULL){
                command_parameter = command_pointer;
                printf("command parameter: %s\n", command_parameter);
            }
        } else {

        }

        if(strcmp(command_name, "USER") == 0) {

        } else if(strcmp(command_name, "CWD") == 0) {
            ftp_cwd(command_parameter);
        }else if(strcmp(command_name, "CDUP") == 0) {

        }else if(strcmp(command_name, "REIN") == 0) {

        }else if(strcmp(command_name, "QUIT") == 0) {

        }else if(strcmp(command_name, "PORT") == 0) {

        }else if(strcmp(command_name, "RETR") == 0) {

        }else if(strcmp(command_name, "STOR") == 0) {

        }else if(strcmp(command_name, "APPE") == 0) {

        }else if(strcmp(command_name, "REST") == 0) {

        }else if(strcmp(command_name, "RNFR") == 0) {

        }else if(strcmp(command_name, "RNTO") == 0) {

        }else if(strcmp(command_name, "ABOR") == 0) {

        }else if(strcmp(command_name, "DELE") == 0) {

        }else if(strcmp(command_name, "RMD") == 0) {

        }else if(strcmp(command_name, "MKD") == 0) {

        }else if(strcmp(command_name, "PWD") == 0) {

        }else if(strcmp(command_name, "LIST") == 0) {

        }else if(strcmp(command_name, "STAT") == 0) {

        }else if(strcmp(command_name, "NOOP") == 0) {

        } else {
            //give error
            printf("command didn't match\n");
        }

    }

    // fd = open(server_pipe, O_WRONLY);
    // for(i=0; i < 10; i++){
    // write(fd, fifoName, strlen(fifoName));
    // write(fd, &newline, 1);
    // }
    // close(fd);
    // unlink(fifoName);
    // exit(0);

    /*
    sprintf(fifoName,"/home/halanka/Desktop/asp/ftp/server/fifo%d", pid);
    mkfifo(fifoName, 0777);
    chmod(fifoName, 0777);
    fd = open(fifoName, O_WRONLY);
    for(i=0; i < 10; i++){
        write(fd, fifoName, strlen(fifoName));
        write(fd, &newline, 1);
    }
    close(fd);
    unlink(fifoName);
    exit(0);
    */
}

void ftp_cwd(char *path){
    char new_path[PATH_MAX];
    printf("cwd: %s\n",getcwd(new_path,PATH_MAX));
    chdir(path);
    printf("after cwd: %s\n",getcwd(new_path,PATH_MAX));
}

void ftp_cdup(){
    char new_path[PATH_MAX];
    printf("cwd: %s\n",getcwd(new_path, PATH_MAX));
    chdir("..");
    printf("after cwd: %s\n",getcwd(new_path, PATH_MAX));
}

void ftp_port(char *pipeName){
    char * myfifo = pipeName;
    mkfifo(myfifo, 0666);
}
