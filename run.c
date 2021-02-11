#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <magic.h>
#include <errno.h>
#include <sys/ioctl.h>

#include "run.h"
#include "entry.h"
#include "w3m.c"

#define LEN(x) sizeof(x)/sizeof(*x)


char* terminal[] = {"st", "-e"};
char* fileopener[] = {"xdg-open"};
char* terminaleditor[] = {"vim"};
char* extractcmd[] = {"atool", "-x"};

// if NULL use $SHELL
char* shell = NULL;

magic_t cookie = NULL;

void set_entry_type(entry* file) {
    if (!cookie) {
        cookie = magic_open(MAGIC_MIME);
        magic_load(cookie, NULL);
    }

    const char* output = magic_file(cookie, file->name); // TODO: this doesn't work with links
    errno = 0; //magic_file sets errno for invalid argument, but works for some reason

    if (!output)
        return;
    else if (strstr(output, "image/"))
        file->type = ENTRY_TYPE_IMAGE;
    else if (!strstr(output, "charset=binary") || strstr(output, "inode/x-empty"))
        file->type = ENTRY_TYPE_TEXT;
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

int run_open_file(entry* file,int wait) {
    char** arguments;
    int argumentindex = 0;

    if (file->type == ENTRY_TYPE_FILE)
        set_entry_type(file);

    if (terminaleditor[0] && file->type == ENTRY_TYPE_TEXT) {
        if (!wait) {
            arguments = malloc(sizeof(*arguments)* (LEN(terminal) + LEN(terminaleditor) + 2));

            for (int i = 0; i < LEN(terminal); i++) {
                arguments[argumentindex++] = terminal[i];
            }

        }
        else {
            arguments = malloc(sizeof(*arguments)* (LEN(terminaleditor) + 2));
        }

        for (int i = 0; i < LEN(terminaleditor); i++) {
            arguments[argumentindex++] = terminaleditor[i];
        }

    }
    else {
        arguments = malloc(sizeof(*arguments) * (LEN(fileopener) + 2));

        for (int i = 0; i < LEN(fileopener); i++) {
            arguments[argumentindex++] = fileopener[i];
        }

    }

    arguments[argumentindex++] = file->name;
    arguments[argumentindex++] = NULL;

    run_(arguments,wait);

    free(arguments);

    return wait;
}

int run_open_terminal() {
    char** arguments = malloc(sizeof(char*)*2);
    arguments[0] = terminal[0]; 
    arguments[1] = (char*) NULL;
    int returnvalue = run_(arguments,0);

    free(arguments);

    return returnvalue;
}

void run_extract_file(entry* file) {
    char** arguments = malloc(sizeof(*arguments) * (LEN(extractcmd) + 2));
    int i;
    for (i = 0; i < LEN(extractcmd); i++) {
        arguments[i] = extractcmd[i];
    }
    arguments[i++] = file->name;
    arguments[i++] = (char*) NULL;

    run_(arguments, 1);

    free(arguments);
}

void run_shell() {
    if (!shell) {
        shell = getenv("SHELL");
    }
    char** arguments = malloc(sizeof(*arguments) * 2);
    arguments[0] = shell;
    arguments[1] = (char *)NULL;
    
    run_(arguments, 1);
    
    free(arguments);
}

void run_copy_to_clipboard(char** filenames, int count) {
    int pipefd[2];
    pipe(pipefd);

    int pid = fork();

    if (pid == 0) {
        close(pipefd[1]);
        dup2(pipefd[0], STDIN_FILENO);
        execl("/usr/bin/xclip", "xclip", "-sel", "clip", (char *) NULL);
        exit(EXIT_FAILURE);
    }

    close(pipefd[0]);
    int i;
    for (i = 0; i < count - 1; i++) {
        write(pipefd[1], filenames[i], strlen(filenames[i]));
        write(pipefd[1], " ", 1);
    }
    write(pipefd[1], filenames[i], strlen(filenames[i]));
    close(pipefd[1]);
    return;
}

void run_preview(char *path, entry* file, int begx, int begy, int maxx, int maxy) {
    if (file->type == ENTRY_TYPE_FILE)
        set_entry_type(file);

    if (file->type == ENTRY_TYPE_TEXT) {
        char *preview = malloc(sizeof(*preview) * (maxx * maxy) + 1);
        FILE* f = fopen(file->name, "r");
        int bytesread = fread(preview, sizeof(*preview), maxx * maxy, f);
        preview[bytesread] = '\0';
        fclose(f);

        char *in_ptr, *out_ptr;
        in_ptr = out_ptr = preview;

        while (*in_ptr != '\0') {
            if (*in_ptr == '\r' && *(in_ptr + 1) == '\n') {
                in_ptr++;
            }
            *out_ptr = *in_ptr;
            in_ptr++;
            out_ptr++;
        }

        file->preview = preview;
    }
    else if (file->type == ENTRY_TYPE_IMAGE) {
        w3m_preview_image(path, file->name, begx, begy, maxx, maxy);
    }
}

void run_clear_image_preview(char *path, entry *file, int begx, int begy, int maxx, int maxy) {
    w3m_clear(path, file->name, begx, begy, maxx, maxy);
}

void run_cleanup() {
    w3m_kill();
}
