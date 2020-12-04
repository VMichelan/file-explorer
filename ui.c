#include <ncurses.h>
#include <errno.h>

#include "dir.h"

#define W1_RATIO 1/8
#define W2_RATIO 3/8
#define W3_RATIO 4/8

#define WIN_YMAX(yMax) (yMax-1)
#define WIN_YSIZE(yMax) (yMax-2)

#define FILELINE(directory, i) (i - directory->index)
#define CURSORLINE(directory) (FILELINE(directory, directory->cursor))

WINDOW *w1, *w2, *w3, *w2w3, *cmdw, *pathw, *wbetweenw2w3;

int yMax,xMax;

void ui_init_win() {
    getmaxyx(stdscr,yMax,xMax);
    w1 =    newwin(WIN_YSIZE(yMax), xMax*W1_RATIO, 1, 0);
    w2 =    newwin(WIN_YSIZE(yMax), xMax*W2_RATIO, 1, xMax*W1_RATIO+1);
    w3 =    newwin(WIN_YSIZE(yMax), xMax*W3_RATIO, 1, (xMax*W1_RATIO+1)+(xMax*W2_RATIO)+1);
    w2w3 =  newwin(WIN_YSIZE(yMax), xMax*W2_RATIO + xMax*W3_RATIO, 1, xMax*W1_RATIO+1);
    pathw = newwin(1, xMax, 0, 0);
    cmdw =  newwin(1, xMax, yMax-1, 0);
    wbetweenw2w3 = newwin(WIN_YSIZE(yMax), 1, 1, xMax*W1_RATIO + xMax*W2_RATIO + 1);
}

enum COLOR {
    COLOR_DEFAULT = 0,
    COLOR_DIRECTORY = 1,
    COLOR_PATH = 2,
    COLOR_ERROR = 3,
    COLOR_LINK = 4,
    COLOR_SEARCH = 5,
};

void ui_init() {
    start_color();
    use_default_colors();

    init_pair(COLOR_DIRECTORY,  COLOR_BLUE,    -1          );
    init_pair(COLOR_PATH,       COLOR_GREEN,   -1          );
    init_pair(COLOR_ERROR,      -1,            COLOR_RED   );
    init_pair(COLOR_LINK,       COLOR_CYAN,    -1          );
    init_pair(COLOR_SEARCH,     COLOR_YELLOW,  -1          );

    refresh();
    ui_init_win();
}

void ui_handle_resize() {
    endwin();
    refresh();
    clear();
    delwin(w1);
    delwin(w2);
    delwin(w3);
    delwin(w2w3);
    delwin(pathw);
    delwin(cmdw);
    delwin(wbetweenw2w3);
    ui_init_win();
}

void ui_print_path(char* path) {
    wattron(pathw,COLOR_PAIR(2));
    mvwaddstr(pathw,0,0,path);
    wattroff(pathw,COLOR_PAIR(2));
    wclrtoeol(pathw);
    wrefresh(pathw);
}

enum COLOR get_color(struct entry *e) {
    if (e->islink) {
        return COLOR_LINK;
    }
    else if (e->type == ENTRY_TYPE_DIRECTORY) {
        return COLOR_DIRECTORY;
    }
    return COLOR_DEFAULT;
}

void ui_highlight_line(WINDOW* w, dir* directory, int line, attr_t attr) {
    short color;
    if (directory->contents[directory->index + line]->islink) {
        color = 4;
    }
    else if (IS_DIR(directory, line + directory->index)) {
        color = 1;
    }
    else {
        color = 0;
    }
    mvwchgat(w, line, 0, -1, attr, color, NULL);
}

void ui_print_dir(WINDOW* w, dir* directory) {
    if (!directory) {
        return;
    }
    int i = 0, yMax, xMax;
    getmaxyx(w, yMax, xMax);
    if (directory->size == 0) {
        wmove(w, 0, 0);
        wclrtoeol(w);
        wattron(w, COLOR_PAIR(3));
        waddstr(w, "***EMPTY***");
        wattroff(w, COLOR_PAIR(3));
        i++;
    }
    while (i + directory->index < directory->size && i < yMax) {
        enum COLOR c = get_color(directory->contents[i + directory->index]); 

        wattron(w, COLOR_PAIR(c));

        if (directory->contents[i + directory->index]->marked) {
            mvwaddch(w, i, 0, '>');
        }
        else {
            mvwaddch(w, i, 0, ' ');
        }
        waddnstr(w, directory->contents[i + directory->index]->name, xMax-2);

        wclrtoeol(w);

        i++;
        wattroff(w, COLOR_PAIR(c));
    }
    wclrtobot(w);
    if (directory->size > 0) {
        mvwchgat(w, CURSORLINE(directory), 0, -1, A_STANDOUT, get_color(directory->contents[directory->cursor]), NULL);
    }
    wrefresh(w);
}

void ui_print_cmd(dir* directory, char* str) {
    wmove(cmdw, 0, 0);

    if (str) {
        waddstr(cmdw, str);
    }
    else if (errno) {
        int temp = errno;
        wattron(cmdw, COLOR_PAIR(3));
        waddstr(cmdw, strerror(temp));
        wattroff(cmdw, COLOR_PAIR(3));
        errno = 0;
    }

    wclrtoeol(cmdw);

    char* position = malloc(sizeof(*position) * xMax);
    int num_bytes = snprintf(position, xMax, "%d/%d", directory->cursor + 1, directory->size);
    mvwaddstr(cmdw, 0, xMax - num_bytes, position);

    wrefresh(cmdw);
}
