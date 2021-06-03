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

struct timespec timespec_new(int sec) {
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
        pcode(errno, file);
        return;
    }

    printf("Invio di %s....\n", file);
    if (writeFile(file, backup_folder) != 0) {
        fprintf(stderr, "Errore nell invio del file %s al Server\n", file);
        errcode = errno;
        pcode(errcode, file);
    } else {
        psucc("File \"%s\" inviato!\n\n", strrchr(file, '/') + 1);
    }

    char *filepath = realpath(file, NULL);
    closeFile(filepath);
}

int main(int argc, char *argv[]) {
    char *download_folder = NULL;   //-d
    char *backup_folder = NULL;     //-D
    char *myargv[argc];
    int myargc = 0;
    bool found = false;

    for (int i = 0; i < argc; i++) {
        if (str_startsWith(argv[i], "-f")) {
            found = true;
            sockname = ((argv[i]) += 2);
            if (openConnection(sockname, 0, timespec_new(0)) != 0) {
                pcode(errno, NULL);
                exit(errno);
            }
        } else if (str_startsWith(argv[i], "-d")) {
            download_folder = ((argv[i]) += 2);
        } else if (str_startsWith(argv[i], "-D")) {
            backup_folder = ((argv[i]) += 2);
        } else if (str_startsWith(argv[i], "-t")) {
            str_toInteger(&t_sleep, (argv[i]) += 2);
        } else if (str_startsWith(argv[i], "-p")) {
            p_op = true;
        } else {
            myargv[myargc] = argv[i];
            myargc++;
        }
    }

    if (!found) {
        fprintf(stderr, "Opzione Socket -f[sockname] non specificata\n");
        fprintf(stderr, "Inserire il Server a cui connettersi\n");
        return -1;
    }
    int opt, errcode;
    while ((opt = getopt(myargc, myargv, ":h:w:W:r:R::c:")) != -1) {
        switch (opt) {
            case 'h': {
                printf("Comandi supportati:\n"
                       "h;f;w;W;D;r;R;d;t;c;p\n");
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
                    usleep(t_sleep * 1000);
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
                    usleep(t_sleep * 1000);
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
                        pcode(errcode, files[i]);
                    } else {
                        psucc("%s: %zu | Ricevuto\n", files[i], size, (char *) buff);
                        if (download_folder != NULL) {
                            if (!str_endsWith(download_folder, "/")) {
                                download_folder = str_concat(download_folder, "/");
                            }

                            char *path = str_concatn(download_folder, (strrchr(files[i], '/') + 1), NULL);
                            printf("%s\n", path);
                            FILE *file = fopen(path, "wb");
                            if (file == NULL) {
                                perr("Cartella %s sbagliata\n", path);
                                download_folder = NULL;
                            } else {
                                if (fwrite(buff, sizeof(char), size, file) == 0) {
                                    perr("Errore nella scrittura di %s\n"
                                         "I successivi file verranno ignorati\n", path);
                                    download_folder = NULL;
                                }
                                fclose(file);
                            }
                        }
                        free(buff);
                    }

                    usleep(t_sleep * 1000);
                }

                str_clearArray(&files, n);
                download_folder = NULL;
                break;
            }

            case 'R': {
                int n = 0;
                if (optarg != NULL) {
                    optarg++;
                    if (str_toInteger(&n, optarg) != 0) {
                        fprintf(stderr, "%s non è un numero\n", optarg);
                        break;
                    }
                }
                if (readNFiles(n, download_folder) != 0) {
                    errcode = errno;
                    pcode(errcode, NULL);
                }
                download_folder = NULL;
                break;
            }

            case 'c': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                for (int i = 0; i < n; i++) {
                    if (removeFile(files[i]) != 0) {
                        errcode = errno;
                        pcode(errcode, files[i]);
                        fprintf(stderr, "RemoveFile: errore sul file %s\n", files[i]);
                    } else {
                        psucc("File %s rimosso con successo\n", files[i]);
                    }
                    usleep(t_sleep * 1000);
                }
                str_clearArray(&files, n);
                break;
            }

            default: {
                printf("not supported\n");
                break;
            }

        }
    }

    if (backup_folder != NULL) {
        fprintf(stderr, "Errore: -D deve essere usata con -w o -W\n");
    }

    if (download_folder != NULL) {
        fprintf(stderr, "Errore: -d deve essere usata con -r o -R\n");
    }

    if (closeConnection(sockname) != 0) {
        errcode = errno;
        pcode(errcode, NULL);
    }
    return 0;
}