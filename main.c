#define _GNU_SOURCE
#include <ncurses.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <locale.h>
#include <signal.h>

#include "ui.c"
#include "run.h"
#include "dir.h"

#define CTRL(x) (x - 64)
enum ACTION {
    NONE,
    UP,
    S_UP,
    DOWN,
    S_DOWN,
    RIGHT,
    S_RIGHT,
    LEFT,
    SHELL,
    TERMINAL,
    BEGIN,
    END,
    FIND,
    RELOAD,
    EXTRACT,
    MARK,
    MARK_ADVANCE,
    UNMARK,
    YANK,
    S_YANK,
    PREVIEW,
    QUIT
};

struct keybinding {
    int key;
    enum ACTION action;
};

static struct keybinding keybindings[] = {
    {'k'        , UP            },
    {CTRL('U')  , S_UP          },
    {KEY_PPAGE  , S_UP          },
    {'j'        , DOWN          },
    {CTRL('D')  , S_DOWN        },
    {KEY_NPAGE  , S_DOWN        },
    {'l'        , RIGHT         },
    {'L'        , S_RIGHT       },
    {'h'        , LEFT          },
    {'S'        , SHELL         },
    {'t'        , TERMINAL      },
    {'g'        , BEGIN         },
    {'G'        , END           },
    {'f'        , FIND          },
    {'r'        , RELOAD        },
    {'X'        , EXTRACT       },
    {'m'        , MARK          },
    {' '        , MARK_ADVANCE  },
    {'M'        , UNMARK        },
    {'y'        , YANK          },
    {'Y'        , S_YANK        },
    {'p'        , PREVIEW       },
    {'q'        , QUIT          }
};

void init(){
    setlocale(LC_ALL, "");
    initscr();
    start_color();
    use_default_colors();
    init_pair(1,COLOR_BLUE,-1);
    init_pair(2,COLOR_GREEN,-1);
    init_pair(3,-1,COLOR_RED);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr,TRUE);
    set_escdelay(50);
    getmaxyx(stdscr,yMax,xMax);
    refresh();
}

int find(dir* directory) {
    int pos = 0, i, j;
    int ch;
    char *str = malloc(sizeof(*str) * 1024);
    char *tmp = NULL;
    str[0] = '\0';

    mvwaddstr(cmdw, 0, 0, "Find:");
    wrefresh(cmdw);
    for (;;) {
        ch = getch();
        switch (ch) {
            case KEY_BACKSPACE:
                if (pos > 0) {
                    str[--pos] = '\0';
                }
                break;
            case 9:
            case CTRL('N'):
                for (i = 0; i < directory->size; i++) {
                    j = (i + directory->cursor + 1) % directory->size;
                    if (strcasestr(directory->contents[j]->name, str)) {
                        dir_move_cursor(directory, WIN_YMAX(yMax), j - directory->cursor);
                        break;
                    }
                }
                break;
            case CTRL('P'):
                for (i = 0; i < directory->size; i++) {
                    j = (directory->cursor - i - 1);
                    if (j < 0) {
                        j += directory->size;
                    }
                    if (strcasestr(directory->contents[j]->name, str)) {
                        dir_move_cursor(directory, WIN_YMAX(yMax), j - directory->cursor);
                        break;
                    }
                }
                break;
            case 10:
                for (i = 0; i < directory->size; i++) {
                    j = (i + directory->cursor) % directory->size;
                    if (strcasestr(directory->contents[j]->name, str)) {
                        directory->cursor = j;
                        break;
                    }
                }
                return 1;
            case 27:
                return 0;
            default:
                if (pos < 1024) {
                    str[pos] = ch;
                    str[++pos] = '\0';
                }
        }
        mvwaddstr(cmdw, 0, 5, str);
        wclrtoeol(cmdw);
        ui_print_dir(w2, directory);
        for (j = directory->index; j < directory->size && j < directory->index + WIN_YSIZE(yMax); j++) {
            tmp = strcasestr(directory->contents[j]->name, str);
            if (tmp) {
                mvwchgat(w2, FILELINE(directory, j), tmp - directory->contents[j]->name + 1, pos, A_STANDOUT, 4, NULL);
            }
        }
        wrefresh(w2);
        wrefresh(cmdw);
    }
}

