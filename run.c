#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>

#include "run.h"

#define LEN(x) sizeof(x)/sizeof(*x)

char* terminal[] = {"urxvt", "-e"};
char* fileopener[] = {"rifle"};
char* terminaleditor[] = {"vim"};

char istextfile(char* filename) {
    FILE* fp;
    char output[1024];
    char* cmd = calloc(1024, sizeof(*cmd));
    if (!cmd) {
        exit(EXIT_FAILURE);
    }
    sprintf(cmd, "file -ib \"%s\"", filename);
    fp = popen(cmd, "r");
    if (fp == NULL) {
        return -1;
    }

    fgets(output, sizeof(output)-1, fp);

    free(cmd);
    pclose(fp);

    return !strstr(output, "charset=binary") || strstr(output, "inode/x-empty");
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
    char** arguments;
    int wait;
    int argumentindex = 0;

    if (terminaleditor[0] && istextfile(file)) {
        if (newterm) {
            wait = 0;
            arguments = malloc(sizeof(*arguments)* (LEN(terminal) + LEN(terminaleditor) + 2));

            for (int i = 0; i < LEN(terminal); i++) {
                arguments[argumentindex++] = terminal[i];
            }

        }
        else {
            wait = 1;
            arguments = malloc(sizeof(*arguments)* (LEN(terminaleditor) + 2));
        }

        for (int i = 0; i < LEN(terminaleditor); i++) {
            arguments[argumentindex++] = terminaleditor[i];
        }

    }
    else {
        wait = 0;
        arguments = malloc(sizeof(*arguments) * (LEN(fileopener) + 2));

        for (int i = 0; i < LEN(fileopener); i++) {
            arguments[argumentindex++] = fileopener[i];
        }

    }

    arguments[argumentindex++] = file;
    arguments[argumentindex++] = NULL;

    run_(arguments,wait);

    free(arguments);

    return wait;
}

int open_terminal() {
    char** arguments = malloc(sizeof(char*)*2);
    arguments[0] = terminal[0]; 
    arguments[1] = (char*) NULL;
    int returnvalue = run_(arguments,0);
    return returnvalue;
}
