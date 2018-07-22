#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ncurses.h>
#include <sys/wait.h>
#include <ctype.h>

#include "dir.h"

int free_dir(dir* dir_info){
    int i;
    for(i = 0; i < dir_info->size;i++){
        free(dir_info->content[i]); 
    }
    free(dir_info->type);
    free(dir_info->path);
    free(dir_info);
}

void sort_dir(dir* directory){
    int i,j;
    char* temp;
    unsigned char temp2;
    for(i = 0;i<directory->size;i++){
        for(j =0;j < directory->size;j++){
            if(strcasecmp(directory->content[i],directory->content[j]) < 0){

                temp = directory->content[i];
                directory->content[i] = directory->content[j];
                directory->content[j] = temp;

                temp2 = directory->type[i];
                directory->type[i] = directory->type[j];
                directory->type[j] = temp2;

            }
        }
    }
    char** aux = (char**) malloc(sizeof(char*)*directory->size);
    unsigned char* auxtype = (unsigned char*) malloc(sizeof(char)*directory->size);
    j = 0;
    for(i = 0;i < directory->size;i++){
        if(directory->type[i] == DT_DIR){
            aux[j] = directory->content[i];
            directory->content[i] = NULL;
            auxtype[j] = DT_DIR;
            j++;
        }
    }
    for(i = 0;i < directory->size;i++){
        if (directory->content[i] != NULL){
            aux[j] = directory->content[i];
            auxtype[j] = directory->type[i];
            j++;
        }
    }
    for(i = 0;i < directory->size;i++){
        directory->content[i] = aux[i];
        directory->type[i] = auxtype[i];
    }
    free(aux);
    free(auxtype);
}

dir* read_directory(){
    char* directory_path = (char*) malloc(PATH_SIZE);
    getcwd(directory_path,PATH_SIZE);
    DIR* directory = opendir(directory_path);
    struct dirent *directory_entry;
    int i = 0;
    dir* dir_info = (dir*) malloc(sizeof(dir));
    dir_info->content = (char**) malloc(sizeof(char*));
    dir_info->type = (unsigned char*) malloc(sizeof(char));
    while(directory_entry = readdir(directory)){

        if(directory_entry->d_name[0] != '.' && strcmp(directory_entry->d_name,"..")){
            dir_info->content = (char**) realloc(dir_info->content,sizeof(char*)*i+1);
            dir_info->content[i] = (char*) malloc(FILENAME_SIZE);
            strcpy(dir_info->content[i],directory_entry->d_name);

            dir_info->type = (unsigned char*) realloc(dir_info->type,sizeof(unsigned char)*i+1);
            dir_info->type[i] = directory_entry->d_type;
            i++;
        }
    }
    dir_info->path = directory_path;
    dir_info->size = i;
    dir_info->cursor = 0;
    dir_info->index = 0;
    sort_dir(dir_info);
    return dir_info;
}

int find_entry(dir* directory){
    int i,j,count = 0;
    char ch;
    int* list = (int*) malloc(sizeof(int)*directory->size);
    for (i = 0; i < directory->size;i++)
        list[i] = 1;
    i = 0;
    while((ch = getch()) != 27){
        if(ch == 10 || count == 1){
            for(int h = 0;h < directory->size;h++)
                if(list[h] == 1){
                    free(list);
                    return h;
                }
            return -1;
        }
        count = 0;
        for(j = 0;j < directory->size;j++){
            if(list[j] == 1)
                if(tolower(directory->content[j][i]) != ch)
                    list[j] = 0;
                else 
                    count++;
        }
        i++;
    }
    free(list);
    return -1;
}

int run(char* argument){
    int childPid,status;
    char** arguments = (char**) malloc(sizeof(char)*3);
    arguments[0] = malloc(sizeof(char)*6);
    strcpy(arguments[0], "rifle");
    arguments[1] = argument;
    arguments[2] = (char*) NULL;
    childPid = fork();
    if(fork < 0){
        fprintf(stderr,"FAILED FORK");
        exit(EXIT_FAILURE);
    }
    if(childPid != 0){
        waitpid(-1,&status,0);
    }
    else
        execvp(arguments[0],arguments);

    return 0;
}
    


dir* open_entry(dir* directory){
    if (directory->cursor < 0){
        chdir("..");
        free_dir(directory);
        directory = read_directory();
        directory->cursor = 0;
        return directory;
    }
    if(directory->type[directory->cursor] == DT_DIR){
        chdir(directory->content[directory->cursor]);
        free_dir(directory);
        directory = read_directory();
        directory->cursor = 0;
    }
    else{
        run(directory->content[directory->cursor]);
        erase();
        refresh();
        curs_set(1);
        curs_set(0);
    }
    return directory;
}

void move_cursor(dir* directory,int yMax,int number){
    if(number < 0){
        if(directory->cursor > 0){
            directory->cursor += number;
            if (directory->cursor < 0)
                directory->cursor = 0;
            if (directory->index > directory->cursor){
                directory->index = directory->cursor;
                return;
            }
        }
    }
    else{
        if (directory->cursor < directory->size){
            directory->cursor += number;
            if(directory->cursor > directory->size-1)
                directory->cursor = directory->size-1;
            if (directory->cursor >= directory->index+yMax-1)
                directory->index = (directory->cursor - yMax)+2;
        }
    }
    
}

