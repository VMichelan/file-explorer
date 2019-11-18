#ifndef DIR_H_
#define DIR_H_

#define FILENAME_SIZE sizeof(char)*256
#define TYPE_SIZE sizeof(unsigned char)
#define PATH_SIZE sizeof(char)*4096

#define ISDIR(directory, i) (i < directory->dircount)

typedef struct dir dir;

struct dir{
    char** content;
    int size;
    char* marked;
    int markedcount;
    int cursor;
    char* path;
    int index;
    dir* parentdir;
    dir** dirlist;
    int dircount;
};

void free_dir(dir* dir_info);
dir* read_directory();
dir* open_entry(dir* directory);
void move_cursor(dir* directory,int yMax,int number);
void insert_dir(dir* directory);
dir* up_dir(dir* directory);
dir* initdir();
dir* reload_dir(dir* directory);


#endif
