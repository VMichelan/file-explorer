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

#define TAB_KEY 9
#define ENTER_KEY 10
#define ESC_KEY 27

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
    TOGGLE_HIDDEN,
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
    {'.'        , TOGGLE_HIDDEN },
    {'q'        , QUIT          }
};

int find(dir* directory) {
    int pos = 0, i, j;
    int ch;
    char str[1024];
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
            case TAB_KEY:
            case CTRL('N'):
                for (i = 0; i < directory->size; i++) {
                    j = (i + directory->cursor + 1) % directory->size;
                    if (strcasestr(directory->entry_array[j]->name, str)) {
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
                    if (strcasestr(directory->entry_array[j]->name, str)) {
                        dir_move_cursor(directory, WIN_YMAX(yMax), j - directory->cursor);
                        break;
                    }
                }
                break;
            case ENTER_KEY:
                for (i = 0; i < directory->size; i++) {
                    j = (i + directory->cursor) % directory->size;
                    if (strcasestr(directory->entry_array[j]->name, str)) {
                        directory->cursor = j;
                        break;
                    }
                }
                return 1;
            case ESC_KEY:
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
            tmp = strcasestr(directory->entry_array[j]->name, str);
            if (tmp) {
                mvwchgat(w2, FILELINE(directory, j), tmp - directory->entry_array[j]->name + 1, pos, A_STANDOUT, COLOR_SEARCH, NULL);
            }
        }
        wrefresh(w2);
        wrefresh(cmdw);
    }
}

void display_dir(dir* directory) {
    werase(wbetweenw2w3);
    wrefresh(wbetweenw2w3);

    if (directory->parentdir) {
        ui_print_dir(w1, directory->parentdir);
    }
    else {
        werase(w1);
        wrefresh(w1);
    }

    if (directory->size == 0) {
        ui_print_dir(w2w3, directory);
        return;
    }

    if (!IS_DIR(directory, directory->cursor)) {
        if (directory->entry_array[directory->cursor]->preview) {
            mvwaddstr(w3, 0, 0, directory->entry_array[directory->cursor]->preview);
            wclrtobot(w3);
            wrefresh(w3);
            ui_print_dir(w2, directory);
        }
        else {
            ui_print_dir(w2w3, directory);
        }
    }
    else {
        if (directory->dir_array[directory->cursor] != NULL) {
            ui_print_dir(w2, directory);
            ui_print_dir(w3, directory->dir_array[directory->cursor]);
        }
        else {
            ui_print_dir(w2w3, directory);
        }
    }
}

void move_cursor(dir *directory, int n) {
    dir_move_cursor(directory,WIN_YMAX(yMax), n);
    dir_load_dir_at_cursor(directory);
}

