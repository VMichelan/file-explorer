#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <errno.h>

#include "run.h"
#include "dir.h"

#define CTRL(x) (x - 64)

#define W1FACTOR 1/5
#define W2FACTOR 2/5
#define W3FACTOR 2/5

#define WINYMAX(yMax) (yMax-1)
#define WINYSIZE(yMax) (yMax-2)

#define FILELINE(directory, i) (i - directory->index)
#define CURSORLINE(directory) (FILELINE(directory, directory->cursor))

WINDOW *w1, *w2, *w3, *w2w3, *cmdw, *pathw, *wbetweenw2w3;

int yMax,xMax;

void initui() {
    w1 =    newwin(WINYSIZE(yMax), xMax*W1FACTOR, 1, 0);
    w2 =    newwin(WINYSIZE(yMax), xMax*W2FACTOR, 1, xMax*W1FACTOR+1);
    w3 =    newwin(WINYSIZE(yMax), xMax*W3FACTOR, 1, (xMax*W1FACTOR+1)+(xMax*W2FACTOR)+1);
    w2w3 =  newwin(WINYSIZE(yMax), xMax*W2FACTOR + xMax*W3FACTOR, 1, xMax*W1FACTOR+1);
    pathw = newwin(1, xMax, 0, 0);
    cmdw =  newwin(1, xMax, yMax-1, 0);
    wbetweenw2w3 = newwin(WINYSIZE(yMax), 1, 1, xMax*W1FACTOR + xMax*W2FACTOR + 1);

}

void handle_resize() {

    clear();
    refresh();
    delwin(w1);
    delwin(w2);
    delwin(w3);
    delwin(w2w3);
    delwin(pathw);
    delwin(cmdw);
    delwin(wbetweenw2w3);

    getmaxyx(stdscr,yMax,xMax);

    initui();

}

void print_path(char* path) {
    wattron(pathw,COLOR_PAIR(2));
    mvwaddstr(pathw,0,0,path);
    wattroff(pathw,COLOR_PAIR(2));
    wclrtoeol(pathw);
    wrefresh(pathw);
}

void highlight_line(WINDOW* w, dir* directory, int line, attr_t attr) {
    short color;
    if (ISDIR(directory, line)) {
        color = 1;
    }
    else {
        color = 0;
    }
    mvwchgat(w, line, 0, -1, attr, color, NULL);
}

void render_contents(WINDOW* w,dir* directory) {
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
        if (ISDIR(directory, i + directory->index)) {
            wattron(w, COLOR_PAIR(1));
        }

        if (directory->marked[i]) {
            mvwaddch(w, i, 0, '>');
        }
        else {
            mvwaddch(w, i, 0, ' ');
        }
        waddnstr(w, directory->content[i + directory->index], xMax-2);

        wclrtoeol(w);

        i++;
        wattroff(w, COLOR_PAIR(1));
    }
    wclrtobot(w);
    if (directory->size > 0) {
        highlight_line(w, directory, CURSORLINE(directory), A_STANDOUT);
    }
    wrefresh(w);

}

void init(){
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    use_default_colors();
    init_pair(1,COLOR_BLUE,-1);
    init_pair(2,COLOR_GREEN,-1);
    init_pair(3,-1,COLOR_RED);
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr,TRUE);
    set_escdelay(50);
    getmaxyx(stdscr,yMax,xMax);
    refresh();
}

void display_dir(dir* directory) {
    werase(wbetweenw2w3);
    wrefresh(wbetweenw2w3);

    if (directory->parentdir) {
        render_contents(w1,directory->parentdir);
    }
    else {
        werase(w1);
        wrefresh(w1);
    }

    if (!ISDIR(directory, directory->cursor)) {
        render_contents(w2w3, directory);
    }
    else {
        if (directory->dirlist[directory->cursor] != NULL) {
            render_contents(w2, directory);
            render_contents(w3,directory->dirlist[directory->cursor]);
        }
        else {
            if (chdir(directory->content[directory->cursor]) == -1) {
                render_contents(w2w3, directory);
            }
            else {
                dir* temp = read_directory();
                temp->parentdir = directory;
                insert_dir(temp);
                chdir(directory->path);
                render_contents(w2, directory);
                render_contents(w3, temp);
            }
        }
    }

}

void print_cmd(dir* directory, char* str) {
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

int main(int argc, char* argv[])
{   
    int ch;
    int temp = 0;
    init();
    initui();
    dir* directory = initdir();
    print_path(directory->path);
    while (true) {

        display_dir(directory);
        print_cmd(directory, NULL);

        ch = getch();
        if (ch == KEY_RESIZE) {
            handle_resize();
            print_path(directory->path);
            continue;
        }
        switch (ch) {
            case 'j':
                move_cursor(directory,WINYMAX(yMax),1);
                break;

            case CTRL('D'):
            case KEY_NPAGE:
                move_cursor(directory,WINYMAX(yMax),(WINYMAX(yMax)/2));
                break;

            case 'k':
                move_cursor(directory,WINYMAX(yMax),-1);
                break;

            case CTRL('U'):
            case KEY_PPAGE:
                move_cursor(directory,WINYMAX(yMax),(-WINYMAX(yMax)/2));
                break;

            case 'h':
                directory = up_dir(directory);
                print_path(directory->path);

                break;    

            case 'l':
                if (ISDIR(directory, directory->cursor)) {
                    directory = open_entry(directory);
                    print_path(directory->path);
                }
                else {
                    endwin();
                    run(directory->content[directory->cursor],1);
                }
                break;

            case 'L':
                run(directory->content[directory->cursor],0);
                break;

            case 'f':
                mvwprintw(cmdw,0,0,"FIND MODE");
                wrefresh(cmdw);
                render_contents(w2,directory);
                temp = find_entry(directory);
                if (temp >= 0) {
                    directory->cursor = temp;
                    directory = open_entry(directory);
                    if (ISDIR(directory, directory->cursor)) {
                        directory->index = 0;
                    }
                }

                render_contents(w2,directory);
                wmove(cmdw,0,0);
                wclrtoeol(cmdw);
                wrefresh(cmdw);
                timeout(250);
                ch = getch();
                if (ch != 'h' && ch != 'l') {
                    ungetch(ch);
                }
                timeout(-1);


                break;

            case 't':
                open_terminal();
                break;

            case 'r':
                directory = reload_dir(directory);
                break;

            case 'X':
                endwin();
                extract_file(directory->content[directory->cursor]);
                directory = reload_dir(directory);
                break;

            case 'S':
                endwin();
                run_shell();
                break;

            case ' ':
                ungetch('j');
            case 'm':
                directory->markedcount -= directory->marked[directory->cursor];
                directory->marked[directory->cursor] = !directory->marked[directory->cursor];
                directory->markedcount += directory->marked[directory->cursor];
                break;

            case 'M':
                for (int i = 0; i < directory->size; i++) {
                    directory->marked[i] = 0;
                }
                directory->markedcount = 0;
                break;

            case 'q':
                clear();
                refresh(); 
                endwin();
                return 0;

        } 

    }
    return 0;
}
