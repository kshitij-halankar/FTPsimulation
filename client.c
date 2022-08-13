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

int main(int argc, char *argv[]){
    char ch;
    int mainpipe_fd, server_fd, client_fd;
    pid_t pid = getpid();
    char *server_main_pipe = "/home/halanka/Desktop/asp/ftp/server/server_main_pipe";
    
    char *server_pipe = malloc(1024 * sizeof(char)); // for receiving data form server
    char *client_pipe = malloc(1024 * sizeof(char)); // for sending commands to server
    strcpy(server_pipe, "/home/halanka/Desktop/asp/ftp/server/serverpipe_");
    strcpy(client_pipe, "/home/halanka/Desktop/asp/ftp/server/clientpipe_");
    char *client_pid = malloc(6*sizeof(char));
    sprintf(client_pid, "%d", pid);
    strcat(server_pipe, client_pid);
    strcat(client_pipe,client_pid);
    printf("server_pipe: %s\n",server_pipe);
    printf("client_pipe: %s\n",client_pipe);

	// open main server pipe and write pid
    while((mainpipe_fd = open(server_main_pipe, O_WRONLY))==-1){
        fprintf(stderr, "trying to connect to main pipe\n");
        sleep(5);
    }
    write(mainpipe_fd, &pid, sizeof(pid_t)); // write pid to main server pipe
    close(mainpipe_fd);
    

    //waiting for server to create command pipes
    sleep(2);
    // sprintf(fifoName,"/home/halanka/Desktop/asp/ftp/server/fifo%ld", pid);
    // sleep(1);

	//open server pipe
    while((server_fd = open(server_pipe, O_RDONLY))==-1){
        fprintf(stderr, "trying to connect to server pipe %d\n", pid);
        sleep(5);
    }
    while(1) {
        // int i=0;
        // char server_data[1024];
        while(read(server_fd, &ch, 1) == 1){
            fprintf(stderr, "%c", ch);
            // server_data[i]=ch;
            // i++;
        }
        close(server_fd);
        chmod(client_pipe, 0777);

        while((client_fd = open(client_pipe, O_WRONLY))==-1){
            fprintf(stderr, "trying to connect to client pipe %d\n", pid);
            sleep(5);
        }
        printf("Enter a command: \n");
        char command_buf[1024];
        fgets(command_buf, 1024, stdin);
        command_buf[strcspn(command_buf, "\n")] = 0;
        // fprintf(stderr, "typed command: %s\n",command_buf);
        write(client_fd, command_buf, strlen(command_buf));

        close(client_fd);
    }
    // close(fd);
}
