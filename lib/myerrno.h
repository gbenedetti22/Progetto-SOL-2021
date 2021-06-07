#include <errno.h>
#include <stdarg.h>

#define S_SUCCESS 0
#define SFILE_ALREADY_EXIST 1
#define SFILE_NOT_FOUND 2
#define SFILE_ALREADY_OPENED 3
#define SFILE_NOT_OPENED 4
#define SFILE_NOT_EMPTY 5
#define S_STORAGE_EMPTY 6
#define SFILES_FOUND_ON_EXIT 7
#define SOCKET_ALREADY_CLOSED 8
#define S_STORAGE_FULL 9    //non è un errore, in quanto se il Server è pieno, dei file vengono rimossi
#define EOS_F 10 //end-of-stream-files
#define HASH_NULL_PARAM 11
#define HASH_INSERT_SUCCESS 12
#define HASH_DUPLICATE_KEY 13
#define HASH_KEY_NOT_FOUND 14
#define SFILE_TOO_LARGE 15
#define SFILE_OPENED 16
#define CONNECTION_TIMED_OUT 17
#define WRONG_SOCKET 18
#define FILE_NOT_FOUND 19
#define INVALID_ARG 20
#define S_FREE_ERROR 21
#define CONNECTION_REFUSED 22
#define CONNECTION_ACCEPTED 23
#define MALLOC_ERROR 24

#ifndef PROGETTO_MYERRNO_H
#define PROGETTO_MYERRNO_H
#define RD    "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define MAG   "\x1B[35m"
#define CYN   "\x1B[36m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

enum Color {RED='r', GREEN='g', YELLOW='y', BLUE='b', MAGENTA='m', CYAN='c', WHITE='w', STANDARD='d'};

static void pcolor(enum Color c, char* s, ...){
    va_list argp;
    va_start(argp, s);
    char* p;
    switch (c) {
        case RED:
            p = str_concat(RD,s);
            break;
        case GREEN:
            p = str_concat(GRN,s);
            break;
        case YELLOW:
            p = str_concat(YEL,s);
            break;
        case BLUE:
            p = str_concat(BLU,s);
            break;
        case MAGENTA:
            p = str_concat(MAG,s);
            break;
        case CYAN:
            p = str_concat(CYN,s);
            break;
        case WHITE:
            p = str_concat(WHT,s);
            break;
        case STANDARD:
            p = str_concat(RESET,s);
            break;
    }

    vprintf(p, argp);
    va_end(argp);
    free(p);
    printf(RESET);
}

static void psucc(char *s, ...) {
    va_list argp;
    va_start(argp, s);
    char* p= str_concat(GRN,s);
    vprintf(p, argp);
    va_end(argp);
    free(p);
    printf(RESET);
}

static void pwarn(char* s,...){
    va_list argp;
    va_start(argp, s);
    char* p= str_concat(YEL,s);
    vprintf(p, argp);
    va_end(argp);
    free(p);
    printf(RESET);
}

static void perr(char* s,...){
    va_list argp;
    va_start(argp, s);
    char* p= str_concat(RD,s);
    vprintf(p, argp);
    va_end(argp);
    free(p);
    printf(RESET);
}

static void pcode(int code, char* file) {
    if(file==NULL)
        file="(null)";

    switch (code) {
        case SFILE_ALREADY_EXIST : {
            perr("ERRORE: Il file %s è già presente sul Server\n"
                            "Codice: SFILE_ALREADY_EXIST\n\n", file);
            break;
        }

        case SFILE_NOT_FOUND : {
            perr("ERRORE: Il file %s non è presente sul Server\n"
                            "Codice: SFILE_NOT_FOUND\n\n", file);
            break;
        }

        case SFILE_ALREADY_OPENED : {
            printf(YEL "WARNING: Il file %s è già stato aperto\n"
                   "Codice: SFILE_ALREADY_OPENED\n\n", file);
            break;
        }
        case SFILE_NOT_OPENED : {
            perr("ERRORE: Il file %s non è stato aperto\n"
                            "Operazioni di scrittura non ammesse su file chiusi\n"
                            "Codice: SFILE_NOT_OPENED\n\n", file);
            break;
        }

        case SFILE_NOT_EMPTY : {
            perr("ERRORE: Operazioni di Write() non consentite su file non vuoti\n"
                            "File: %s\n"
                            "Codice: SFILE_NOT_EMPTY\n\n", file);
            break;
        }

        case S_STORAGE_EMPTY : {
            printf(YEL "WARNING: Il Server non contiene file\n"
                   "Codice: S_STORAGE_EMPTY\n\n");
            break;
        }

        case SFILES_FOUND_ON_EXIT : {
            printf(YEL "WARNING: Alcuni file non sono stati chiusi.\n"
                   "Il Server chiuderà i file per te, ma si prega di chiuderli una volta usati\n"
                   "Codice: SFILES_FOUND_ON_EXIT\n\n");
            break;
        }

        case SOCKET_ALREADY_CLOSED : {
            perr("ERRORE: Il socket è già stato chiuso\n"
                            "Codice: SOCKET_ALREADY_CLOSED\n\n");
            break;
        }

        case SFILE_TOO_LARGE : {
            perr("ERRORE: Il file %s è troppo grande\n"
                            "Codice: SFILE_TOO_LARGE\n\n", file);
            break;
        }

        case SFILE_OPENED : {
            printf(YEL "WARNING: Tentata operazione (forse di cancellazione ?) su file aperto.\n"
                   "E' obbligatorio prima chiuderlo, per questioni si sicurezza.\n"
                   "Codice: SFILE_OPENED\n\n");
            break;
        }

        case CONNECTION_TIMED_OUT : {
            perr("ERRORE: Impossibile stabilire una connessione con il Server\n"
                            "Codice: CONNECTION_TIMED_OUT\n\n");
            break;
        }

        case WRONG_SOCKET : {
            perr("ERRORE: Il Socket passato come argomento non corrisponde al Socket\n"
                            "con cui questo Client si è connesso\n"
                            "Codice: WRONG_SOCKET\n\n");
            break;
        }

        case FILE_NOT_FOUND : {
            perr("ERRORE: File %s non trovato\n"
                            "Codice: FILE_NOT_FOUND\n\n", file);
            break;
        }

        case INVALID_ARG : {
            perr("ERRORE: Argomento passato non valido\n"
                            "Codice: INVALID_ARG\n\n");
            break;
        }

        case S_FREE_ERROR : {
            perr("ERRORE: Impossibile fare spazio sul Server\n"
                            "Prova a chiudere qualche file\n"
                            "Codice: S_FREE_ERROR\n\n");
            break;
        }

        case CONNECTION_REFUSED : {
            perr("ERRORE: Impossibile stabilire una connessione con il Server.\n"
                            "Codice: CONNECTION_REFUSED\n\n");
            break;
        }
        case MALLOC_ERROR : {
            perr("ERRORE: Una malloc sul Server non è andata a buon fine\n"
                 "Prova a cancellare qualche file e riprova\n"
                 "Codice: MALLOC_ERROR\n\n");
            break;
        }


        default: {
            break;
        }
    }
}

#undef GRN
#undef YEL
#undef WHT
#undef RD
#undef RESET
#endif //PROGETTO_MYERRNO_H