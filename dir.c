#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/wait.h>
#include <ctype.h>

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


void free_dir(dir *dir_info){
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

dir* read_directory() {
    int i = 0;
    int dircount = 0;

    dir* dir_info = calloc_or_die(1, sizeof(dir));
    dir_info->contents = calloc_or_die(1, sizeof(entry*));

    char* directory_path = (char*) malloc_or_die(PATH_SIZE);
    getcwd(directory_path, PATH_SIZE);
    DIR* directory = opendir(directory_path);
    struct dirent *directory_entry;

    while (directory_entry = readdir(directory)) {
        if (directory_entry->d_name[0] != '.' && strcmp(directory_entry->d_name, "..")) {
            dir_info->contents = realloc_or_die(dir_info->contents, sizeof(entry*) * i + 1);
            dir_info->contents[i] = calloc_or_die(1, sizeof(entry));
            dir_info->contents[i]->name = malloc_or_die(FILENAME_SIZE);
            strcpy(dir_info->contents[i]->name, directory_entry->d_name);
            dir_info->contents[i]->marked = 0;
            switch (directory_entry->d_type) {
                case DT_REG:
                    dir_info->contents[i]->type = ENTRY_TYPE_FILE;
                    break;
                case DT_DIR:
                    dir_info->contents[i]->type = ENTRY_TYPE_DIRECTORY;
                    dircount++;
                    break;
                case DT_LNK:
                    dir_info->contents[i]->type = ENTRY_TYPE_LINK;
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

    dir_info->dircount = dircount;
    dir_info->path = directory_path;
    dir_info->size = i;
    dir_info->markedcount = 0;
    dir_info->cursor = 0;
    dir_info->index = 0;
    dir_info->parentdir = NULL;
    sort_dir(dir_info);

    return dir_info;
}

void insert_dir(dir* directory) {
    int i, j = 0;

    for (i = 0; i < strlen(directory->path); i++) {
        if (directory->path[i] == '/') {
            j = i;
        }
    }

    j++;

    for (int i = 0; i < directory->parentdir->size; i++) {
        if (!strcmp(directory->path + j, directory->parentdir->contents[i]->name)) {
            directory->parentdir->contents[i]->dir_ptr = directory;
            directory->parentdir->cursor = i;
            break;
        }
    }
}

dir* up_dir(dir* directory) {
    if (!strcmp(directory->path, "/")) {
        return directory;
    }
    if (directory->parentdir) {
        if (directory->parentdir->parentdir) {
            chdir("..");
            return directory->parentdir;
        }
        else {
            chdir("..");
            char* currentdir = malloc_or_die(sizeof(*currentdir) * PATH_SIZE);
            getcwd(currentdir, PATH_SIZE);
            if (!strcmp(currentdir, "/")) {
                free(currentdir);
                return directory->parentdir;
            }
            free(currentdir);
            chdir("..");
            dir* temp;
            temp = read_directory();
            chdir(directory->parentdir->path);
            directory->parentdir->parentdir = temp;
            insert_dir(directory->parentdir);
            return directory->parentdir;
        }
    }
    return directory->parentdir;
}

dir* open_entry(dir* directory) {
    if (directory->contents[directory->cursor]->dir_ptr == NULL) {
        if (chdir(directory->contents[directory->cursor]->name) != -1) {
            dir* temp = read_directory();
            temp->parentdir = directory;
            directory->contents[directory->cursor]->dir_ptr = temp;
        }
        else {
            return directory;
        }
    }
    else { 
        chdir(directory->contents[directory->cursor]->dir_ptr->path);
    }

    return directory->contents[directory->cursor]->dir_ptr;
}

void move_cursor(dir* directory,int yMax,int number) {
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

dir* initdir() {
    dir* directory = read_directory();
    char* currentdir = malloc_or_die(sizeof(*currentdir)*PATH_SIZE);
    getcwd(currentdir, PATH_SIZE);
    if (strcmp(currentdir,"/")) {
        chdir("..");
        directory->parentdir = read_directory();
        chdir(directory->path);
        insert_dir(directory);
    }
    free(currentdir);
    return directory;
}

dir* reload_dir(dir* directory) {
    dir* newdirectory = read_directory();

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

    free_dir(directory);
    return newdirectory;
}
