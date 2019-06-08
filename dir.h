#ifndef DIR_H_
#define DIR_H_

#define FILENAME_SIZE sizeof(char)*256
#define TYPE_SIZE sizeof(unsigned char)
#define PATH_SIZE sizeof(char)*4096

typedef struct dir dir;

struct dir{
    char** content;
    int size;
    int cursor;
    unsigned char* type;
    char* path;
    int index;
    dir* parentdir;
    dir** dirlist;
    int dircount;
};

int free_dir(dir* dir_info);
void sort_dir(dir* directory);
dir* read_directory();
int find_entry(dir* directory);
dir* open_entry(dir* directory);
void move_cursor(dir* directory,int yMax,int number);
void insert_dir(dir* directory,dir* ins);
dir* up_dir(dir* directory);
dir* initdir();
dir* reload_dir(dir* directory);


#endif
