#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#include "run.h"

int run(char* cmd,char* argument){
    int childPid,status;
    char** arguments = (char**) malloc(sizeof(char)*3);
    arguments[0] = malloc(sizeof(char)*6);
    strcpy(arguments[0], cmd);
    arguments[1] = argument;
    arguments[2] = (char*) NULL;
    childPid = fork();
    if(fork < 0){
        fprintf(stderr,"FAILED FORK");
        exit(EXIT_FAILURE);
    }
    if(childPid == 0)
        execvp(arguments[0],arguments);

    return 0;
}

int open_terminal(){
    return run("i3-sensible-terminal",NULL);
}
    