void display_dir(dir* directory, char* preview) {
    werase(wbetweenw2w3);
    wrefresh(wbetweenw2w3);

    if (directory->parentdir) {
        ui_print_dir(w1, directory->parentdir);
    }
    else {
        werase(w1);
        wrefresh(w1);
    }

    if (!IS_DIR(directory, directory->cursor)) {
        if (preview[0] != '\0') {
            mvwaddstr(w3, 0, 0, preview);
            wclrtobot(w3);
            wrefresh(w3);
            preview[0] = '\0';
        }
        else {
            ui_print_dir(w2w3, directory);
        }
    }
    else {
        if (directory->contents[directory->cursor]->dir_ptr != NULL) {
            ui_print_dir(w2, directory);
            ui_print_dir(w3, directory->contents[directory->cursor]->dir_ptr);
        }
        else {
            if (dir_cd_cursor(directory) == directory) {
                ui_print_dir(w2w3, directory);
            }
            else {
                chdir(directory->path);
                ui_print_dir(w2, directory);
                ui_print_dir(w3, directory->contents[directory->cursor]->dir_ptr);
            }
        }
    }

}

int main(int argc, char* argv[])
{   
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = ui_handle_resize;
    sigaction(SIGWINCH, &sa, NULL);

    int ch;
    int temp = 0;
    init();
    ui_init();
    dir* directory = dir_init();

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGWINCH);

    enum ACTION action;

    char* preview = malloc(6667);
    preview[0] = '\0';

    while (true) {

        display_dir(directory, preview);
        ui_print_cmd(directory, NULL);
        ui_print_path(directory->path);

        ch = getch();

        action = NONE;

        for (int i = 0; i < sizeof(keybindings)/sizeof(*keybindings); i++) {
            if (keybindings[i].key == ch) {
                action = keybindings[i].action;
                break;
            }
        }

        switch (action) {
            case DOWN:
                dir_move_cursor(directory,WIN_YMAX(yMax),1);
                break;

            case S_DOWN:
                dir_move_cursor(directory,WIN_YMAX(yMax),(WIN_YMAX(yMax)/2));
                break;

            case UP:
                dir_move_cursor(directory,WIN_YMAX(yMax),-1);
                break;

            case S_UP:
                dir_move_cursor(directory,WIN_YMAX(yMax),(-WIN_YMAX(yMax)/2));
                break;

            case LEFT:
                directory = dir_up(directory);
                ui_print_path(directory->path);
                break;    

            case RIGHT:
                if (IS_DIR(directory, directory->cursor)) {
                    directory = dir_cd_cursor(directory);
                    ui_print_path(directory->path);
                }
                else {
                    sigprocmask(SIG_BLOCK, &sigs, 0);
                    endwin();
                    run_open_file(directory->contents[directory->cursor]->name, 1);
                    sigprocmask(SIG_UNBLOCK, &sigs, 0);
                }
                break;

            case S_RIGHT:
                run_open_file(directory->contents[directory->cursor]->name, 0);
                break;

            case FIND:
                if (find(directory)) {
                    ungetch('l');
                }
                break;

            case TERMINAL:
                run_open_terminal();
                break;

            case RELOAD:
                directory = dir_reload(directory);
                break;

            case EXTRACT:
                endwin();
                run_extract_file(directory->contents[directory->cursor]->name);
                directory = dir_reload(directory);
                break;

            case SHELL:
                endwin();
                run_shell();
                break;

            case MARK_ADVANCE:
                for (int i = 0; i < sizeof(keybindings)/sizeof(*keybindings); i++) {
                    if (keybindings[i].action == DOWN) {
                        ungetch(keybindings[i].key);
                        break;
                    }
                }
            case MARK:
                directory->markedcount -= directory->contents[directory->cursor]->marked;
                directory->contents[directory->cursor]->marked = !directory->contents[directory->cursor]->marked;
                directory->markedcount += directory->contents[directory->cursor]->marked;
                break;

            case UNMARK:
                for (int i = 0; i < directory->size; i++) {
                    directory->contents[i]->marked = 0;
                }
                directory->markedcount = 0;
                break;

            case YANK:
                {
                    if (directory->markedcount > 0) {
                        char *filenames[directory->markedcount];
                        int j = 0;
                        for (int i = 0; i < directory->size && j < directory->markedcount; i++) {
                            if (directory->contents[i]->marked) {
                                filenames[j] = directory->contents[i]->name;
                                j++;
                            }
                        }
                        run_copy_to_clipboard(filenames, directory->markedcount);
                    }
                    else {
                        run_copy_to_clipboard(&directory->contents[directory->cursor]->name, 1);
                    }
                }
                break;

            case S_YANK:
                run_copy_to_clipboard(&directory->path, 1);
                break;

            case PREVIEW:
                run_preview(directory->contents[directory->cursor]->name, preview, WIN_YSIZE(yMax) * W3_RATIO * xMax);
                break;

            case BEGIN:
                dir_move_cursor(directory, WIN_YMAX(yMax), -directory->cursor);
                break;

            case END:
                dir_move_cursor(directory, WIN_YMAX(yMax), directory->size);
                break;

            case NONE:
                break;

            case QUIT:
                clear();
                refresh(); 
                endwin();
                return 0;
        } 

    }
    return 0;
}
