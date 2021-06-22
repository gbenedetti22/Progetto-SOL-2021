#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "lib/fs_client_api.h"

static struct timespec timespec_new() {
    struct timespec timeToWait;
    struct timeval now;

    gettimeofday(&now, NULL);

    timeToWait.tv_sec = now.tv_sec;
    timeToWait.tv_nsec = (now.tv_usec + 1000UL * 1) * 1000UL;

    return timeToWait;
}
int main(void) {
    //lanciare con make fifo_test

    openConnection("socket/mysock", 0, timespec_new());

    char* file1="./TestFolder/test2/archivio1.zip";    //145kb
    char* file2="./TestFolder/test2/archivio2.zip";    //294kb
    char* file3="./TestFolder/test2/archivio3.zip";    //647kb
    char* file4="./TestFolder/test2/archivio4.zip";    //330kb
    char* file5="./TestFolder/test2/archivio5.zip";    //330kb

    file1 = realpath(file1, NULL);
    file2 = realpath(file2, NULL);
    file3 = realpath(file3, NULL);
    file4 = realpath(file4, NULL);
    file5 = realpath(file5, NULL);

    openFile(file1, O_CREATE);  //creo file1 e non lo chiudo
    writeFile(file1,NULL);

    openFile(file2, O_CREATE);  //creo file 2
    writeFile(file2,NULL);
    closeFile(file2);       //chiudo file2, in caso di capacity misses verrà cancellato per primo

    openFile(file3, O_CREATE);      //capacity misses: espello il primo file non aperto (cioè file2)
    writeFile(file3,NULL);   //lo scrivo, adesso c'è spazio
    closeFile(file3);              //chiudo file3, in caso di capacity misses verrà cancellato per primo

    closeFile(file1);       //chiudo file1, in caso di capacity misses verrà cancellato per primo

    openFile(file4, O_CREATE);  //capacity misses: espello il primo file non aperto (cioè file1)
    writeFile(file4,NULL);  //lo scrivo e non lo chiudo

    openFile(file5, O_CREATE);  //capacity misses: espello il primo file non aperto (cioè file3)
    writeFile(file5,NULL);  //lo scrivo
    closeFile(file5);

    closeFile(file4);


    /* COMPORTAMENTO DELLA CODA (in ordine di inserimento):
     * + file1
     * + file2
     * + file3 -> capacity misses -> cancello file2
     * + file4 -> capacity misses -> cancello file1
     * + file5 -> capacity misses -> cancello file3
     *
     * output in TestFolder/downloadDir: file4.zip & file5.zip
     * */
    closeConnection("mysock");
    return 0;
}