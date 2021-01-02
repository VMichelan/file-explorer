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

#define LEN(x) sizeof(x)/sizeof(*x)

#define W3MIMGDISPLAYPATH "/usr/lib/w3m/w3mimgdisplay"

char* terminal[] = {"st", "-e"};
char* fileopener[] = {"xdg-open"};
char* terminaleditor[] = {"vim"};
char* extractcmd[] = {"atool", "-x"};

// if NULL use $SHELL
char* shell = NULL;

magic_t cookie = NULL;

struct w3m_config {
    int pid;
    FILE *fout;
    FILE *fin;
    int max_width_pixels;
    int max_height_pixels;
    int fontx;
    int fonty;
};

struct w3m_config w3m_config;

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

void w3m_get_sizes(int maxx, int maxy) {
    struct winsize screen_size;

    ioctl(0, TIOCGWINSZ, &screen_size);
    int max_width_pixels, max_height_pixels;
    int fontx = screen_size.ws_xpixel / screen_size.ws_col;
    int fonty = screen_size.ws_ypixel / screen_size.ws_row;

    w3m_config.fontx = fontx;
    w3m_config.fonty = fonty;
    w3m_config.max_width_pixels = maxx * fontx;
    w3m_config.max_height_pixels = maxy * fonty;
}

void w3m_start() {
    int inpipe[2], outpipe[2];

    pipe(inpipe);
    pipe(outpipe);

    int status;
    int pid = fork();

    if (pid == 0) {
        close(outpipe[1]);
        close(inpipe[0]);
        dup2(outpipe[0], STDIN_FILENO);
        dup2(inpipe[1], STDOUT_FILENO);
        dup2(inpipe[1], STDERR_FILENO);
        execl(W3MIMGDISPLAYPATH, "w3mimgdisplay", (char *) NULL);
        exit(1);
    }

    close(outpipe[0]);
    close(inpipe[1]);

    FILE *fin, *fout;
    fout = fdopen(outpipe[1], "w");
    fin = fdopen(inpipe[0], "r");

    w3m_config.pid = pid;
    w3m_config.fout = fout;
    w3m_config.fin = fin;
}

void w3m_kill() {
    if (!w3m_config.pid)
        return;
    int status;
    kill(w3m_config.pid, SIGTERM);
    waitpid(w3m_config.pid, &status, 0);
}

int w3m_get_img_info(char *path, entry *file, int *width, int *height) {
    char buffer[1024];
    int img_width, img_height;
    fprintf(w3m_config.fout, "5;%s/%s\n", path, file->name);
    fflush(w3m_config.fout);
    fgets(buffer, 1024, w3m_config.fin);
    if (sscanf(buffer, "%d %d", &img_width, &img_height) == 0) {
        return 0;
    }

    if (img_width > w3m_config.max_width_pixels) {
        img_height = (img_height * w3m_config.max_width_pixels) / img_width;
        img_width = w3m_config.max_width_pixels;
    }
    if (img_height > w3m_config.max_height_pixels) {
        img_width = (img_width * w3m_config.max_height_pixels) / img_height;
        img_height = w3m_config.max_height_pixels;
    }

    *width = img_width;
    *height = img_height;

    return 1;
}

void run_preview(char *path, entry* file, int begx, int begy, int maxx, int maxy) {
    if (file->type == ENTRY_TYPE_FILE)
        set_entry_type(file);

    if (file->type == ENTRY_TYPE_TEXT) {
        char *preview = malloc(sizeof(*preview) * (maxx * maxy) + 1);
        FILE* f = fopen(file->name, "r");
        int bytesread = fread(preview, 1, maxx * maxy, f);
        preview[bytesread] = '\0';
        fclose(f);

        file->preview = preview;
    }
    else if (file->type == ENTRY_TYPE_IMAGE) {
        if (w3m_config.pid == 0) {
            w3m_start();
            w3m_get_sizes(maxx, maxy);
        }

        int startx, starty;
        startx = (begx * w3m_config.fontx);
        starty = (begy * w3m_config.fonty);

        int img_width, img_height;
        if (w3m_get_img_info(path, file, &img_width, &img_height)) {
            fprintf(w3m_config.fout, "0;1;%d;%d;%d;%d;;;;;%s/%s\n4;\n3;\n", startx, starty, img_width, img_height, path, file->name);
            fflush(w3m_config.fout);
        }
    }
}

void run_clear_image_preview(char *path, entry *file, int begx, int begy, int maxx, int maxy) {
    char buffer[1024];
    fgets(buffer, 1024, w3m_config.fin);
    int startx, starty;
    startx = (begx * w3m_config.fontx);
    starty = (begy * w3m_config.fonty);

    int img_width, img_height;
    if (w3m_get_img_info(path, file, &img_width, &img_height)) {
        fprintf(w3m_config.fout, "6;%d;%d;%d;%d;\n3;\n", startx, starty, img_width + w3m_config.fontx, img_height + w3m_config.fonty);
        fflush(w3m_config.fout);
    }
}

void run_cleanup() {
    w3m_kill();
}
