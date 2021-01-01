#ifndef DIR_H_
#define DIR_H_

#include "entry.h"

#define FILENAME_SIZE sizeof(char)*256
#define TYPE_SIZE sizeof(unsigned char)
#define PATH_SIZE sizeof(char)*4096

#define IS_DIR(directory, i) (directory->entry_array[i]->type == ENTRY_TYPE_DIRECTORY)
#define IS_PATH_ROOT(path) (path && path[0] == '/' && path[1] == '\0')

typedef struct dir dir;

struct dir{
    entry** entry_array;
    dir** dir_array;
    char* path;
    dir* parentdir;
    int size;
    int markedcount;
    int cursor;
    int index;
    int dircount;
};

void dir_delete(dir*);
dir* dir_create(const char*);
void dir_load_dir_at_cursor(dir*);
dir* dir_cd_cursor(dir*);
void dir_move_cursor(dir* , int, int);
void dir_insert(dir*);
dir* dir_up(dir*);
dir* dir_init(char*);
dir* dir_reload(dir*);
dir* dir_toggle_hidden(dir*);

#endif
