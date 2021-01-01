#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <ctype.h>
#include <errno.h>

#include "dir.h"

int show_hidden = 0;

void* malloc_or_die(int size) {
    void* returnPrt = malloc(size);
    if (!returnPrt) {
        printw("FAILLED MALLOC");
        refresh();
        getch();
        endwin();
        exit(EXIT_FAILURE);
    }
    else return returnPrt;
}

void* realloc_or_die(void* prt,int size) {
    void* returnPrt = realloc(prt,size);
    if (!returnPrt) {
        printw("FAILLED REALLOC");
        refresh();
        getch();
        endwin();

        exit(EXIT_FAILURE);
    }
    else return returnPrt;
}

void* calloc_or_die(int size,int size2) {
    void* returnPrt = calloc(size,size2);
    if (!returnPrt) {
        printw("FAILLED CALLOC");
        refresh();
        getch();
        endwin();
        exit(EXIT_FAILURE);
    }
    else return returnPrt;
}


void dir_free(dir *dir_info){
    int i;
    if (dir_info == NULL) {
        return;
    }
    for (i = 0; i < dir_info->size; i++) {
        free(dir_info->entry_array[i]->name);
        free(dir_info->entry_array[i]->preview);
        free(dir_info->entry_array[i]);
        if (i < dir_info->dircount) {
            dir_free(dir_info->dir_array[i]);
        }
    }
    free(dir_info->entry_array);
    free(dir_info->path);
    free(dir_info);
}

int entry_cmp(const void * entry1, const void * entry2) {
    return strcasecmp((*(entry **) entry1)->name, (*(entry **) entry2)->name);
}

void sort_dir(dir* directory){
    entry** dir_entries = malloc_or_die(directory->dircount * sizeof(entry*));
    entry** file_entries = malloc_or_die((directory->size - directory->dircount) * sizeof(entry *));

    int dirindex = 0, fileindex = 0;

    for (int entryindex = 0; entryindex < directory->size; entryindex++) {
        if (directory->entry_array[entryindex]->type == ENTRY_TYPE_DIRECTORY) {
            dir_entries[dirindex] = directory->entry_array[entryindex];
            dirindex++;
        }
        else {
            file_entries[fileindex] = directory->entry_array[entryindex];
            fileindex++;
        }
    }

    qsort(dir_entries, directory->dircount, sizeof(entry *), entry_cmp);
    qsort(file_entries, directory->size - directory->dircount, sizeof(entry *), entry_cmp);

    memcpy(directory->entry_array, dir_entries, directory->dircount * sizeof(entry *));
    memcpy( directory->entry_array + directory->dircount,
            file_entries,
            (directory->size - directory->dircount) * sizeof(entry *));

    free(dir_entries);
    free(file_entries);
}

dir* dir_create(const char* directorypath) {
    if (!directorypath)
        return NULL;

    int i = 0;
    int dircount = 0;

    dir* dir_info = calloc_or_die(1, sizeof(dir));
    dir_info->entry_array = calloc_or_die(1, sizeof(entry*));

    DIR* directory = opendir(directorypath);

    if (!directory)
        return NULL;

    struct dirent *directory_entry;
    struct stat sb;

    char* cwd = malloc_or_die(sizeof(*cwd) * PATH_MAX);
    getcwd(cwd, PATH_MAX);
    chdir(directorypath);

    struct entry *e;

    while (directory_entry = readdir(directory)) {
        if (    strcmp(directory_entry->d_name, "..")   &&
                strcmp(directory_entry->d_name, ".")    &&
                (show_hidden || directory_entry->d_name[0] != '.')
            ) {
            e = calloc_or_die(1, sizeof(entry));
            e->name = malloc_or_die(FILENAME_SIZE);
            strcpy(e->name, directory_entry->d_name);
            e->marked = 0;
            e->islink = 0;
            e->preview = NULL;
            switch (directory_entry->d_type) {
                case DT_REG:
                    e->type = ENTRY_TYPE_FILE;
                    break;
                case DT_DIR:
                    e->type = ENTRY_TYPE_DIRECTORY;
                    dircount++;
                    break;
                case DT_LNK:
                    e->islink = 1;
                case DT_UNKNOWN:
                    if (stat(directory_entry->d_name, &sb) && errno == ENOENT || errno == ELOOP) {
                        if (lstat(directory_entry->d_name, &sb)) {
                            e->type = ENTRY_TYPE_UNKNOWN;
                            break;
                        }
                    }

                    if (S_ISDIR(sb.st_mode)) {
                        e->type = ENTRY_TYPE_DIRECTORY;
                        dircount++;
                    }
                    else {
                        e->type = ENTRY_TYPE_FILE;
                    }
                    break;
                default:
                    e->type = ENTRY_TYPE_UNKNOWN;
                    break;
            }
            dir_info->entry_array = realloc_or_die(dir_info->entry_array, sizeof(entry*) * i + 1);
            dir_info->entry_array[i] = e;

            i++;
        }
    }

    closedir(directory);

    dir_info->path = malloc_or_die(sizeof(*dir_info->path) * PATH_MAX);
    getcwd(dir_info->path, PATH_MAX);

    chdir(cwd);
    free(cwd);

    dir_info->dircount = dircount;
    dir_info->dir_array = calloc_or_die(sizeof(*(dir_info->dir_array)), dircount);
    dir_info->size = i;
    dir_info->markedcount = 0;
    dir_info->cursor = 0;
    dir_info->index = 0;
    dir_info->parentdir = NULL;
    sort_dir(dir_info);

    errno = 0;
    return dir_info;
}

