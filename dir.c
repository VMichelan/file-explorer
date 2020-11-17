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
        free(dir_info->contents[i]->name);
        free(dir_info->contents[i]->dir_ptr);
        free(dir_info->contents[i]);
    }
    free(dir_info->contents);
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
        if (directory->contents[entryindex]->type == ENTRY_TYPE_DIRECTORY) {
            dir_entries[dirindex] = directory->contents[entryindex];
            dirindex++;
        }
        else {
            file_entries[fileindex] = directory->contents[entryindex];
            fileindex++;
        }
    }

    qsort(dir_entries, directory->dircount, sizeof(entry *), entry_cmp);
    qsort(file_entries, directory->size - directory->dircount, sizeof(entry *), entry_cmp);

    memcpy(directory->contents, dir_entries, directory->dircount * sizeof(entry *));
    memcpy( directory->contents + directory->dircount,
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
    dir_info->contents = calloc_or_die(1, sizeof(entry*));

    DIR* directory = opendir(directorypath);

    if (!directory)
        return NULL;

    struct dirent *directory_entry;
    struct stat sb;

    char* cwd = malloc_or_die(sizeof(*cwd) * PATH_MAX);
    getcwd(cwd, PATH_MAX);
    chdir(directorypath);

    while (directory_entry = readdir(directory)) {
        if (directory_entry->d_name[0] != '.' && strcmp(directory_entry->d_name, "..")) {
            dir_info->contents = realloc_or_die(dir_info->contents, sizeof(entry*) * i + 1);
            dir_info->contents[i] = calloc_or_die(1, sizeof(entry));
            dir_info->contents[i]->name = malloc_or_die(FILENAME_SIZE);
            strcpy(dir_info->contents[i]->name, directory_entry->d_name);
            dir_info->contents[i]->marked = 0;
            dir_info->contents[i]->islink = 0;
            switch (directory_entry->d_type) {
                case DT_REG:
                    dir_info->contents[i]->type = ENTRY_TYPE_FILE;
                    break;
                case DT_DIR:
                    dir_info->contents[i]->type = ENTRY_TYPE_DIRECTORY;
                    dircount++;
                    break;
                case DT_LNK:
                    dir_info->contents[i]->islink = 1;
                case DT_UNKNOWN:
                    if (stat(directory_entry->d_name, &sb) && errno == ENOENT || errno == ELOOP) {
                        if (lstat(directory_entry->d_name, &sb)) {
                            dir_info->contents[i]->type = ENTRY_TYPE_UNKNOWN;
                            break;
                        }
                    }

                    if (S_ISDIR(sb.st_mode)) {
                        dir_info->contents[i]->type = ENTRY_TYPE_DIRECTORY;
                        dircount++;
                    }
                    else {
                        dir_info->contents[i]->type = ENTRY_TYPE_FILE;
                    }
                    break;
                default:
                    dir_info->contents[i]->type = ENTRY_TYPE_UNKNOWN;
                    break;
            }

            dir_info->contents[i]->dir_ptr = NULL;
            i++;
        }
    }

    closedir(directory);

    dir_info->path = malloc_or_die(sizeof(*dir_info->path) * PATH_MAX);
    getcwd(dir_info->path, PATH_MAX);

    chdir(cwd);
    free(cwd);

    dir_info->dircount = dircount;
    dir_info->size = i;
    dir_info->markedcount = 0;
    dir_info->cursor = 0;
    dir_info->index = 0;
    dir_info->parentdir = NULL;
    sort_dir(dir_info);

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
        if (strcmp(dirname, directory->parentdir->contents[i]->name) == 0) {
            directory->parentdir->contents[i]->dir_ptr = directory;
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

dir* dir_cd_cursor(dir* directory) {
    if (directory->contents[directory->cursor]->dir_ptr == NULL) {
        char* foldername = directory->contents[directory->cursor]->name;
        char* path = malloc(sizeof(*path) * strlen(directory->path) + strlen(foldername) + 2);

        strncpy(path, directory->path, strlen(directory->path) + 1);
        strncat(path, foldername, strlen(foldername));

        int pathlen = strlen(path);
        path[pathlen] = '/';
        path[pathlen + 1] = '\0';

        dir* temp = dir_create(path);

        free(path);

        if (!temp || chdir(foldername) == -1) {
            dir_free(temp);
            return directory;
        }

        temp->parentdir = directory;
        directory->contents[directory->cursor]->dir_ptr = temp;
    }

    chdir(directory->contents[directory->cursor]->dir_ptr->path);
    return directory->contents[directory->cursor]->dir_ptr;
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

dir* dir_init() {
    char* pwd = getenv("PWD");
    int pwdlen = strlen(pwd);
    char* path = malloc_or_die(sizeof(*path) * pwdlen + 2);
    strncpy(path, pwd, pwdlen);

    if (!IS_PATH_ROOT(path)) {
        path[pwdlen] = '/';
        path[pwdlen + 1] = '\0';
    }

    dir* directory = dir_create(path);
    free(path);

    if (!IS_PATH_ROOT(path)) {
        directory->parentdir = dir_create("..");
        dir_inser(directory);
    }

    return directory;
}

dir* dir_reload(dir* directory) {
    dir* newdirectory = dir_create(directory->path);

    for (int i = 0; i < directory->dircount; i++) {
        if (directory->contents[i]->dir_ptr) {
            directory->contents[i]->dir_ptr->parentdir = newdirectory;
        }
    }

    for (int i = 0; i < newdirectory->dircount; i++) {
        for (int j = i; j < directory->dircount; j++) {
            if (!strcmp(newdirectory->contents[i]->name, directory->contents[j]->name)) {
                newdirectory->contents[i]->dir_ptr = directory->contents[j]->dir_ptr;
                directory->contents[j]->dir_ptr = NULL;
                break;
            }
        }
    }

    if (directory->parentdir) {
        for (int i = 0; i < directory->parentdir->dircount; i++) {
            if (directory->parentdir->contents[i]->dir_ptr == directory) {
                directory->parentdir->contents[i]->dir_ptr= newdirectory;
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
