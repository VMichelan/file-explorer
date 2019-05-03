#include <string.h>
#include <stdlib.h>
#include <ncurses.h>
#include <dirent.h>
#include <unistd.h>

#include "dir.h"

#define W1FACTOR 1/5
#define W2FACTOR 2/5
#define W3FACTOR 2/5

WINDOW *w1,*w2,*w3,*cmdw,*pathw;

int utf8_strlen(char* str,int len)
{
    int c,i,ix,q;
    for (q=0, i=0, ix=len; i < ix; i++, q++)
    {
        c = (unsigned char) str[i];
        if      (c>=0   && c<=127) i+=0;
        else if ((c & 0xE0) == 0xC0) i+=1;
        else if ((c & 0xF0) == 0xE0) i+=2;
        else if ((c & 0xF8) == 0xF0) i+=3;
        //else if (($c & 0xFC) == 0xF8) i+=4; // 111110bb //byte 5, unnecessary in 4 byte UTF-8
        //else if (($c & 0xFE) == 0xFC) i+=5; // 1111110b //byte 6, unnecessary in 4 byte UTF-8
        else return 0;//invalid utf8
    }
    return q;
}

void initui(){
    int yMax,xMax;
    getmaxyx(stdscr,yMax,xMax);
    w1 = newwin(yMax-2,xMax*W1FACTOR,1,0);
    w2 = newwin(yMax-2,xMax*W2FACTOR,1,xMax*W1FACTOR+1);
    w3 = newwin(yMax-2,xMax*W3FACTOR,1,(xMax*W1FACTOR+1)+(xMax*W2FACTOR)+1);
    pathw = newwin(1,xMax,0,0);
    cmdw = newwin(1,xMax,yMax-1,0);
    keypad(w2,TRUE);

}

int getchar(){
    return wgetch(w2);
}

void handle_resize(){
    int yMax,xMax,i=0;
    getmaxyx(stdscr,yMax,xMax); 
    while (i < yMax){
        wmove(w3,i,0);
        wclrtoeol(w3);
        i++;
    }
    clear();
    refresh();
    delwin(w1);
    delwin(w2);
    delwin(w3);
    delwin(pathw);
    delwin(cmdw);
    initui();
    wmove(cmdw,0,0);
    wclrtoeol(cmdw);
    wrefresh(cmdw);

}

void print_path(WINDOW* w,char* path){
    wmove(w,0,0);
    wclrtoeol(w);
    wattron(w,COLOR_PAIR(2));
    mvwprintw(w,0,0,path);
    wattroff(w,COLOR_PAIR(2));
    wrefresh(w);
}

void render_contents(WINDOW* w,dir* directory){
    if (!directory)
        return;
    int i = 0,j,yMax,xMax,len,count,index = directory->index;
    char ch;
    getmaxyx(w,yMax,xMax);
    if(directory->size == 0){
        mvwprintw(w,i,0," ");
        wclrtoeol(w);
        wattron(w,COLOR_PAIR(3));
        wprintw(w,"***EMPTY***");
        wattroff(w,COLOR_PAIR(3));
        i++;
    }
    while(index < directory->size && i < yMax){
        if(directory->cursor == index)
            wattron(w,A_STANDOUT);
        else
            wattroff(w,A_STANDOUT);
        if(directory->type[index] == DT_DIR)
            wattron(w,COLOR_PAIR(1));

        len = strlen(directory->content[index]);
        len = utf8_strlen(directory->content[index],len);

        mvwprintw(w,i,0," ");
        if (len > xMax){
            ch = directory->content[index][xMax];
            directory->content[index][xMax] = '\0';
            mvwprintw(w,i,1,"%s\n",directory->content[index]);
            directory->content[index][xMax] = ch;
        }
        else
            mvwprintw(w,i,1,"%s\n",directory->content[index]);

        for(j = len+1;j < xMax;j++)
            mvwprintw(w,i,j," ");

        index++;
        i++;
        wattroff(w,COLOR_PAIR(1));
   }
    while (i < yMax){
        wmove(w,i,0);
        wclrtoeol(w);
        i++;
    }
    wattroff(w,A_STANDOUT);
    wrefresh(w);

}
