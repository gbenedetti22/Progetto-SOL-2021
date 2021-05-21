#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config_parser.h"
#include "../my_string.h"
#include "../file_reader.h"

int convert_str(char* s){
    char* type;
    int converted_s= (int) strtol(s,&type,10);
    type= str_clean(type);

    if(str_equals_ic(type, "m")) return converted_s*1024*1024;
    else if(str_equals_ic(type, "g")) return converted_s*1024*1024*1024;
    else if(str_isEmpty(type)) return converted_s;

    return 0;
}

void setConfigfor(settings* s, char* key, char* value){
    key = str_clean(key);
    int converted_v=convert_str(value);
    if(!converted_v && !str_equals(key,"SOCK_PATH")){
        fprintf(stderr, "Error on parsing [%s]: default value set\n", key);
        return;
    }

    if(str_equals(key, "N_THREAD_WORKERS")){
            s->N_THREAD_WORKERS = converted_v;
    } else if(str_equals(key, "MAX_STORAGE_SPACE")){
            s->MAX_STORAGE_SPACE= converted_v;
    }else if(str_equals(key, "MAX_STORABLE_FILES")){
            s->MAX_STORABLE_FILES= converted_v;
    }else if(str_equals(key, "SOCK_PATH")){
        free(s->SOCK_PATH);
        char* new_path= str_clean(value);
        s->SOCK_PATH= str_create(new_path);
    }else if(str_equals(key,"STORAGE_BASE_SIZE")){
        if(converted_v > s->MAX_STORAGE_SPACE){
            fprintf(stderr, "Error on parsing [%s]: cannot be less than MAX_STORAGE_SPACE, default value set\n", key);
            converted_v -= s->MAX_STORAGE_SPACE;
        }
        s->STORAGE_BASE_SIZE=converted_v;
    }
}

int settings_load(settings* s, char* path){
    if(!str_endsWith(path,".ini")){
        fprintf(stderr, "Config file not valid\n");
        return NOT_VALID_CONFIG;
    }

    FILE* config;

    if(path == NULL || str_isEmpty(path)){
        config = fileReader("config.ini");
    } else{
        config = fileReader(path);
    }

    if(config==NULL)
        return CONFIG_NOT_FOUND;

    if(str_isEmpty(s->SOCK_PATH)){
        s->SOCK_PATH= str_create("mysock");
    }

    char** array=NULL;
    char* line;
    while((line=file_readline(config)) != NULL){
        line = str_clean(line);
        if(!str_startsWith(line,"#") && !str_isEmpty(line)) {
            str_splitn(&array, line, "=#", 3);
            setConfigfor(s, array[0], array[1]);

            free(array[0]);
            free(array[1]);
            if(array[2] != NULL)
                free(array[2]);
        }
    }

    free(array);
    return 0;
}

void settings_free(settings* s){
    free(s->SOCK_PATH);
}

void settings_print(settings s){
    printf("MAX_STORABLE_FILES:\t\t%d\n", s.MAX_STORABLE_FILES);
    printf("MAX_STORAGE_SPACE:\t\t%d\n", s.MAX_STORAGE_SPACE);
    printf("STORAGE_BASE_SIZE:\t\t%d\n", s.STORAGE_BASE_SIZE);
    printf("N_THREAD_WORKERS:\t\t%d\n", s.N_THREAD_WORKERS);
    printf("SOCK_PATH:\t\t\t\t%s\n", s.SOCK_PATH);
}

//int main() {
//    settings config = DEFAULT_SETTINGS;
//    settings_load(&config,"../config.ini");
//
//    // do stuff...
//
//    settings_print(config);
//    settings_free(&config);
//
//    return 0;
//}
