#define _GNU_SOURCE
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

static struct timespec timespec_new() {
    struct timespec timeToWait;
    struct timeval now;

    gettimeofday(&now, NULL);

    timeToWait.tv_sec = now.tv_sec;
    timeToWait.tv_nsec = (now.tv_usec + 1000UL * 1) * 1000UL;

    return timeToWait;
}

void sendfile_toServer(const char *backup_folder, char *file) {
    int errcode;
    if (openFile(file, O_CREATE) != 0) {
        pcode(errno, file);
        return;
    }
    if(p_op)
        printf("Invio di %s....\n", file);
    if (writeFile(file, backup_folder) != 0) {
        fprintf(stderr, "Writefile: errore nell invio del file %s al Server\n", file);
        errcode = errno;
        pcode(errcode, file);
    } else if(p_op){
        psucc("File \"%s\" inviato!\n\n", strrchr(file, '/') + 1);
    }

    char *filepath = realpath(file, NULL);
    closeFile(filepath);
    free(filepath);
}

void print_commands() {
    printf("\t-h \t\t\tPrints this helper.\n");
    printf("\t-f <sock> \t\tSets socket name to <sock>. \033[0;31m This option must be set once and only once. \033[0m\n");
    printf("\t-p \t\t\tIf set, every operation will be printed to stdout. \033[0;31m This option must be set at most once. \033[0m\n");
    printf("\t-t <time> \t\tSets the waiting time (in milliseconds) between requests. Default is 0.\n");
    printf("\t-a <time> \t\tSets the time (in seconds) after which the app will stop attempting to connect to server. Default value is 5 seconds. \033[0;31m This option must be set at most once. \033[0m\n");
    printf("\t-w <dir>[,<num>] \tSends the content of the directory <dir> (and its subdirectories) to the server. If <num> is specified, at most <num> files will be sent to the server.\n");
    printf("\t-W <file>{,<files>}\tSends the files passed as arguments to the server.\n");
    printf("\t-D <dir>\t\tWrites into directory <dir> all the files expelled by the server app. \033[0;31m This option must follow one of -w or -W. \033[0m\n");
    printf("\t-r <file>{,<files>}\tReads the files specified in the argument list from the server.\n");
    printf("\t-R[<num>] \t\tReads <num> files from the server. If <num> is not specified, reads all files from the server. \033[0;31m There must be no space bewteen -R and <num>.\033[0m\n");
    printf("\t-d <dir> \t\tWrites into directory <dir> the files read from server. If it not specified, files read from server will be lost. \033[0;31m This option must follow one of -r or -R. \033[0m\n");
    printf("\t-l <file>{,<files>} \tLocks all the files given in the file list.\n");
    printf("\t-u <file>{,<files>} \tUnlocks all the files given in the file list.\n");
    printf("\t-c <file>{,<files>} \tDeletes from server all the files given in the file list, if they exist.\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *download_folder = NULL;   //-d
    char *backup_folder = NULL;     //-D
    char *myargv[argc];
    int myargc = 0;
    bool found = false;
    bool found_r = false;
    bool found_w = false;

    for (int i = 0; i < argc; i++) {
        if (str_startsWith(argv[i], "-f")) {
            found = true;
            sockname = ((argv[i]) += 2);
            if (openConnection(sockname, 0, timespec_new()) != 0) {
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
            if(str_startsWith(argv[i],"-r") || str_startsWith(argv[i],"-R")){
                found_r=true;
            }else if(str_startsWith(argv[i],"-w") || str_startsWith(argv[i],"-W")){
                found_w=true;
            }
            myargv[myargc] = argv[i];
            myargc++;
        }
    }

    if (!found) {
        perr( "Opzione Socket -f[sockname] non specificata\n");
        perr( "Inserire il Server a cui connettersi\n");
        return -1;
    }

    if(!found_r && download_folder != NULL){
        perr("L opzione -d va usata congiuntamente con l opzione -r o -R");
    }

    if(!found_w && backup_folder != NULL){
        perr("L opzione -D va usata congiuntamente con l opzione -w o -W");
    }

    int opt, errcode;
    while ((opt = getopt(myargc, myargv, ":h:w:W:r:R::c:")) != -1) {
        switch (opt) {
            case 'h': {
                print_commands();
                break;
            }

            case 'w': {
                char **array = NULL;
                char **files = NULL;
                int n_files = -1;
                int count;
                int n = str_split(&array, optarg, ",");
                if (n > 2) {
                    perr("Troppi argomenti per il comando -w dirname[,n=x]\n");
                    break;
                }

                if (n == 2) {
                    if(str_toInteger(&n_files, array[1])!=0){
                        perr("%s non è un numero\n", optarg);
                        break;
                    }
                }

                count = file_nscanAllDir(&files, array[0], n_files);
                for (int i = 0; i < count; i++) {
                    sendfile_toServer(backup_folder, files[i]);
                    usleep(t_sleep * 1000);
                }
                str_clearArray(&array, n);
                str_clearArray(&files, count);
                break;
            }

            case 'W': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                for (int i = 0; i < n; i++) {
                    sendfile_toServer(backup_folder, files[i]);
                    usleep(t_sleep * 1000);
                }
                str_clearArray(&files, n);
                break;
            }

            case 'r': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                void *buff;
                size_t size;
                for (int i = 0; i < n; i++) {
                    if(openFile(files[i], O_OPEN) == 0) {   // apro il file
                        if (readFile(files[i], &buff, &size) != 0) {    //lo leggo
                            perr("ReadFile: Errore nel file %s\n", files[i]);
                            errcode = errno;
                            pcode(errcode, files[i]);
                        } else {    //a questo punto o salvo i file nella cartella dirname (se diversa da NULL) o dò un messaggio di conferma
                            char *filename = strrchr(files[i], '/') + 1;

                            if (download_folder != NULL) {
                                if (!str_endsWith(download_folder, "/")) {
                                    download_folder = str_concat(download_folder, "/");
                                }
                                char *path = str_concatn(download_folder, filename, NULL);
                                FILE *file = fopen(path, "wb");
                                if (file == NULL) {
                                    perr("Cartella %s sbagliata\n", path);
                                    download_folder = NULL;
                                } else {
                                    if (fwrite(buff, sizeof(char), size, file) == 0) {  //scrivo il file ricevuto nella cartella download_folder
                                        perr("Errore nella scrittura di %s\n"
                                             "I successivi file verranno ignorati\n", path);
                                        download_folder = NULL;
                                    } else if (p_op) {
                                        pcolor(GREEN, "File \"%s\" scritto nella cartella: ", filename);
                                        printf("%s\n\n", path);
                                    }
                                    fclose(file);
                                }
                                free(path);
                            } else if (p_op) {
                                psucc("Ricevuto file: ");
                                printf("%s\n\n", filename);
                            }
                            free(buff);
                        }
                        if(closeFile(files[i])!=0){
                            perr("CloseFile: Errore nella chiusura del file %s\n", files[i]);
                            errcode = errno;
                            pcode(errcode, files[i]);
                        }
                    }else{
                        perr("OpenFile: Errore nell apertura del file %s\n", files[i]);
                        errcode = errno;
                        pcode(errcode, files[i]);
                    }
                    usleep(t_sleep * 1000);
                }

                str_clearArray(&files, n);
                break;
            }

            case 'R': {
                int n = 0;
                if (optarg != NULL) {
                    optarg++;
                    if (str_toInteger(&n, optarg) != 0) {
                        perr("%s non è un numero\n", optarg);
                        break;
                    }
                }
                if (readNFiles(n, download_folder) != 0) {
                    errcode = errno;
                    pcode(errcode, NULL);
                } else if(p_op){
                    psucc("Ricevuti %d file\n\n", n);
                }
                break;
            }

            case 'c': {
                char **files = NULL;
                int n = str_split(&files, optarg, ",");
                for (int i = 0; i < n; i++) {
                    if (removeFile(files[i]) != 0) {
                        errcode = errno;
                        pcode(errcode, files[i]);
                        perr( "RemoveFile: errore sul file %s\n", files[i]);
                    } else if(p_op){
                        psucc("File %s rimosso con successo\n\n", files[i]);
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

    if (closeConnection(sockname) != 0) {
        errcode = errno;
        pcode(errcode, NULL);
    }

    //Per rimuovere i warning
    pwarn("");
    return 0;
}