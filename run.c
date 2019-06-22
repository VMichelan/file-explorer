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

int config_index(char* file) {
        FILE *fp;
        char path[1035];
        int i;

        char* cmd = calloc(sizeof(char)*1024,1);
        if (!cmd) {
            exit(EXIT_FAILURE);
        }
        sprintf(cmd, "file --mime-type \"%s\"",file);
        fp = popen(cmd, "r");
        if (fp == NULL) {
            return -1;
        }

        fgets(path, sizeof(path)-1, fp);

        free(cmd);
        pclose(fp);

        for (i = 0;i < CONFIG_LENGTH;i++) {
            if (strstr(path,config[i].type)) {
                break;
            }
        }

        if (i >= CONFIG_LENGTH) {
            return -1;
        }
        else {
            return i;
        }
}

int run_(char** arguments, int wait) {
    int childPid,status;

    childPid = fork();

    if (childPid < 0) {
        return -1;
    }

    if (childPid == 0) {

        int secondChildPid = fork();
        if (secondChildPid == 0) {
            if (!wait) {
                freopen("/dev/null", "r", stdin);
                freopen("/dev/null", "w", stdout);
                freopen("/dev/null", "w", stderr);
            }
            execvp(arguments[0],arguments);
            exit(EXIT_FAILURE);
        }
        else {
            if (wait) {
                int secondStatus;
                waitpid(secondChildPid,&secondStatus,0); 
            }
            exit(EXIT_SUCCESS);
        }
    }

    waitpid(childPid,&status,0); 

    return 0;
}



int run(char* file,int newterm) {
    int configindex = config_index(file);
    if (configindex == -1) {
        return -1;
    }
    int argumentindex = 0;
    int wait = config[configindex].interm;
    char** arguments = malloc(sizeof(char*)*5);
    if (newterm == 1) {
        wait = 0;
        arguments[0] = malloc(sizeof(char)*1024); 
        arguments[1] = malloc(sizeof(char)*1024); 

        strcpy(arguments[0],"urxvt");
        strcpy(arguments[1],"-e");

        argumentindex = argumentindex + 2;
    }



    arguments[argumentindex] = malloc(sizeof(char)*1024);
    arguments[argumentindex+1] = malloc(sizeof(char)*1024);
    arguments[argumentindex+2] = (char*) NULL;
    strcpy(arguments[argumentindex], config[configindex].program);
    strcpy(arguments[argumentindex+1],file);

    run_(arguments,wait);

    free(arguments[argumentindex]);
    free(arguments[argumentindex+1]);
    if (newterm == 1) {
        free(arguments[0]);
        free(arguments[1]);
    }
    free(arguments);

    return wait;
}

int open_terminal() {
    char** arguments = malloc(sizeof(char*)*2);
    arguments[0] = malloc(sizeof(char)*1024);
    strcpy(arguments[0],TERMINAL);
    arguments[1] = (char*) NULL;
    int returnvalue = run_(arguments,0);
    free(arguments[0]);
    free(arguments);
    return returnvalue;
}
