#ifndef DIR_H_
#define DIR_H_

#define FILENAME_SIZE sizeof(char)*256
#define TYPE_SIZE sizeof(unsigned char)
#define PATH_SIZE sizeof(char)*4096

#define ISDIR(directory, i) (i < directory->dircount)

enum ENTRY_TYPE {
    ENTRY_TYPE_UNKNOWN,
    ENTRY_TYPE_FILE,
    ENTRY_TYPE_BINARY,
    ENTRY_TYPE_TEXT,
    ENTRY_TYPE_IMAGE,
    ENTRY_TYPE_PDF,
    ENTRY_TYPE_ARCHIVE,
    ENTRY_TYPE_DIRECTORY,
    ENTRY_TYPE_LINK,
};

typedef struct entry entry;

typedef struct dir dir;

struct entry {
    char* name;
    char marked;
    enum ENTRY_TYPE type;
    dir* dir_ptr;
};

struct dir{
    entry** contents;
    char* path;
    dir* parentdir;
    int size;
    int markedcount;
    int cursor;
    int index;
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