int main(int argc, char* argv[])
{   
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = ui_handle_resize;
    sigaction(SIGWINCH, &sa, NULL);

    int ch;
    int temp = 0;
    int begx, begy;
    int xmaxw3, ymaxw3;

    setlocale(LC_ALL, "");
    initscr();
    raw();
    noecho();
    curs_set(0);
    keypad(stdscr,TRUE);
    set_escdelay(50);
    ui_init();
    dir* directory = dir_init(NULL);
    dir_load_dir_at_cursor(directory);

    sigset_t sigs;
    sigemptyset(&sigs);
    sigaddset(&sigs, SIGWINCH);

    enum ACTION action;

    while (true) {
        display_dir(directory);
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
                move_cursor(directory,1);
                break;

            case S_DOWN:
                move_cursor(directory,(WIN_YMAX(yMax)/2));
                break;

            case UP:
                move_cursor(directory,-1);
                break;

            case S_UP:
                move_cursor(directory,(-WIN_YMAX(yMax)/2));
                break;

            case LEFT:
                directory = dir_up(directory);
                ui_print_path(directory->path);
                break;    

            case RIGHT:
                if (IS_DIR(directory, directory->cursor)) {
                    if (!directory->dir_array[directory->cursor]) {
                        dir_load_dir_at_cursor(directory);
                    }
                    if (directory->dir_array[directory->cursor]) {
                        chdir(directory->dir_array[directory->cursor]->path);
                        directory = directory->dir_array[directory->cursor];
                        if (directory->size > 0 && directory->entry_array[directory->cursor]->type == ENTRY_TYPE_DIRECTORY) {
                            dir_load_dir_at_cursor(directory);
                        }
                        ui_print_path(directory->path);
                    }
                }
                else {
                    sigprocmask(SIG_BLOCK, &sigs, 0);
                    endwin();
                    run_open_file(directory->entry_array[directory->cursor], 1);
                    sigprocmask(SIG_UNBLOCK, &sigs, 0);
                }
                break;

            case S_RIGHT:
                run_open_file(directory->entry_array[directory->cursor], 0);
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
                run_extract_file(directory->entry_array[directory->cursor]);
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
                directory->markedcount -= directory->entry_array[directory->cursor]->marked;
                directory->entry_array[directory->cursor]->marked = !directory->entry_array[directory->cursor]->marked;
                directory->markedcount += directory->entry_array[directory->cursor]->marked;
                break;

            case UNMARK:
                for (int i = 0; i < directory->size; i++) {
                    directory->entry_array[i]->marked = 0;
                }
                directory->markedcount = 0;
                break;

            case YANK:
                {
                    if (directory->markedcount > 0) {
                        char *filenames[directory->markedcount];
                        int j = 0;
                        for (int i = 0; i < directory->size && j < directory->markedcount; i++) {
                            if (directory->entry_array[i]->marked) {
                                filenames[j] = directory->entry_array[i]->name;
                                j++;
                            }
                        }
                        run_copy_to_clipboard(filenames, directory->markedcount);
                    }
                    else {
                        run_copy_to_clipboard(&directory->entry_array[directory->cursor]->name, 1);
                    }
                }
                break;

            case S_YANK:
                run_copy_to_clipboard(&directory->path, 1);
                break;

            case PREVIEW:
                getbegyx(w3, begy, begx);
                begx++;
                getmaxyx(w3, ymaxw3, xmaxw3);
                wclear(w3);
                wclear(wbetweenw2w3);
                wrefresh(w3);
                wrefresh(wbetweenw2w3);
                usleep(10000);
                run_preview(directory->path, directory->entry_array[directory->cursor], begx, begy, xmaxw3, ymaxw3);
                if (directory->entry_array[directory->cursor]->type == ENTRY_TYPE_IMAGE ||
                    directory->entry_array[directory->cursor]->type == ENTRY_TYPE_VIDEO) {
                    ch = getch();
                    run_clear_image_preview(directory->entry_array[directory->cursor], begx, begy);
                    ungetch(ch);
                }
                break;

            case BEGIN:
                dir_move_cursor(directory, WIN_YMAX(yMax), -directory->cursor);
                break;

            case END:
                dir_move_cursor(directory, WIN_YMAX(yMax), directory->size);
                break;

            case NONE:
                break;

            case TOGGLE_HIDDEN:
                directory = dir_toggle_hidden(directory);
                break;

            case QUIT:
                clear();
                refresh(); 
                endwin();
                run_cleanup();
                for (int i = 0; i < argc; i++) {
                    if (!strcmp(argv[i], "-o")) {
                        char *runtime_dir = getenv("XDG_RUNTIME_DIR");
                        if (runtime_dir) {
                            char *file_path = malloc(sizeof(*file_path) * strlen(runtime_dir) + 100);
                            strncpy(file_path, runtime_dir, strlen(runtime_dir));
                            strcat(file_path, "/tnfx");
                            FILE *f = fopen(file_path, "w");
                            fprintf(f, directory->path);
                            fclose(f);
                            free(file_path);
                        }
                    }
                }
                return 0;
        } 

    }

    return 0;
}
