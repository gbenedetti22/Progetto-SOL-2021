#include <stdbool.h>
#ifndef C_PROJECTS_MY_STRING_H
#define C_PROJECTS_MY_STRING_H

char* str_create(char* s);
char str_charAt(char* s, int index);
bool str_equals(char* s1, char* s2);
bool str_equals_ic(char* s1, char* s2);
char* str_concat(char* s1, char* s2);                   //ritorna s1+s2

/*Concatena n stringhe pre-allocate.
 *L ultimo argomento DEVE essere un NULL terminatore.
 *Se gli argomenti sono diversi da char* o l ultimo parametro
 *non è NULL, il risultato è undefined behaviour*/
char* str_concatn(char* s1, ...);
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
int str_split(char*** output, char* s, char* delimiter);
int str_splitn(char*** output, char* s, char* delimiter, int n);
int str_contains(char* s, char* word);
int str_startsWith(char* s, char* prefix);
bool str_endsWith(char* s, char* suffix);
int str_firstOcc(char* s, char ch);
int str_lastOcc(char* s, char ch);
bool str_isEmpty(char* s);
char* str_replace(char* s,char* word_to_replace, char* replace_with);          //cambia la prima occorrenza in s con replace_with
char* str_replace_all(char* s, char* word_to_replace, char* replace_with);       //cambia tutte le occorrenze di s con replace_with
char* str_cut(char* s, int from, int to);
char* str_toUpper(char* s);
char* str_toLower(char* s);
void str_removeNewLine(char** s);
char* str_clean(char* s);
char* str_trim(char* s);                                //s ma senza spazi bianchi

char* str_int_toStr(int n);
char* str_float_toStr(float n);
char* str_double_toStr(double n);
char* str_long_toStr(long n);
char* str_char_toStr(char c);
int str_length(char* s);
void str_startTokenizer(char* s, char* delim);
bool str_hasToken();
void str_clearToken();
char* str_getToken();
void str_clearArray(char*** array, int lenght);
int str_toInteger(int* output, char* s);
#endif
