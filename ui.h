#ifndef UI_H
#define UI_H

void initui();
void handle_resize();
void render_contents(WINDOW* w,dir* directory);
void print_path(WINDOW* w,char* path);

extern WINDOW *w1, *w2, *w3, *w2w3, *cmdw, *pathw, *wbetweenw2w3;

#endif