void dir_inser(dir* directory) {
    char* dirpath = strdup(directory->path);
    char* dirname;
    char* next = strtok(dirpath, "/");
    do {
        dirname = next;
        next = strtok(NULL, "/");
    } while (next);

    for (int i = 0; i < directory->parentdir->size; i++) {
        if (strcmp(dirname, directory->parentdir->entry_array[i]->name) == 0) {
            directory->parentdir->dir_array[i] = directory;
            directory->parentdir->cursor = i;
            break;
        }
    }

    free(dirpath);
}

dir* dir_up(dir* directory) {
    if (IS_PATH_ROOT(directory->path)) {
        return directory;
    }

    if (directory->parentdir) {
        if (!directory->parentdir->parentdir) {
            if (!IS_PATH_ROOT(directory->parentdir->path)) {
                directory->parentdir->parentdir = dir_create("../..");
                dir_inser(directory->parentdir);
            }
        }
        chdir(directory->parentdir->path);
        return directory->parentdir;
    }

    return directory;
}

void dir_load_dir_at_cursor(dir *directory) {
    entry *entry_at_cursor = directory->entry_array[directory->cursor];
    if (entry_at_cursor->type == ENTRY_TYPE_DIRECTORY && !directory->dir_array[directory->cursor]) {
        directory->dir_array[directory->cursor] = dir_create(entry_at_cursor->name);
        if (directory->dir_array[directory->cursor])
            directory->dir_array[directory->cursor]->parentdir = directory;
    }
}

void dir_move_cursor(dir* directory,int yMax,int number) {
    if(number < 0) {
        if(directory->cursor > 0) {
            directory->cursor += number;
            if (directory->cursor < 0) {
                directory->cursor = 0;
            }
            if (directory->index > directory->cursor) {
                directory->index = directory->cursor;
                return;
            }
        }
    }
    else {
        if (directory->cursor < directory->size) {
            directory->cursor += number;
            if(directory->cursor > directory->size-1) {
                directory->cursor = directory->size-1;
            }
            if (directory->cursor >= directory->index+yMax-1) {
                directory->index = (directory->cursor - yMax)+2;
            }
        }
    }
}

dir* dir_init(char *arg_path) {
    char path[PATH_MAX];
    if (arg_path) {
        strncpy(path, arg_path, PATH_MAX);
    }
    else {
        char* pwd = getenv("PWD");
        int pwdlen = strlen(pwd);
        strncpy(path, pwd, pwdlen);
    }

    if (!IS_PATH_ROOT(path)) {
        int path_len = strlen(path);
        path[path_len] = '/';
        path[path_len + 1] = '\0';
    }

    dir* directory = dir_create(path);

    if (!IS_PATH_ROOT(path)) {
        directory->parentdir = dir_create("..");
        dir_inser(directory);
    }

    return directory;
}

dir* dir_reload(dir* directory) {
    dir* newdirectory = dir_create(directory->path);

    for (int i = 0; i < directory->dircount; i++) {
        if (directory->dir_array[i]) {
            directory->dir_array[i]->parentdir = newdirectory;
        }
    }

    for (int i = 0; i < newdirectory->dircount; i++) {
        for (int j = 0; j < directory->dircount; j++) {
            if (!strcmp(newdirectory->entry_array[i]->name, directory->entry_array[j]->name)) {
                newdirectory->dir_array[i] = directory->dir_array[j];
                directory->dir_array[j] = NULL;
                break;
            }
        }
    }

    if (directory->parentdir) {
        for (int i = 0; i < directory->parentdir->dircount; i++) {
            if (directory->parentdir->dir_array[i] == directory) {
                directory->parentdir->dir_array[i]= newdirectory;
                break;
            }
        }
    }

    newdirectory->parentdir = directory->parentdir;
    newdirectory->cursor = directory->cursor;
    newdirectory->index = directory->index;

    dir_free(directory);
    return newdirectory;
}

dir* dir_toggle_hidden(dir *directory) {
    char path[PATH_MAX];
    char *ptr;
    strncpy(path, directory->path, PATH_MAX);
    dir_free(directory);
    ptr = strstr(path, "/.");
    if (ptr) {
        *(ptr + 1) = '\0';
    }
    show_hidden = !show_hidden;
    chdir(path);
    return dir_init(path);
}
