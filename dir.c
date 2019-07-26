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


int free_dir(dir* dir_info){
    int i;
    if (dir_info == NULL) {
        return 0;
    }
    for (i = 0; i < dir_info->size;i++) {
        free(dir_info->content[i]); 
    }
    free(dir_info->type);
    free(dir_info->path);
    free(dir_info);
}

void sort_dir(dir* directory){
    int i,j;
    char* temp;
    unsigned char temp2;
    for (i = 0;i<directory->size;i++) {
        for (j =0;j < directory->size;j++) {
            if (strcasecmp(directory->content[i],directory->content[j]) < 0) {

                temp = directory->content[i];
                directory->content[i] = directory->content[j];
                directory->content[j] = temp;

                temp2 = directory->type[i];
                directory->type[i] = directory->type[j];
                directory->type[j] = temp2;

            }
        }
    }
    char** aux = (char**) malloc(sizeof(char*)*directory->size);
    unsigned char* auxtype = (unsigned char*) malloc(sizeof(char)*directory->size);
    j = 0;
    for (i = 0;i < directory->size;i++) {
        if (directory->type[i] == DT_DIR) {
            aux[j] = directory->content[i];
            directory->content[i] = NULL;
            auxtype[j] = DT_DIR;
            j++;
        }
    }
    for (i = 0;i < directory->size;i++) {
        if (directory->content[i] != NULL) {
            aux[j] = directory->content[i];
            auxtype[j] = directory->type[i];
            j++;
        }
    }
    for (i = 0;i < directory->size;i++) {
        directory->content[i] = aux[i];
        directory->type[i] = auxtype[i];
    }
    free(aux);
    free(auxtype);
}

dir* read_directory() {
    char* directory_path = (char*) malloc_or_die(PATH_SIZE);
    getcwd(directory_path,PATH_SIZE);
    DIR* directory = opendir(directory_path);
    struct dirent *directory_entry;
    int i = 0;
    dir* dir_info = (dir*) calloc_or_die(1,sizeof(dir));
    dir_info->content = (char**) calloc_or_die(1,sizeof(char*));
    dir_info->type = (unsigned char*) calloc_or_die(1,sizeof(char));
    int dircount = 0;
    while (directory_entry = readdir(directory)) {

        if (directory_entry->d_name[0] != '.' && strcmp(directory_entry->d_name,"..")) {
            dir_info->content = (char**) realloc_or_die(dir_info->content,sizeof(char*)*i+1);
            dir_info->content[i] = (char*) calloc_or_die(1,FILENAME_SIZE);
            strcpy(dir_info->content[i],directory_entry->d_name);
            if(directory_entry->d_type == DT_DIR) {
                dircount++;
            }

            dir_info->type = (unsigned char*) realloc_or_die(dir_info->type,sizeof(unsigned char)*i+1);
            dir_info->type[i] = directory_entry->d_type;
            i++;
        }
    }

    closedir(directory);

    dir_info->dirlist = calloc_or_die((dircount+1),sizeof(dir*));
    dir_info->dircount = dircount;
    dir_info->path = directory_path;
    dir_info->size = i;
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
        if (!strcmp(directory->path + j, directory->parentdir->content[i])) {
            directory->parentdir->dirlist[i] = directory;
            directory->parentdir->cursor = i;
            break;
        }
    }
}

int find_entry(dir* directory) {
    int i,j,count = 0;
    char ch;
    int* list = (int*) malloc(sizeof(int)*directory->size);
    for (i = 0; i < directory->size;i++) {
        list[i] = 1;
    }
    i = 0;
    while ((ch = getch()) != 27) {
        if (ch == 10 || count == 1) {
            for (int h = 0;h < directory->size;h++) {
                if (list[h] == 1) {
                    free(list);
                    return h;
                } 
            }
            return -1;
        }
        count = 0;
        for (j = 0;j < directory->size;j++) {
            if (list[j] == 1) {
                if (tolower(directory->content[j][i]) != ch) {
                    list[j] = 0;
                }
                else {
                    count++;
                }
            }
        }
        i++;
    }
    free(list);
    return -1;
}

dir* up_dir(dir* directory) {
    if (!strcmp(directory->path,"/")) {
        return directory;
    }
    if (directory->parentdir) {
        if (directory->parentdir->parentdir) {
            chdir("..");
            return directory->parentdir;
        }

        else {
            chdir("..");
            char* currentdir = malloc_or_die(sizeof(*currentdir)*PATH_SIZE);
            getcwd(currentdir, PATH_SIZE);
            if (!strcmp(currentdir,"/")) {
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
    if (directory->dirlist[directory->cursor] == NULL) {
        if (chdir(directory->content[directory->cursor]) != -1) {
            dir* temp = read_directory();
            temp->parentdir = directory;
            directory->dirlist[directory->cursor] = temp;
        }
        else {
            return directory;
        }
    }
    else { 
        chdir(directory->dirlist[directory->cursor]->path);
    }

    return directory->dirlist[directory->cursor];
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
    chdir("..");
    directory->parentdir = read_directory();
    chdir(directory->path);
    insert_dir(directory);
    return directory;
}

dir* reload_dir(dir* directory) {
    dir* newdirectory = read_directory();

    for (int i = 0; i < directory->dircount; i++) {
        if (directory->dirlist[i]) {
            directory->dirlist[i]->parentdir = newdirectory;
        }
    }

    for (int i = 0; i < newdirectory->dircount; i++) {
        for (int j = i; j < directory->dircount; j++) {
            if (!strcmp(newdirectory->content[i], directory->content[j])) {
                newdirectory->dirlist[i] = directory->dirlist[j];
                directory->dirlist[j] = NULL;
                break;
            }
        }
    }

    // frees any dir that doesn't exist anymore
    for (int i = 0; i < directory->dircount; i++)
        free_dir(directory->dirlist[i]);

    if (directory->parentdir) {
        for (int i = 0; i < directory->parentdir->dircount; i++) {
            if (directory->parentdir->dirlist[i] == directory) {
                directory->parentdir->dirlist[i] = newdirectory;
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
