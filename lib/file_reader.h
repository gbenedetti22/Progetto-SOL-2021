#include <stdbool.h>

#ifndef FILEREADER_FILE_READER_H
#define FILEREADER_FILE_READER_H
FILE* fileReader(char* path);
FILE* file_open(char* filename, char* mode);
char* file_readline(FILE* file);
void* file_readAll(FILE* file);
size_t file_getsize(FILE* file);
bool is_directory(const char *file);
/* La funzione file_scanAllDir() scannerizza ricorsivamente tutti
 * i file e le cartelle all interno di init_dir, riportando
 * il loro path assoluto dentro output.
 *
 * init_dir è la cartella iniziale di "start", successivamente
 * se una cartella viene trovata, anche questa viene scannerizzata.
 *
 * Return value:
 * 0 se il path iniziale dato è sbagliato, altrimenti
 * ritorna il numero di file trovati e inseriti all interno di
 * output, allocato opportunamente.
 * L array output deve essere passato inizializzato a NULL.
 *
 * */
int file_scanAllDir(char*** output, char* init_dir);
int file_nscanAllDir(char*** output, char* init_dir, int n);
#endif //FILEREADER_FILE_READER_H