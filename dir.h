#ifndef DIR_H_
#define DIR_H_

#define FILENAME_SIZE sizeof(char)*256
#define TYPE_SIZE sizeof(unsigned char)
#define PATH_SIZE sizeof(char)*4096

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

#define IS_DIR(directory, i) (directory->contents[i]->type == ENTRY_TYPE_DIRECTORY)
#define IS_PATH_ROOT(path) (path && path[0] == '/' && path[1] == '\0')

typedef struct entry entry;

typedef struct dir dir;

struct entry {
    char* name;
    char marked;
    char islink;
    enum ENTRY_TYPE type;
    dir* dir_ptr;
    char* preview;
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
void dir_load_dir_at_cursor(dir*);
dir* dir_cd_cursor(dir*);
void dir_move_cursor(dir* , int, int);
void dir_insert(dir*);
dir* dir_up(dir*);
dir* dir_init();
dir* dir_reload(dir*);


#endif
