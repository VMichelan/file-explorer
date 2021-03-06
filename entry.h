#ifndef ENTRY_H_
#define ENTRY_H_

enum ENTRY_TYPE {
    ENTRY_TYPE_UNKNOWN,
    ENTRY_TYPE_UNKNOWN_FILE,    //A file that hasn't been categorized yet
    ENTRY_TYPE_FILE,            //A file that has been categorized as generic file (doesn't fit any other category)
    ENTRY_TYPE_BINARY,
    ENTRY_TYPE_TEXT,
    ENTRY_TYPE_IMAGE,
    ENTRY_TYPE_VIDEO,
    ENTRY_TYPE_PDF,
    ENTRY_TYPE_ARCHIVE,
    ENTRY_TYPE_DIRECTORY,
    ENTRY_TYPE_LINK,
};

typedef struct entry entry;

struct entry {
    char* name;
    char marked;
    char islink;
    enum ENTRY_TYPE type;
    void* dir_ptr;
    char* preview;
};

#endif
