#ifndef RUN_H_
#define RUN_H_

int run(char* file,int newterm);
int open_terminal();
void extract_file(char* filename);
void run_shell();
void copy_to_clipboard(char** filenames, int count);
void run_preview(char* file, char* preview, int previewsize);

#endif


