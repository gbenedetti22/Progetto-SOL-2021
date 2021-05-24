#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/fs_client_api.h"

#define SOCK_NAME "../../socket/pippo"

struct timespec buildAbsTime(int sec) {
    struct timespec timeToWait;
    struct timeval now;

    gettimeofday(&now, NULL);

    timeToWait.tv_sec = now.tv_sec + sec;
    timeToWait.tv_nsec = (now.tv_usec + 1000UL * 1) * 1000UL;

    return timeToWait;
}

void connect1() {
    struct timespec abstime = buildAbsTime(1);
    int status = openConnection(SOCK_NAME, 1000, abstime);
    if (status != 0)
        exit(-1);
}

int main() {
    connect1();
    char* abs= realpath("../../file.txt",NULL);

    int status=openFile(abs, O_CREATE);
    if(status != 0){
        fprintf(stderr,"errore nella openfile\n");
        return -1;
    }

    status=writeFile("../../file.txt",NULL);
    if(status != 0){
        fprintf(stderr,"errore nella writefile\n");
        return -1;
    }

    char* msg=" ciao mondo";
    status=appendToFile(abs,msg, strlen(msg),NULL);
    if(status != 0){
        fprintf(stderr,"errore nella append\n");
        return -1;
    }


    void* buff;
    size_t s;

    status=readFile(abs,&buff,&s);
    if(status != 0){
        fprintf(stderr,"errore nella readfile\n");
        return -1;
    }

    printf("size: %zu | %s\n", s ,(char*)buff);

    status=closeFile(abs);
    printf("File chiuso: %d\n", status);
    closeConnection(SOCK_NAME);

    return 0;
}
