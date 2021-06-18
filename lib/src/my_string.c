#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include <errno.h>
#include "../my_string.h"

#define MAX 64  //ogni quanto rialloco l array di string in split(s,delem)
#define NUMB_OF_STR(x) (x*sizeof(char*))

char *str_create(const char *s) {
    if (s == NULL || str_isEmpty(s)) {
        char *empty_s = malloc(sizeof(char));
        if(empty_s==NULL){
            fprintf(stderr, "str_create() malloc error\n");
            exit(errno);
        }
        strncpy(empty_s, "", 1);
        empty_s[0]=0;
        return empty_s;
    }

    return strdup(s);
}

int str_toInteger(int* output, char* s){
    char* rem;
    s = str_clean(s);
    int converted_s= (int) strtol(s,&rem,10);
    if(!str_isEmpty(rem)){
        return -1;
    }
    *output=converted_s;
    return 0;
}

bool str_equals(const char *s1, const char *s2) {
    if(s1==NULL || s2==NULL){
        return false;
    }

    if (strlen(s1) == strlen(s2)) {
        if (strncmp(s1, s2, strlen(s1)) == 0)
            return true;
    }

    return false;
}

bool str_equals_ic(const char *s1, const char *s2) {
    if(s1==NULL || s2==NULL){
        return false;
    }

    if (strlen(s1) == strlen(s2)) {
        if (strncasecmp(s1, s2, strlen(s1)) == 0)
            return true;
    }

    return false;
}

char *str_concat(const char *s1, const char *s2) {
    if(s1==NULL && s2 != NULL)
        return str_create(s2);
    else if(s1 != NULL && s2==NULL)
        return str_create(s1);

    char *concatenated_string = calloc(strlen(s1) + strlen(s2) + 1, sizeof(char));
    if(concatenated_string==NULL){
        return NULL;
    }
    strncpy(concatenated_string, s1, strlen(s1));
    concatenated_string[strlen(s1)]='\0';

    return strncat(concatenated_string, s2, strlen(s2));
}

char* str_concatn(const char* s1, ...){
    char* result= str_create(s1);
    va_list ap;
    va_start(ap,s1);
    char* s=NULL;
    while ((s = va_arg(ap, char*))!=NULL) {
        char* temp = str_concat(result, s);
        if(temp==NULL){
            free(result);
            return NULL;
        }
        free(result);
        result=temp;
    }

    va_end(ap);
    return result;
}

int str_split(char ***output, const char *s, const char *delimiter) {
    return str_splitn(output, s, delimiter, 0);
}

void str_clearArray(char*** array, const int lenght){
    for (int i = 0; i < lenght; i++) {
        free((*array)[i]);
    }

    free(*array);
}

int str_splitn(char ***output, const char *s, const char *delimiter, int n) {
    if(s==NULL)
        return -1;

    int max_elements = NUMB_OF_STR(MAX);    //numero di string massime all interno dell array
    *output = calloc(MAX, sizeof(char* ));

    if (*output == NULL) {
        printf("cannot allocate array");
        return -1;
    }

    int i = 0;
    int count = MAX;
    char *backup = str_create(s);

    char *token = strtok(backup, delimiter);
    while (token != NULL) {
        if(n>=0)
            n--;
        if (i == count) {
            count += MAX;
            max_elements += NUMB_OF_STR(MAX);
            char **temp = realloc(*output, max_elements);
            if (max_elements >= INT_MAX) {
                printf("Overflow error\n");
                return -1;
            }
            if (temp == NULL) {
                printf("cannot reallocate, index %d allocated %d bytes", i, max_elements);
                return -1;
            } else {
                *output = temp;
            }
        }

        if (n == 0) {
            //metto il corrente nell array
            (*output)[i] = str_create(token);
            i++;

            //metto il resto della stringa alla posizione successiva
            token = strtok(NULL, "");
            if (token != NULL) {
                (*output)[i] = str_create(token);
                i++;
            }
            break;
        }


        (*output)[i] = str_create(token);

        token = strtok(NULL, delimiter);
        i++;
    }

    free(backup);

    return i;
}

int str_startsWith(const char *s, const char *prefix) {
    if (!s || !prefix)
        return 0;
    int lenstr = str_length(s);
    int lenprefix = str_length(prefix);
    if (lenprefix > lenstr)
        return false;
    int result = strncmp(s, prefix, lenprefix);
    if (result != 0) {
        return false;
    }
    return true;
}

bool str_endsWith(const char *s, const char *suffix) {
    if (!s || !suffix)
        return false;

    int lenstr = str_length(s);
    int lensuffix = str_length(suffix);
    if (lensuffix > lenstr)
        return false;
    int result = strncmp(s + lenstr - lensuffix, suffix, lensuffix);
    if (result != 0) {
        return false;
    }
    return true;
}

bool str_isEmpty(const char *s) {
    return str_length(s) == 0;
}

char *str_cut(const char *s, int from, int to) {
    if ((from + to) > strlen(s)) {
        return NULL;
    }

    char *ret = calloc(to+1, sizeof(char));
    if (ret == NULL) {
        perror("malloc error on str_cut");
        return NULL;
    }

    strncpy(ret, s + from, to);
    ret[to]='\0';

    assert(str_length(ret)==to);
    return ret;
}

void str_removeNewLine(char **s) {
    (*s)[strcspn(*s, "\n")] = 0;
}

char *str_clean(char *s) {
    char *result = str_trim(s);
    str_removeNewLine(&result);
    return result;
}

char *str_trim(char *s) {
    while (isspace((unsigned char) *s)) s++;

    if (*s == 0)
        return s;

    char *end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char) *end)) end--;

    end[1] = '\0';

    return s;
}

char *str_long_toStr(long n) {
    char *result = malloc(sizeof(long));
    if(result==NULL){
        fprintf(stderr, "str_long_toStr malloc error: impossibile convertire long in string\n");
        exit(errno);
    }
    sprintf(result, "%ld", n);

    return result;
}

int str_length(const char *s) {
    return (int) strlen(s);
}