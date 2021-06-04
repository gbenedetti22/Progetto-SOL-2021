#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../config_parser.h"
#include "../my_string.h"
#include "../file_reader.h"
#include <limits.h>
#include <errno.h>
#include "../myerrno.h"

#define CHECK_ULONG_LIMIT(x) if((x)>ULONG_MAX){ \
        (x)=ULONG_MAX;\
    }

#define CHECK_UINT_LIMIT(x) if((x)>UINT_MAX){\
        (x)=UINT_MAX;\
    }

size_t convert_str(char *s) {
    char *type;
    size_t converted_s = strtol(s, &type, 10);
    type = str_clean(type);

    if (str_equals_ic(type, "m")) return converted_s * 1024 * 1024;
    else if (str_equals_ic(type, "g")) return converted_s * 1024 * 1024 * 1024;
    else if (str_isEmpty(type)) return converted_s;

    errno = ERROR_CONV;
    return -1;
}

void settings_default(settings* s){
    s->N_THREAD_WORKERS=5;
    s->MAX_STORAGE_SPACE=209715200;
    s->STORAGE_BASE_SIZE=2097152;
    s->MAX_STORABLE_FILES=100;
    s->SOCK_PATH= str_create("mysock");
    s->PRINT_LOG=1;
}

void setConfigfor(settings *s, char *key, char *value) {
    key = str_clean(key);
    int converted_v = 0;
    size_t sp;

    if (str_equals(key, "N_THREAD_WORKERS")) {
        if (str_toInteger(&converted_v, value) != 0) {
            fprintf(stderr, "Error on parsing [%s]: default value set\n", key);
            return;
        }

        CHECK_UINT_LIMIT(converted_v)

        s->N_THREAD_WORKERS = converted_v;
    } else if (str_equals(key, "MAX_STORAGE_SPACE")) {
        errno = 0;
        sp = convert_str(value);
        if (errno == ERROR_CONV) {
            fprintf(stderr, "Error on parsing [%s]: default value set\n", key);
            return;
        }

        CHECK_ULONG_LIMIT(sp)

        s->MAX_STORAGE_SPACE = sp;
    } else if (str_equals(key, "MAX_STORABLE_FILES")) {
        if (str_toInteger(&converted_v, value) != 0) {
            fprintf(stderr, "Error on parsing [%s]: default value set\n", key);
            return;
        }

        CHECK_UINT_LIMIT(converted_v)

        s->MAX_STORABLE_FILES = converted_v;
    } else if (str_equals(key, "SOCK_PATH")) {
        free(s->SOCK_PATH);
        char *new_path = str_clean(value);
        s->SOCK_PATH = str_create(new_path);
    } else if (str_equals(key, "STORAGE_BASE_SIZE")) {
        errno = 0;
        sp = convert_str(value);
        if (errno == ERROR_CONV) {
            fprintf(stderr, "Error on parsing [%s]: default value set\n", key);
            return;
        }

        if (sp > s->MAX_STORAGE_SPACE) {
            sp = s->MAX_STORAGE_SPACE;
        }

        s->STORAGE_BASE_SIZE = sp;
    }else if(str_equals(key, "PRINT_LOG")){
        if (str_toInteger(&converted_v, value) != 0) {
            fprintf(stderr, "Error on parsing [%s]: default value set\n", key);
            return;
        }
        if(converted_v != 0 && converted_v != 1 && converted_v != 2){
            fprintf(stderr, "PRINT_LOG option must be 0, 1 or 2. See the documentation\n"
                            "Default value set\n");
            return;
        }

        s->PRINT_LOG = converted_v;
    }
}

void settings_load(settings *s, char *path) {
    FILE *c;

    if (path == NULL || str_isEmpty(path)) {
        c = fopen("config.ini", "r");
    } else {
        c = fopen(path, "r");
    }
    if(c==NULL){
        settings_default(s);
        return;
    }

    if(s->SOCK_PATH==NULL){
        s->SOCK_PATH= str_create("mysock");
    }

    char **array = NULL;
    char *line;
    while ((line = file_readline(c)) != NULL) {
        char* cleaned_line = str_clean(line);
        if (!str_startsWith(cleaned_line, "#") && !str_isEmpty(cleaned_line)) {
            int n=str_splitn(&array, cleaned_line, "=#", 3);
            setConfigfor(s, array[0], array[1]);

            str_clearArray(&array,n);
        }
        free(line);
    }

}

void settings_free(settings *s) {
    free(s->SOCK_PATH);
}

void settings_print(settings s) {
    enum Color c=MAGENTA;
    pcolor(c, "MAX_STORABLE_FILES:\t\t\t");
    printf("%u\n", s.MAX_STORABLE_FILES);

    pcolor(c, "MAX_STORAGE_SPACE (in bytes):\t\t");
    printf("%lu\n", s.MAX_STORAGE_SPACE);

    pcolor(c, "STORAGE_BASE_SIZE:\t\t\t");
    printf("%lu\n", s.STORAGE_BASE_SIZE);

    pcolor(c, "N_THREAD_WORKERS:\t\t\t");
    printf("%u\n", s.N_THREAD_WORKERS);

    pcolor(c, "PRINT_LOG:\t\t\t\t");
    printf("%d\n", s.PRINT_LOG);

    pcolor(c, "SOCK_PATH:\t\t\t\t");
    printf("%s\n", s.SOCK_PATH);
}