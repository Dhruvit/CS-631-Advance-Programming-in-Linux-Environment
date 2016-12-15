/*
 * CGI file
 */

#define _GNU_SOURCE


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "main.h"

int exe_cgi(char* path, char* argument, int newline, int fdin){
    int return_val;

    int fd[2];
    int status;
    pid_t pid;
    int rpipe;
    int wpipe;


    if((return_val = putenv("GATRWAY_INTERFACE=CGI1.1") < 0)){
        perror("Fail to add GATRWAY_INTERFACE into env value:");
        return EXIT_FAILURE;
    }
    
    if((return_val = putenv("SERVER_PROTOCOL=HTTP/1.0") < 0)){
        perror("Fail to add SERVER_PROTOCOL into env value:");
        return EXIT_FAILURE;
    }
    
    if((return_val = setenv("SCRIPT_NAME", path, 1) < 0)){
        perror("Fail to add SCRIPT_NAME env value:");
        return EXIT_FAILURE;
    }
    
    if((return_val = setenv("QUERY_STRING", argument, 1) < 0)){
        perror("Fail to add QUERY_STRING into env value:");
        return EXIT_FAILURE;
    }
    
    pipe2(fd, O_NONBLOCK);
    rpipe = fd[0];
    wpipe = fd[1];
    
    pid = fork();

    if (pid < 0){
        perror("Fail to fork: ");
        exit(EXIT_FAILURE);
    }
    if (pid == 0){
      if(fdin != 0){
	dup2(fdin, STDIN_FILENO);
      }

      close(rpipe);
      dup2(wpipe, STDOUT_FILENO);
	 
      if(newline == 1){
	write(wpipe, "\r\n", 2);
	execlp(path, path, argument, NULL);
      }
      else
	execl(path, path, argument, NULL);

#ifdef DEBUG
    fprintf(stderr, "Debug mode");
#endif
    } 
    else{
      waitpid(pid, &status, 0);
      close(wpipe);
    }

    return rpipe;
}
