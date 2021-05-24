//
// Created by gabry on 18/04/21.
//

#ifndef FILEREADER_FILE_READER_H
#define FILEREADER_FILE_READER_H
FILE* fileReader(char* path);
FILE* file_open(char* filename, char* mode);
char* file_readline(FILE* file);
void* file_readAll(FILE* file);
size_t file_getsize(FILE* file);
void file_close(FILE* file);
#endif //FILEREADER_FILE_READER_H
