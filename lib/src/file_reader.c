#include <stdio.h>
#include <stdlib.h>
#include "../file_reader.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include "../my_string.h"
#include <assert.h>


FILE* fileReader(char* filename){
    FILE* temp=fopen(filename,"r");
    if(temp==NULL){
        fprintf(stderr, "File non trovato!\n");
        exit(0);
    }
    return temp;
}

char* file_readline(FILE* file){
    static char* line;
    static size_t len;
    static int words_counted=0;

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
    size_t file_size=file_getsize(file);

    void* buffer= malloc(sizeof(char) * file_size);
    fread(buffer, sizeof(char), file_size, file);

    return buffer;
}

size_t file_getsize(FILE* file){
    fseek(file, 0L, SEEK_END);
    size_t size = ftell(file);
    rewind(file);
    return size;
}

void file_close(FILE* fp){
    fclose(fp);
}

bool is_directory(const char *file)
{
    struct stat p;
    stat(file, &p);
    return S_ISDIR(p.st_mode);
}

int file_scanAllDir(char*** output, char* init_dir){
    return file_nscanAllDir(output,init_dir,-1);
}

int file_nscanAllDir(char*** output, char* init_dir, int n)
{
    static int max=2;
    static int current_length=0;
    static int count=0;
    count=n;
    DIR *current_dir = opendir(init_dir);

    if (current_dir == NULL || count==0)
    {
        return current_length;
    }

    if(*output==NULL){
        *output = calloc(max, sizeof(char*));
    }

    struct dirent *file;

    while ((file = readdir(current_dir)) != NULL)
    {
        if(count==0)
            break;

        if(current_length==max){
            max*=2;
            *output = realloc(*output, max * sizeof(char*));
        }
        char *file_name = file->d_name;

        if (strcmp(".", file_name) != 0 && strcmp("..", file_name) != 0 && file_name[0] != '.')
        {
            file_name= str_concatn(init_dir,"/",file_name,NULL);
            char* filepath= realpath(file_name,NULL);
            assert(filepath != NULL);

            if(!is_directory(filepath)) {
                (*output)[current_length] = filepath;
                free(file_name);
                current_length++;
                count--;
            } else
            {
                current_length=file_nscanAllDir(output, filepath, count);
            }
        }
    }

    closedir(current_dir);
    return current_length;
}