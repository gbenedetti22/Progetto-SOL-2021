#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include "lib/fs_client_api.h"
#include "lib/my_string.h"
#include "lib/file_reader.h"
#include "lib/myerrno.h"

char *sockname = NULL;
int t_sleep = 0;
bool p_op = false;

struct timespec buildAbsTime(int sec) {
    struct timespec timeToWait;
    struct timeval now;

    gettimeofday(&now, NULL);

    timeToWait.tv_sec = now.tv_sec + sec;
    timeToWait.tv_nsec = (now.tv_usec + 1000UL * 1) * 1000UL;

    return timeToWait;
}

void sendfile_toServer(const char *backup_folder, char *file) {
    int errcode;
    if (openFile(file, O_CREATE) != 0) {
        errcode = errno;
        if (errcode != SFILE_ALREADY_EXIST) {
            pcode(errcode);
            return;
        }

        file = realpath(file,NULL);
        if (openFile(file, O_OPEN) != 0) {
            fprintf(stderr, "Errore: file %s invalido\n", file);

            errcode = errno;
            pcode(errcode);
            return;
        }
    }

    if (writeFile(file, backup_folder) != 0) {
        fprintf(stderr, "Errore nell invio del file %s al Server\n", file);
        errcode = errno;
        pcode(errcode);
    } else{
        psucc("Invio file", file);
    }

    char *filepath = realpath(file, NULL);
    closeFile(filepath);
}

int main(int argc, char *argv[]) {
    char *backup_folder = NULL;
    bool found = false;

    for (int i = 0; i < argc; i++) {
        if (str_startsWith(argv[i], "-f")) {
            found = true;
        }
    }

    if (!found) {
        fprintf(stderr, "Opzione Socket -f[sockname] non specificata\n");
        fprintf(stderr, "Inserire il Server a cui connettersi\n");
        return -1;
    }

    int opt, errcode;
    while ((opt = getopt(argc, argv, ":h:t:f:w:D:W:r:R::d:c:p")) != -1) {
        switch (opt) {
            case 'h': {
                printf("Comandi supportati:\n"
                       "h;f;w;W;D;r;R;d;t;c;p\n");
                break;
            }

            case 'f': {
                if (openConnection(optarg, 0, buildAbsTime(0)) != 0) {
                    errcode = errno;
                    pcode(errcode);
                    return -1;
                }
                sockname = optarg;
                break;
            }

            case 't': {
                if (optarg != NULL)
                    str_toInteger(&t_sleep, optarg);
                break;
            }

            case 'w': {
                char **array = NULL;
                char **files = NULL;
                int n_files = -1;
                int count;
                int n = str_split(&array, optarg, ",");
                if (n > 2) {
                    fprintf(stderr, "Troppi argomenti per il comando -w dirname[,n=x]\n");
                    break;
                }

                if (n == 2) {
                    str_toInteger(&n_files, array[1]);
                }

                count = file_nscanAllDir(&files, array[0], n_files);
                for (int i = 0; i < count; i++) {
                    sendfile_toServer(backup_folder, files[i]);
                }
                str_clearArray(&array, n);
                str_clearArray(&files, count);
                backup_folder = NULL;
                break;
            }

            case 'W': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                for (int i = 0; i < n; i++) {
                    sendfile_toServer(backup_folder, files[i]);
                }
                backup_folder = NULL;
                str_clearArray(&files, n);
                break;
            }

            case 'r': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                void *buff;
                size_t size;
                for (int i = 0; i < n; i++) {
                    if (readFile(files[i], &buff, &size) != 0) {
                        fprintf(stderr, "Errore nel file %s\n", files[i]);
                        errcode = errno;
                        pcode(errcode);
                    } else {
                        printf("%s: %zu, buff: \"%s\"\n", files[i], size, (char *) buff);
                    }
                }
                str_clearArray(&files, n);
                break;
            }

            case 'D': {
                backup_folder = optarg;
                break;
            }

            case 'R': {
                int n = 0;
                if (optarg != NULL) {
                    if(str_toInteger(&n, optarg)!=0){
                        fprintf(stderr, "%s non è un numero\n", optarg);
                        break;
                    }
                }
                if (readNFiles(n, backup_folder) != 0) {
                    fprintf(stderr, "Errore readNFiles\n");
                    errcode = errno;
                    pcode(errcode);
                }
                break;
            }

            case 'd': {
                backup_folder = optarg;
                break;
            }

            case 'c': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                for (int i = 0; i < n; i++) {
                    if (removeFile(files[i]) != 0) {
                        errcode = errno;
                        pcode(errcode);
                        fprintf(stderr, "RemoveFile: errore sul file %s\n", files[i]);
                    }
                }
                str_clearArray(&files, n);
                break;
            }

            case 'p': {
                p_op = true;
                break;
            }

            default: {
                printf("not supported\n");
                break;
            }

        }
    }

    if (backup_folder != NULL) {
        fprintf(stderr, "Errore: -D e -d devono essere usati con -w/-W/-r/-R\n");
    }
    if (closeConnection(sockname) != 0) {
        errcode = errno;
        pcode(errcode);
    }
    return 0;
}