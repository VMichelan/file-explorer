#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>

#include "run.h"
#include "dir.h"
#include "ui.h"

#define CTRL(x) (x - 64)

void init(){
    setlocale(LC_ALL, "en_US.utf8");
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
    if (directory->type[directory->cursor] == DT_DIR) {
        if (directory->dirlist[directory->cursor] == NULL) {
            if (chdir(directory->content[directory->cursor]) != -1) {
                dir* temp = read_directory();
                temp->parentdir = directory;
                render_contents(w3,temp);
                insert_dir(directory,temp);
                chdir(directory->path);
                render_contents(w2, directory);
                render_contents(w3, temp);
            }
            else {
                render_contents(w2w3, directory);
            }
        }
        else {
            render_contents(w2, directory);
            render_contents(w3,directory->dirlist[directory->cursor]);
        }
    }
    else {
        render_contents(w2w3, directory);
    }
}

int main(int argc, char* argv[])
{   
    int yMax,xMax;
    int ch;
    int temp = 0;
    init();
    initui();
    dir* directory = initdir();
    print_path(directory->path);
    while (true) {
        getmaxyx(stdscr,yMax,xMax); 
        yMax -= 1;

        display_dir(directory);

        ch = getch();
        if (ch == KEY_RESIZE) {
            handle_resize();
            continue;
        }
        switch (ch) {
            case 'j':
                move_cursor(directory,yMax,1);
                break;

            case CTRL('D'):
            case KEY_NPAGE:
                move_cursor(directory,yMax,(yMax/2));
                break;

            case 'k':
                move_cursor(directory,yMax,-1);
                break;

            case CTRL('U'):
            case KEY_PPAGE:
                move_cursor(directory,yMax,(-yMax/2));
                break;

            case 'h':
                directory = up_dir(directory);
                print_path(directory->path);

                break;    

            case 'l':
                if (directory->type[directory->cursor] == DT_DIR) {
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
                    if (directory->type[directory->cursor] == DT_DIR) {
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

            case 'q':
                clear();
                refresh(); 
                endwin();
                return 0;

        } 

    }
    return 0;
}
