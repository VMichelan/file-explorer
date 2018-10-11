#ifndef RUN_H_
#define RUN_H_

#define TERMINAL "urxvt"
#define TERMINAL_EXEC "-e"

typedef struct configstruct configstruct;

struct configstruct{
    char* type;
    char* program;
    int interm;
};

int run(char* file,int newterm);
int open_terminal();

#endif


