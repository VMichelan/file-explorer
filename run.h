#ifndef RUN_H_
#define RUN_H_

#define TERMINAL "x-terminal-emulator"

typedef struct configstruct configstruct;

struct configstruct{
    char* type;
    char* program;
    int interm;
};

int run(char* file);
int open_terminal();

#endif


