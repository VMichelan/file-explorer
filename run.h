#ifndef RUN_H_
#define RUN_H_

#include "entry.h"

int run_open_file(entry *, int);
int run_open_terminal();
void run_extract_file(entry *);
void run_shell();
void run_copy_to_clipboard(char**, int);
void run_preview(char*, entry*, int, int, int, int);
void run_clear_image_preview(char*, entry*, int, int, int, int);
void run_cleanup();

#endif


