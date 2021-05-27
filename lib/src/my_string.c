#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <ctype.h>
#include <stdarg.h>
#include <assert.h>
#include "../my_string.h"

#define MAX 64  //ogni quanto rialloco l array di string in split(s,delem)
#define NUMB_OF_STR(x) (x*sizeof(char*))

char *str_create(char *s) {
    if (s == NULL || str_isEmpty(s)) {
        char *empty_s = malloc(sizeof(char));
        strncpy(empty_s, "", 1);
        empty_s[0]=0;
        return empty_s;
    }
    return strdup(s);
}

char str_charAt(char *s, int index) {
    if (index > strlen(s))
        return '\0';

    return s[index];
}

int str_toInteger(int* output, char* s){
    char* rem;
    int converted_s= (int) strtol(s,&rem,10);
    if(!str_isEmpty(rem)){
        return -1;
    }
    *output=converted_s;
    return 0;
}

bool str_equals(char *s1, char *s2) {
    if(s1==NULL || s2==NULL){
        return false;
    }

    if (strlen(s1) == strlen(s2)) {
        if (strncmp(s1, s2, strlen(s1)) == 0)
            return true;
    }

    return false;
}

bool str_equals_ic(char *s1, char *s2) {
    if(s1==NULL || s2==NULL){
        return false;
    }

    if (strlen(s1) == strlen(s2)) {
        if (strncasecmp(s1, s2, strlen(s1)) == 0)
            return true;
    }

    return false;
}

char *str_concat(char *s1, char *s2) {
    if(s1==NULL && s2 != NULL)
        return s2;
    else if(s1 != NULL && s2==NULL)
        return s1;

    char *concatenated_string = calloc(strlen(s1) + strlen(s2) + 2, sizeof(char));
    if(concatenated_string==NULL){
        return NULL;
    }
    strncpy(concatenated_string, s1, strlen(s1));
    concatenated_string[strlen(s1)]=0;

    return strncat(concatenated_string, s2, strlen(s2));
}

char* str_concatn(char* s1, ...){
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

int str_split(char ***output, char *s, char *delimiter) {
    return str_splitn(output, s, delimiter, 0);
}

char *current_s = NULL;
char *current_delim = NULL;
char *current_token = NULL;

void str_startTokenizer(char *s, char *delim) {
    if (s == NULL)
        return;

    if (!str_equals(s, current_s)) {
        free(current_s);
    }

    current_s = str_create(s);
    current_delim = delim;
    current_token = strtok(current_s, current_delim);
}

bool str_hasToken() {
    if (current_token == NULL) {
        free(current_s);
        return false;
    }

    return true;
}

char *str_getToken() {
    char* temp=current_token;
    current_token = strtok(NULL, current_delim);
    return temp;
}

void str_clearToken() {
    free(current_s);
}

void str_clearArray(char*** array, int lenght){
    for (int i = 0; i < lenght; i++) {
        free((*array)[i]);
    }

    free(*array);
}

int str_splitn(char ***output, char *s, char *delimiter, int n) {
    if(s==NULL)
        return -1;

    int max_elements = NUMB_OF_STR(MAX);    //numero di string massime all interno dell array
    int null_i= str_length(s);
    *output = calloc(MAX, sizeof(char* ));
    s[null_i]='\0';

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

int str_contains(char *s, char *word) {
    if (strstr(s, word) != NULL) {
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

int str_startsWith(char *s, char *prefix) {
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

bool str_endsWith(char *s, char *suffix) {
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

int str_firstOcc(char *s, char ch) {
    char *ptr = strchr(s, ch);
    if (ptr) {
        return (int) (ptr - s);
    } else {
        return -1;
    }
}

bool str_isEmpty(char *s) {
    return str_length(s) == 0;
}

char *str_replace(char *s, char *word_to_replace, char *replace_with) {
    if (str_contains(s, word_to_replace) != 0) {
        return NULL;
    }
    char *suffix = strstr(s, word_to_replace);

    int index = (int) (suffix - s);
    char *pref = malloc(index + 1);
    strncpy(pref, s, index);
    pref[index]=0;

    suffix = str_cut(suffix, str_length(word_to_replace), str_length(suffix) - str_length(word_to_replace));

    return str_concat(str_concat(pref, replace_with), suffix);
}

char *str_replace_all(char *s, char *word_to_replace, char *replace_with) {
    char *temp = str_create(s);
    char *result = NULL;
    while ((temp = str_replace(temp, word_to_replace, replace_with)) != NULL) {
        result = temp;
    }

    return result;
}

char *str_cut(char *s, int from, int to) {
    if ((from + to) > strlen(s)) {
        return NULL;
    }

    char *ret = malloc(to);
    if (ret == NULL) {
        perror("malloc error on str_cut");
        return NULL;
    }

    strncpy(ret, s + from, to);
    ret[to]=0;

    assert(str_length(ret)==to);
    return ret;
}

char *str_toUpper(char *s) {
    int len = str_length(s);
    char *out = malloc(len + 1);

    for (int i = 0; i < len; ++i) // converto carattere per carattere
        out[i] = (char) toupper((s)[i]);
    out[len] = '\0';

    return out;
}

char *str_toLower(char *s) {
    int len = str_length(s);
    char *out = malloc(len + 1);

    for (int i = 0; i < len; ++i) // converto carattere per carattere
        out[i] = (char) tolower((s)[i]);
    out[len] = '\0';

    return out;
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

char *str_int_toStr(int n) {
    char *result = malloc(sizeof(int));
    sprintf(result, "%d", n);

    return result;
}

char *str_float_toStr(float n) {
    char *result = malloc(sizeof(float));
    sprintf(result, "%f", n);

    return result;
}

char *str_double_toStr(double n) {
    char *result = malloc(sizeof(double));
    sprintf(result, "%f", n);

    return result;
}

char *str_long_toStr(long n) {
    char *result = malloc(sizeof(long));
    sprintf(result, "%ld", n);

    return result;
}

char *str_char_toStr(char c) {
    char *result = malloc(sizeof(char));
    sprintf(result, "%c", c);

    return result;
}

int str_length(char *s) {
    return (int) strlen(s);
}