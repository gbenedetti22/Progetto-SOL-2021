#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/fs_client_api.h"

struct timespec buildAbsTime(int sec) {
    struct timespec timeToWait;
    struct timeval now;

    gettimeofday(&now, NULL);

    timeToWait.tv_sec = now.tv_sec + sec;
    timeToWait.tv_nsec = (now.tv_usec + 1000UL * 1) * 1000UL;

    return timeToWait;
}

int main() {
    struct timespec abstime = buildAbsTime(1);
    int status = openConnection("../../socket/pippo", 1000, abstime);
    if (status != 0)
        return -1;


    openFile("file.txt", O_CREATE);
    writeFile("../../file.txt",NULL);
    void* buff;
    size_t s;
    char* abs= realpath("../../file.txt",NULL);
    readFile(abs,&buff,&s);
    printf("%s\n", (char*) buff);

    return 0;
}