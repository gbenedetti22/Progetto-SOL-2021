#include <stdbool.h>
#ifndef C_PROJECTS_MY_STRING_H
#define C_PROJECTS_MY_STRING_H

char* str_create(const char* s);
bool str_equals(const char* s1, const char* s2);
bool str_equals_ic(const char* s1, const char* s2);
char* str_concat(const char* s1, const char* s2); //appende s2 a s1

/*Concatena n stringhe pre-allocate.
 *L ultimo argomento DEVE essere un NULL terminatore.
 *Se gli argomenti sono diversi da char* o l ultimo parametro
 *non è NULL, il risultato è undefined behaviour*/
char* str_concatn(const char* s1, ...);
/* Divide la stringa sulla base del delimiter passato come argomento.
 * Se non viene trovato nessun delimiter nella strinag s, allora output conterrà
 * la stringa originale.
 *
 * output DEVE essere passato inizializzato a NULL.
 * In caso di successo, output conterrà la stringa s divisa sulla
 * base di delimiter e la funzione ritornerà il numero di elementi di output.
 * E' bene chiamare la funzione str_clearArray() una volta che output non serve più
 * per non incorrere in memory leak.
 * */
int str_split(char*** output, const char* s, const char* delimiter);
int str_splitn(char*** output, const char* s, const char* delimiter, int n);
int str_startsWith(const char* s, const char* prefix);
bool str_endsWith(const char* s, const char* suffix);
bool str_isEmpty(const char* s);
char* str_cut(const char* s, int from, int to);
void str_removeNewLine(char** s);
char* str_clean(char* s);
char* str_trim(char* s);                                //s ma senza spazi bianchi

char* str_long_toStr(long n);
int str_length(const char* s);
void str_clearArray(char*** array, const int lenght);
int str_toInteger(int* output, char* s);
#endif
