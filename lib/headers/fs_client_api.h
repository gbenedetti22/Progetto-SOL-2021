#define O_OPEN 0
#define O_CREATE 1
#define O_LOCK 2

#ifndef CLIENT_API_FS_CLIENT_API_H
#define CLIENT_API_FS_CLIENT_API_H

int openConnection(const char* sockname, int msec, const struct timespec abstime);
int closeConnection(const char* sockname);
int openFile(const char* pathname, int flags);
int readFile(const char* pathname, void** buf, size_t* size);
int writeFile(const char* pathname, const char* dirname);
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);
int closeFile(const char* pathname);
int removeFile(const char* pathname);
#endif //CLIENT_API_FS_CLIENT_API_H
