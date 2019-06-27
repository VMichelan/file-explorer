#include <ncurses.h>
#include <dirent.h>

#include "dir.h"

#define W1FACTOR 1/5
#define W2FACTOR 2/5
#define W3FACTOR 2/5

WINDOW *w1,*w2,*w3,*cmdw,*pathw;

void initui() {
    int yMax,xMax;
    getmaxyx(stdscr,yMax,xMax);
    w1 = newwin(yMax-2,xMax*W1FACTOR,1,0);
    w2 = newwin(yMax-2,xMax*W2FACTOR,1,xMax*W1FACTOR+1);
    w3 = newwin(yMax-2,xMax*W3FACTOR,1,(xMax*W1FACTOR+1)+(xMax*W2FACTOR)+1);
    pathw = newwin(1,xMax,0,0);
    cmdw = newwin(1,xMax,yMax-1,0);
    keypad(w2,TRUE);

}

int getchar() {
    return wgetch(w2);
}

void handle_resize() {
    int yMax,xMax,i=0;
    getmaxyx(stdscr,yMax,xMax); 
    while (i < yMax) {
        wmove(w3,i,0);
        wclrtoeol(w3);
        i++;
    }
    clear();
    refresh();
    delwin(w1);
    delwin(w2);
    delwin(w3);
    delwin(pathw);
    delwin(cmdw);
    initui();
    wmove(cmdw,0,0);
    wclrtoeol(cmdw);
    wrefresh(cmdw);

}

void print_path(WINDOW* w,char* path) {
    wmove(w,0,0);
    wclrtoeol(w);
    wattron(w,COLOR_PAIR(2));
    mvwprintw(w,0,0,path);
    wattroff(w,COLOR_PAIR(2));
    wrefresh(w);
}

void render_contents(WINDOW* w,dir* directory) {
    if (!directory) {
        return;
    }
    int i = 0, yMax, xMax;
    int cursory, cursorx;
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
        if (directory->cursor == i + directory->index) {
            wattron(w, A_STANDOUT);
        }
        else {
            wattroff(w, A_STANDOUT);
        }   
        if (directory->type[i + directory->index] == DT_DIR) {
            wattron(w, COLOR_PAIR(1));
        }

        mvwaddch(w, i, 0, ' ');
        waddnstr(w, directory->content[i + directory->index], xMax-2);

        getyx(w, cursory, cursorx);

        if (directory->cursor == i + directory->index) {
            while (cursorx < xMax) {
                waddch(w, ' ');
                cursorx++;
            }
        }
        else {
            wclrtoeol(w);
        }

        i++;
        wattroff(w, COLOR_PAIR(1));
    }
    while (i < yMax) {
        wmove(w, i, 0);
        wclrtoeol(w);
        i++;
    }
    wattroff(w, A_STANDOUT);
    wrefresh(w);

}
