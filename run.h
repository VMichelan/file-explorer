#ifndef RUN_H_
#define RUN_H_

int run_open_file(char*, int);
int run_open_terminal();
void run_extract_file(char* filename);
void run_shell();
void run_copy_to_clipboard(char** filenames, int count);
void run_preview(char* file, char* preview, int previewsize);

#endif


