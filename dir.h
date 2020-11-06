#ifndef DIR_H_
#define DIR_H_

#define FILENAME_SIZE sizeof(char)*256
#define TYPE_SIZE sizeof(unsigned char)
#define PATH_SIZE sizeof(char)*4096

#define IS_DIR(directory, i) (i < directory->dircount)
#define IS_PATH_ROOT(path) (path && path[0] == '/' && path[1] == '\0')

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
    char islink;
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

void dir_delete(dir*);
dir* dir_create(const char*);
dir* dir_cd_cursor(dir*);
void dir_move_cursor(dir* , int, int);
void dir_insert(dir*);
dir* dir_up(dir*);
dir* dir_init();
dir* dir_reload(dir*);


#endif
