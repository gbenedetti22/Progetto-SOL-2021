#include <stdio.h>
#include <stdlib.h>
#include "../file_reader.h"
#include <string.h>
#include "../my_string.h"    //deve essere inclusa

int words_counted=0;

FILE* fileReader(char* filename){
    FILE* temp=fopen(filename,"r");
    if(temp==NULL){
        fprintf(stderr, "File non trovato!\n");
        exit(0);
    }
    return temp;
}

FILE* file_open(char* filename, char* mode){
    FILE* temp=fopen(filename,mode);
    if(temp==NULL){
        fprintf(stderr, "File non trovato!\n");
        exit(0);
    }
    return temp;
}

char* file_readline(FILE* file){
    static char* line;
    static size_t len;

    int read_lines=(int) getline(&line,&len,file);
    char* ret=str_create(line);
    if(read_lines == -1){
        return NULL;
    }
    words_counted=read_lines;

    str_removeNewLine(&ret);
    return ret;
}

void* file_readAll(FILE* file){
    int file_size=file_getsize(file);

    void* buffer= malloc(sizeof(char) * file_size);
    fread(buffer, sizeof(char), file_size, file);

    return buffer;
}

int file_getsize(FILE* file){
    fseek(file, 0L, SEEK_END);
    int size = (int) ftell(file);
    rewind(file);
    return size;
}

void file_close(FILE* fp){
    fclose(fp);
}
