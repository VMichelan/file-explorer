#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#include "run.h"

#define CONFIG_LENGTH 3

configstruct config[] = {
    {"text","vim",1},
    {"image","feh",0},
    {"pdf","mupdf",0}
};

int run(char* file){
    int childPid,status,i= 0;
    char** arguments = (char**) malloc(sizeof(char)*3);
    arguments[0] = malloc(sizeof(char)*1024);

    if(file){
        FILE *fp;
        char path[1035];

        char* cmd = calloc(sizeof(char)*1024,1);
        if(!cmd){
            exit(EXIT_FAILURE);
        }
        cmd = strcat(cmd,"file --mime-type \"");
        cmd = strcat(cmd,file);
        cmd = strcat(cmd,"\"");
        fp = popen(cmd, "r");
        if (fp == NULL) {
            return 1;
        }

        fgets(path, sizeof(path)-1, fp);

        for(i = 0;i < CONFIG_LENGTH;i++)
            if(strstr(path,config[i].type))
                break;

        if(i >= CONFIG_LENGTH)
            return 1;

        free(cmd);
        pclose(fp);

        strcpy(arguments[0], config[i].program);
        arguments[1] = malloc(sizeof(char)*1024);
        strcpy(arguments[1],file);
        arguments[2] = (char*) NULL;
    }
    else{
        strcpy(arguments[0],"urxvt");
        arguments[1] = (char*) NULL;
    }

    childPid = fork();

    if(childPid < 0){
        fprintf(stderr,"FAILED FORK");
        exit(EXIT_FAILURE);
    }
    if(childPid == 0){
        execvp(arguments[0],arguments);
        exit(EXIT_FAILURE);
    }
    else{
        if(file && config[i].interm)
            waitpid(childPid,&status,0); 

    }
    free(arguments[0]);
    free(arguments[1]);
    free(arguments);

    return 0;
}

int open_terminal(){
    return run(NULL);
}
