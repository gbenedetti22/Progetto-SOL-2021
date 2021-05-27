#include <errno.h>

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

#ifndef PROGETTO_MYERRNO_H
#define PROGETTO_MYERRNO_H
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define WHT   "\x1B[37m"
#define RESET "\x1B[0m"

static void psucc(char *op, char *file) {
    if (file == NULL) {
        printf(GRN "Operazione: [%s] avvenuta con successo!\n\n", op);
        printf(RESET);
        return;
    }
    printf(GRN "Operazione: [%s] sul file [%s] avvenuta con successo!\n\n", op, file);
    printf(RESET);
}

static void pcode(int code) {
    switch (code) {
        case SFILE_ALREADY_EXIST : {
            fprintf(stderr, "ERRORE: Il file è già presente sul Server\n"
                            "Codice: SFILE_ALREADY_EXIST\n\n");
            break;
        }

        case SFILE_NOT_FOUND : {
            fprintf(stderr, "ERRORE: Il file non è presente sul Server\n"
                            "Codice: SFILE_NOT_FOUND\n\n");
            break;
        }

        case SFILE_ALREADY_OPENED : {
            printf(YEL "WARNING: Il file è già stato aperto\n"
                   "Codice: SFILE_ALREADY_OPENED\n\n");
            break;
        }
        case SFILE_NOT_OPENED : {
            fprintf(stderr, "ERRORE: Il file non è stato aperto\n"
                            "Operazioni di scrittura non ammesse su file chiusi\n"
                            "Codice: SFILE_NOT_OPENED\n\n");
            break;
        }

        case SFILE_NOT_EMPTY : {
            fprintf(stderr, "ERRORE: Operazioni di Write() non consentite su file non vuoti\n"
                            "Codice: SFILE_NOT_EMPTY\n\n");
            break;
        }

        case S_STORAGE_EMPTY : {
            printf(YEL "WARNING: Il Server non contiene file\n"
                   "Codice: S_STORAGE_EMPTY\n\n");
            break;
        }

        case SFILES_FOUND_ON_EXIT : {
            printf(YEL "WARNING: Alcuni file non sono stati chiusi.\n"
                   "Si prega di chiuderli una volta usati\n"
                   "Codice: SFILES_FOUND_ON_EXIT\n\n");
            break;
        }

        case SOCKET_ALREADY_CLOSED : {
            fprintf(stderr, "ERRORE: Il socket è già stato chiuso\n"
                            "Codice: SOCKET_ALREADY_CLOSED\n\n");
            break;
        }

        case SFILE_TOO_LARGE : {
            fprintf(stderr, "ERRORE: File troppo grande\n"
                            "Codice: SFILE_TOO_LARGE\n\n");
            break;
        }

        case SFILE_OPENED : {
            printf(YEL "WARNING: Tentata operazione (forse di cancellazione ?) su file aperto.\n"
                   "E' obbligatorio prima chiuderlo, per questioni si sicurezza.\n"
                   "Codice: SFILE_OPENED\n\n");
            break;
        }

        case CONNECTION_TIMED_OUT : {
            fprintf(stderr, "ERRORE: Impossibile stabilire una connessione con il Server\n"
                            "Codice: CONNECTION_TIMED_OUT\n\n");
            break;
        }

        case WRONG_SOCKET : {
            fprintf(stderr, "ERRORE: Il Socket passato come argomento non corrisponde al Socket\n"
                            "con cui questo Client si è connesso\n"
                            "Codice: WRONG_SOCKET\n\n");
            break;
        }

        case FILE_NOT_FOUND : {
            fprintf(stderr, "ERRORE: File non trovato\n"
                            "Codice: FILE_NOT_FOUND\n\n");
            break;
        }

        case INVALID_ARG : {
            fprintf(stderr, "ERRORE: Argomento passato non valido\n"
                            "Codice: INVALID_ARG\n\n");
            break;
        }

        case S_FREE_ERROR : {
            fprintf(stderr, "ERRORE: Impossibile fare spazio sul Server\n"
                            "Prova a chiudere qualche file\n"
                            "Codice: INVALID_ARG\n\n");
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
#undef RESET
#endif //PROGETTO_MYERRNO1_H