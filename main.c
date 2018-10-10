#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <locale.h>

#include "run.h"
#include "dir.h"
#include "ui.h"

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
    keypad(w2,TRUE);
    set_escdelay(50);
    refresh();
}

int main(int argc, char* argv[])
{   
    init();
    initui();
    int yMax,xMax;
    int ch;
    int temp = 0;
    getmaxyx(stdscr,yMax,xMax);
    dir* directory = read_directory();
    chdir("..");
    directory->parentdir = read_directory();
    insert_dir(directory->parentdir,directory);
    chdir(directory->path);
    while(true){
        getmaxyx(stdscr,yMax,xMax); 
        yMax -= 1;
        print_path(pathw,directory->path);
        render_contents(w2,directory);
        if(strlen(directory->path) > 1){
            render_contents(w1,directory->parentdir);
        }
        else{
            werase(w1);
            wrefresh(w1);
        }
        if(directory->type[directory->cursor] == DT_DIR){
            if(directory->dirlist[directory->cursor] == NULL){
                if(chdir(directory->content[directory->cursor]) != -1){
                    dir* temp = read_directory();
                    temp->parentdir = directory;
                    render_contents(w3,temp);
                    insert_dir(directory,temp);
                    chdir(directory->path);
                }
                else {
                    werase(w3);
                    wrefresh(w3);
                }
            }
            else {
                render_contents(w3,directory->dirlist[directory->cursor]);
            }
        }
        else {
            werase(w3);
            wrefresh(w3);
        }


        ch = getchar();
        if(ch == KEY_RESIZE){
            handle_resize();
            continue;
        }
        switch (ch){
            case 'j':
                move_cursor(directory,yMax,1);
                break;

            case KEY_NPAGE:
                move_cursor(directory,yMax,yMax-5);
                break;

            case 'k':
                move_cursor(directory,yMax,-1);
                break;

            case KEY_PPAGE:
                move_cursor(directory,yMax,(yMax-5)*-1);
                break;

            case 'h':
                directory = open_entry(directory,1);

                break;    

            case 'l':
                directory = open_entry(directory,0);
                break;

            case 'f':
                mvwprintw(cmdw,0,0,"FIND MODE");
                wrefresh(cmdw);
                render_contents(w2,directory);
                temp = find_entry(directory);
                if (temp >= 0){
                    directory->cursor = temp;
                    directory = open_entry(directory,0);
                    if(directory->type[directory->cursor] == DT_DIR)
                        directory->index = 0;
                }

                render_contents(w2,directory);
                wmove(cmdw,0,0);
                wclrtoeol(cmdw);
                wrefresh(cmdw);
                timeout(250);
                ch = getch();
                if(ch != 'h' && ch != 'l')
                    ungetch(ch);
                timeout(-1);


                break;

            case 't':
                open_terminal();
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
