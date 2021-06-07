#define _GNU_SOURCE
#include <stdio.h>
#include "lib/mysocket.h"
#include "lib/hash_table.h"
#include "lib/config_parser.h"
#include "lib/my_string.h"
#include "lib/queue.h"
#include "lib/sorted_list.h"
#include "lib/myerrno.h"
#include <pthread.h>
#include <signal.h>
#include <assert.h>
#include <limits.h>
#include <sys/time.h>
#include <stdlib.h>

#define MASTER_WAKEUP_SECONDS 10
#define MASTER_WAKEUP_MS 0

bool soft_close = false;
bool server_running = true;

hash_table *storage;            //contiene tutti i file associati al loro path assoluto
hash_table *opened_files;       //memorizza quanti file un determinato client ha aperto
settings config = DEFAULT_SETTINGS;
queue *q;
list *fifo;
size_t storage_space;
size_t max_storable_files;
size_t fifo_counts = 0;
int pipe_fd[2];
int connected_clients = 0;

typedef struct {
    char *path;
    void *content;
    size_t size;
    list *pidlist;
} file_s;

void print_files(char *key, void *value, bool *exit, void *argv) {
    pcolor(MAGENTA, "%s: ", (strrchr(key, '/') + 1));
    printf("%s\n", key);
}

static file_s *file_init(char *path) {
    if (str_isEmpty(path))
        return NULL;

    file_s *file = malloc(sizeof(file_s));
    if (file == NULL) {
        perr("Impossibile il file %s\n", (strrchr(path, '/') + 1));
        return NULL;
    }
    file->path = str_create(path);
    file->content = 0;
    file->size = 0;
    file->pidlist = list_create();
    return file;
}

void file_update(file_s **f, void *newContent, size_t newSize) {
    if (newSize == 0) {   //se il file è vuoto
        free(newContent);
        return;
    }
    free((*f)->content);
    storage_space += (*f)->size;

    (*f)->content = newContent;
    (*f)->size = newSize;
}

bool file_removeClient(file_s **f, char *cpid) {
    return list_remove(&(*f)->pidlist, cpid, NULL);
}

bool file_isEmpty(file_s *f) {
    return f->content == NULL;
}

bool file_isOpened(file_s *f) {
    return !list_isEmpty(f->pidlist);
}

bool file_isOpenedBy(file_s *f, char *pid) {
    return list_conteinsKey(f->pidlist, pid);
}

int file_appendClient(file_s **f, char *clientPid) {
    if (file_isOpenedBy(*f, clientPid)) {
        return -1;
    }

    list_insert(&(*f)->pidlist, clientPid, NULL);
    return 0;
}

void file_open(file_s **f, char *cpid) {
    file_appendClient(f, cpid);
    int *n = (int *) hash_getValue(opened_files, cpid);
    *n += 1;
    hash_updateValue(&opened_files, cpid, n, NULL);
}

void file_clientClose(file_s **f, char *cpid) {
    file_removeClient(f, cpid);
    int *n = (int *) hash_getValue(opened_files, cpid);
    *n -= 1;
    hash_updateValue(&opened_files, cpid, n, NULL);

    assert((*(int *) hash_getValue(opened_files, cpid)) >= 0);
}

void file_destroy(void *f) {
    file_s *file = (file_s *) f;
    free(file->content);
    free(file->path);
    list_destroy(&file->pidlist, NULL);
    free(file);
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void init_server(char *config_path) {
    settings_load(&config, config_path);
    storage = hash_create(config.MAX_STORABLE_FILES);
    opened_files = hash_create(config.MAX_STORABLE_FILES);
    q = queue_create();
    max_storable_files = config.MAX_STORABLE_FILES;
    storage_space = config.MAX_STORAGE_SPACE;
    fifo = list_create();
    pipe(pipe_fd);
    if (config.MAX_STORAGE_SPACE >= INT_MAX) {
        fprintf(stderr, "MAX INT REACHED\n");
        exit(-1);
    }

    sigset_t mask;
    sigfillset(&mask);
    pthread_sigmask(SIG_SETMASK, &mask, NULL);
}

void close_server() {
    settings_free(&config);
    hash_destroy(&storage, &file_destroy);
    hash_destroy(&opened_files, &free);
    close(pipe_fd[0]);
    close(pipe_fd[1]);
    list_destroy(&fifo, NULL);
    queue_destroy(&q);

    psucc("Copyright Benedetti Gabriele - Matricola 602202\n");
}

void free_space(int client, char option, size_t fsize) {
    node *curr = fifo->head;

    while (true) {
        if (curr==NULL) {
            if(config.PRINT_LOG==2){
                psucc("Lettura coda FIFO terminata\n\n");
            }
            sendInteger(client, EOS_F);
            return;
        }

        file_s *f = (file_s *) curr->value;
        assert(f != NULL && hash_containsKey(storage, f->path));

        if (!file_isOpened(f)) {
            if(config.PRINT_LOG==2){
                printf("Rimuovo il file %s dalla coda\n", (strrchr(f->path, '/')+1));
            }

            if (option == 'y') {  //caso in cui il file viene espulso e inviato al client
                sendInteger(client, !EOS_F);
                sendStr(client, f->path);
                sendn(client, f->content, f->size);
            }

            if (option == 'c') {  //caso in cui il client tenta di creare un file,
                //ma il numero massimo di file memorizzabili è 0. Vengono quindi
                //inviati al Client i nomi dei file che stanno per essere espulsi

                sendInteger(client, !EOS_F);
                sendStr(client, f->path);
            }

            storage_space += f->size;
            if (storage_space > config.MAX_STORAGE_SPACE) {
                storage_space = config.MAX_STORAGE_SPACE;
            }

            hash_deleteKey(&storage, f->path, &file_destroy);
            max_storable_files++;
            fifo_counts++;

            char* key=curr->key;
            curr=curr->next;
            list_remove(&fifo,key,NULL);
        }
        else{
            curr=curr->next;
        }

        if (fsize <= storage_space && max_storable_files > 0) {
            sendInteger(client, EOS_F);
            return;
        }
    }
}

void openFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];

    if (!hash_containsKey(storage, filepath)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file che non esiste\n", client);
        }

        if (config.PRINT_LOG==1){
            perr("Richiesta non eseguita");
        }
        sendInteger(client, SFILE_NOT_FOUND);
        str_clearArray(&array, n);
        return;
    }

    file_s *f = hash_getValue(storage, filepath);
    if (file_isOpenedBy(f, cpid)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha tentato di aprire un file già aperto\n", client);
        }
        if (config.PRINT_LOG==1){
            perr("Richiesta non eseguita");
        }

        sendInteger(client, SFILE_ALREADY_OPENED);
        str_clearArray(&array, n);
        return;
    }

    file_open(&f, cpid);
    sendInteger(client, S_SUCCESS);

    if (config.PRINT_LOG==1 || config.PRINT_LOG == 2) {
        psucc("Il client %d ha aperto il file %s\n", (strrchr(filepath,'/')+1), client);
    }
    str_clearArray(&array, n);
}

void createFile(int client, char *request) {
    size_t fsize = receiveInteger(client);

    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];
    assert(!str_isEmpty(filepath) && filepath != NULL);

    if (hash_containsKey(storage, filepath)) {
        if (config.PRINT_LOG==2){
            pwarn("Il client %d ha tentato di creare il file %s, che già esiste sul Server\n", client, (strrchr(filepath,'/')+1));
        }
        if (config.PRINT_LOG==1){
            perr("Richiesta non eseguita\n");
        }
        sendInteger(client, SFILE_ALREADY_EXIST);
    } else if (fsize > config.MAX_STORAGE_SPACE) { //se il file è troppo grande
        if (config.PRINT_LOG==2){
            pwarn("Il client %d ha tentato di mettere un file troppo grande\n", client);
        }

        if (config.PRINT_LOG==1){
            perr("Richiesta non eseguita\n");
        }
        sendInteger(client, SFILE_TOO_LARGE);
    } else if (max_storable_files == 0) {
        if (config.PRINT_LOG==2){
            pwarn("Rilevata CAPACITY MISSES\n", client);
        }

        sendInteger(client, S_STORAGE_FULL);
        free_space(client, 'c', 0);
        if (max_storable_files == 0) {
            if (config.PRINT_LOG==2){
                pwarn("Impossibile espellere file\n", client);
            }

            if (config.PRINT_LOG==1){
                perr("Richiesta non eseguita\n");
            }
            sendInteger(client, S_STORAGE_FULL);
        }
    } else {
        assert(hash_containsKey(opened_files, cpid));

        file_s *f = file_init(filepath);//creo un nuovo file
        if (f == NULL) {
            sendInteger(client, MALLOC_ERROR);
            return;
        }
        hash_insert(&storage, filepath, f);//lo memorizzo
        file_open(&f, cpid);//lo apro

        max_storable_files--;//aggiorno il numero massimo di file memorizzabili
        list_insert(&fifo, filepath, f);// e infine lo aggiungo alla coda fifo
        sendInteger(client, S_SUCCESS);
    }
    str_clearArray(&array, n);
}

void readFile(int client, char *filepath) {
    assert(!str_isEmpty(filepath) && filepath != NULL);

    //controllo che il file sia stato creato
    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        return;
    }
    sendInteger(client, S_SUCCESS);
    file_s *file = hash_getValue(storage, filepath);

    sendn(client, file->content, file->size);
}

void send_nfiles(char *key, void *value, bool *exit, void *args) {
    int client = (int) args;
    file_s *f = (file_s *) value;
    sendInteger(client, !EOS_F);
    sendStr(client, key);
    sendn(client, f->content, f->size);
}

void readNFile(int client, char *request) {
    if (hash_isEmpty(storage)) {
        sendInteger(client, S_STORAGE_EMPTY);
        return;
    }

    int n;
    int res = str_toInteger(&n, request);
    assert(res != -1);

    sendInteger(client, S_SUCCESS);
    hash_iteraten(storage, &send_nfiles, (void *) client, n);
    sendInteger(client, EOS_F);
}

void appendFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];
    char option = (array[2])[0];

    assert(option == 'y' || option == 'n');

    void *fcontent;
    size_t fsize;
    receivefile(client, &fcontent, &fsize);
    file_s *f = hash_getValue(storage, filepath);

    if ((f->size + fsize) > config.MAX_STORAGE_SPACE) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha inviato un file troppo grande\n\n", client);
        }

        sendInteger(client, SFILE_TOO_LARGE);
        free(fcontent);
        return;
    }

    if (!hash_containsKey(storage, filepath)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file che non esiste\n\n", client);
        }

        sendInteger(client, SFILE_NOT_FOUND);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    if (!file_isOpenedBy(f, cpid)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file non aperto\n\n", client);
        }
        sendInteger(client, SFILE_NOT_OPENED);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    //da qui in poi il file viene inserito
    if (fsize > storage_space) {  //se non ho spazio
        if (config.PRINT_LOG == 2) {
            pwarn("Rilevata Capacity Misses\n");
        }
        sendInteger(client, S_STORAGE_FULL);
        free_space(client, option, fsize);

        if (fsize >= storage_space) {
            if (config.PRINT_LOG == 1 || config.PRINT_LOG == 2) {
                perr("Non è stato possibile liberare spazio\n\n");
            }
            free(fcontent);
            str_clearArray(&array, n);
            sendInteger(client, S_FREE_ERROR);
            return;
        }

        if (config.PRINT_LOG == 2) {
            printf("Spazio liberato!\n\n");
        }

    }

    size_t newSize = f->size + fsize;
    void *newContent = malloc(newSize);
    if (newContent == NULL) {
        perr("malloc error: impossibile appendere il contenuto richiesto\n");
        sendInteger(client, MALLOC_ERROR);
        return;
    }

    memcpy(newContent, f->content, f->size);
    memcpy(newContent + f->size, fcontent, fsize);

    free(f->content);
    f->content = newContent;
    f->size = newSize;
    sendInteger(client, S_SUCCESS);

    if (config.PRINT_LOG == 1 || config.PRINT_LOG == 2) {
        psucc("Append per il Client %d eseguita\n\n", client);
    }
}

void writeFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":?");
    assert(n == 3);
    char *filepath = array[0];
    char *cpid = array[1];

    char option = (array[2])[0];
    assert(option == 'y' || option == 'n');
    size_t fsize;

    void *fcontent = NULL;
    receivefile(client, &fcontent, &fsize);

    if (fsize > config.MAX_STORAGE_SPACE) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha inviato un file troppo grande\n", client);
        }

        sendInteger(client, SFILE_TOO_LARGE);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    if (!hash_containsKey(storage, filepath)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file che non esiste\n", client);
        }

        sendInteger(client, SFILE_NOT_FOUND);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    file_s *f = hash_getValue(storage, filepath);
    if (!file_isOpenedBy(f, cpid)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file non aperto\n", client);
        }

        sendInteger(client, SFILE_NOT_OPENED);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    } else if (!file_isEmpty(f)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito una Write su un file non vuoto\n", client);
        }

        sendInteger(client, SFILE_NOT_EMPTY);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    //da qui in poi il file viene inserito
    if (fsize > storage_space) {  //se non ho spazio
        if (config.PRINT_LOG == 2) {
            pwarn("Rilevata Capacity Misses\n");
        }

        sendInteger(client, S_STORAGE_FULL);
        free_space(client, option, fsize);

        if (fsize > storage_space) {
            if (config.PRINT_LOG == 1 || config.PRINT_LOG == 2) {
                perr("Non è stato possibile liberare spazio\n");
            }

            free(fcontent);
            str_clearArray(&array, n);
            sendInteger(client, S_FREE_ERROR);
            return;
        }

        if (config.PRINT_LOG == 2) {
            printf("Spazio liberato!\n");
        }
    }

    assert(f->path != NULL && !str_isEmpty(f->path));
    assert(storage_space <= config.MAX_STORAGE_SPACE);

    file_update(&f, fcontent, fsize);
    storage_space -= fsize;

    sendInteger(client, S_SUCCESS);

    str_clearArray(&array, n);

    if (config.PRINT_LOG == 1 || config.PRINT_LOG == 2) {
        psucc("Write completata\n"
              "Capacità dello storage: %d\n\n", storage_space);
    }
}

void clear_openedFiles(char *key, void *value, bool *exit, void *cpid) {
    file_s *f = (file_s *) value;
    if (file_isOpenedBy(f, (char *) cpid)) {
        file_clientClose(&f, (char *) cpid);
    }
}

void closeConnection(int client, char *cpid) {
    int nfiles = *((int *) hash_getValue(opened_files, cpid));

    if (nfiles == 0) {
        sendInteger(client, S_SUCCESS);
    } else {
        if (config.PRINT_LOG == 2) {
            pwarn("ATTENZIONE: il Client %d non ha chiuso dei file\n"
                  "Chiusura in corso...", client);
        }

        sendInteger(client, SFILES_FOUND_ON_EXIT);
        hash_iterate(storage, &clear_openedFiles, (void *) cpid);
        if (config.PRINT_LOG == 2) {
            printf("Chiusura completata!\n");
        }
    }

    assert((*((int *) hash_getValue(opened_files, cpid))) == 0);

    hash_deleteKey(&opened_files, cpid, &free);
    if (close(client) != 0) {
        perr("ATTENZIONE: errore nella chiusura del Socket con il client %d\n", client);
    } else if (config.PRINT_LOG == 1 || config.PRINT_LOG == 2) {
        psucc("Client %d disconnesso\n\n", client);
    }

    connected_clients--;
}

void closeFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    assert(n == 2);

    char *filepath = array[0];
    char *cpid = array[1];

    if (!hash_containsKey(storage, filepath)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file che non esiste\n", client);
        }
        sendInteger(client, SFILE_NOT_FOUND);
        str_clearArray(&array, n);
        return;
    }
    file_s *f = hash_getValue(storage, filepath);

    if (!file_isOpenedBy(f, cpid)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file non aperto\n", client);
        }

        sendInteger(client, SFILE_NOT_OPENED);
        str_clearArray(&array, n);
        return;
    }

    file_clientClose(&f, cpid);

    sendInteger(client, S_SUCCESS);
    if (config.PRINT_LOG==1 || config.PRINT_LOG == 2) {
        psucc("File %s chiuso dal client %d\n\n", (strrchr(filepath,'/')+1), client);
    }

    str_clearArray(&array, n);
}

void removeFile(int client, char *request) {
    //il file non esiste
    if (!hash_containsKey(storage, request)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha eseguito un operazione su un file che non esiste\n", client);
        }

        sendInteger(client, SFILE_NOT_FOUND);
        return;
    }

    file_s *f = hash_getValue(storage, request);
    //elimino il file solo se non è aperto da qualcuno
    if (file_isOpened(f)) {
        if (config.PRINT_LOG == 2) {
            pwarn("Il client %d ha tentato di cancellare un file aperto\n", client);
        }

        sendInteger(client, SFILE_OPENED);
        return;
    }

    storage_space += f->size;
    if (storage_space > config.MAX_STORAGE_SPACE) {
        storage_space = config.MAX_STORAGE_SPACE;
    }
    hash_deleteKey(&storage, request, &file_destroy);
    max_storable_files++;
    sendInteger(client, S_SUCCESS);

    if (config.PRINT_LOG==1 || config.PRINT_LOG == 2) {
        psucc("File %s rimosso dal client %d\n\n", (strrchr(request,'/')+1), client);
    }
}

void *stop_server(void *argv) {
    sigset_t set;
    int signal_captured;
    int t = -1;

    sigemptyset(&set);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGHUP);

    if (config.PRINT_LOG == 2) {
        psucc("SIGWAIT Thread avviato\n\n");
    }

    pthread_sigmask(SIG_SETMASK, &set, NULL);

    if (sigwait(&set, &signal_captured) != 0) {
        soft_close = true;
        return NULL;
    }

    if (signal_captured == SIGINT || signal_captured == SIGQUIT) {   //SIGINT o SIGQUIT -> uscita forzata
        server_running = false;
    } else if (signal_captured == SIGHUP || signal_captured == SIGTERM) { //SIGHUP o SIGTERM -> uscita soft
        soft_close = true;
    }

    writen(pipe_fd[1], &t, sizeof(int)); //sveglio la select scrivendo nella pipe
    return argv;
}

void *worker_function(void *argv) {
    while (true) {
        int client = queue_get(&q);

        pthread_mutex_lock(&lock);

        if (!server_running || client == -1) {
            pthread_mutex_unlock(&lock);
            return argv;
        }

        char *request = receiveStr(client);

        if (!str_isEmpty(request)) {
            if (config.PRINT_LOG == 1 || config.PRINT_LOG == 2) {
                printf("Ricevuta richiesta: %s\n\n", request);
            }
            switch (request[0]) {
                case 'a': {
                    char *cmd = str_cut(request, 2, str_length(request) - 2);
                    appendFile(client, cmd);
                    free(cmd);
                    break;
                }
                case 'o': {
                    char *cmd = str_cut(request, 2, str_length(request) - 2);
                    openFile(client, cmd);
                    free(cmd);
                    break;
                }
                case 'c': {
                    char *cmd;
                    if (request[1] == 'l') {
                        cmd = str_cut(request, 3, str_length(request) - 3);
                        closeFile(client, cmd);
                        free(cmd);
                    } else {
                        cmd = str_cut(request, 2, str_length(request) - 2);
                        createFile(client, cmd);
                        free(cmd);
                    }

                    break;
                }

                case 'r': {
                    char *cmd;
                    if (request[1] == 'n') {
                        cmd = str_cut(request, 3, str_length(request) - 3);
                        readNFile(client, cmd);
                    } else if (request[1] == 'm') {
                        cmd = str_cut(request, 3, str_length(request) - 3);
                        removeFile(client, cmd);
                    } else {
                        cmd = str_cut(request, 2, str_length(request) - 2);
                        readFile(client, cmd);
                    }
                    free(cmd);

                    break;
                }

                case 'w': {
                    char *cmd = str_cut(request, 2, str_length(request) - 2);
                    writeFile(client, cmd);
                    free(cmd);
                    break;
                }

                case 'e': {
                    char *cmd = str_cut(request, 2, str_length(request) - 2);
                    closeConnection(client, cmd);
                    free(cmd);
                    break;
                }

                default: {
                    assert(1 == 0);
                }
            }
        }
        pthread_mutex_unlock(&lock);

        if (request[0] != 'e' || str_isEmpty(request)) {
            if (writen(pipe_fd[1], &client, sizeof(int)) == -1) {
                fprintf(stderr, "An error occurred on write back client to the pipe\n");
                exit(errno);
            }
        }

        free(request);
    }
}

void print_statistics() {
    pcolor(CYAN, "Operazioni effettuate\n");
    printf("1. Numero di files memorizzato nel Server: %zu\n", max_storable_files);
    printf("2. Dimensione massima raggiunta in Mbytes: %zu\n", (storage_space/1048576));
    printf("3. Numero di rimpiazzamenti della cache effettuati: %zu\n\n", fifo_counts);

    if (!hash_isEmpty(storage)) {
        pcolor(CYAN, "Lista dei file memorizzati attualmente sul Server\n");
        hash_iterate(storage, &print_files, NULL);
    } else {
        pcolor(CYAN, "Il Server è vuoto\n");
    }
    printf("\n");

    pcolor(CYAN, "Settaggi caricati:\n\n");
    settings_print(config);
    printf("\n");
}

int main(int argc, char *argv[]) {
    char *config_path = NULL;
    if (argc == 2) {
        if (str_startsWith(argv[1], "-c")) {
            char *cmd = (argv[1]) += 2;
            config_path = realpath(cmd, NULL);
            if (config_path == NULL) {
                fprintf(stderr, "File config non trovato!\n"
                                "Verranno usate le impostazioni di Default\n\n");
            }
        }
    } else if (argc > 2) {
        pwarn("ATTENZIONE: comando -c inserito non valido\n\n");
    }
    init_server(config_path);
    free(config_path);
    sorted_list *fd_list = sortedlist_create();

    int fd_sk = unix_socket(config.SOCK_PATH);
    if (socket_bind(fd_sk, config.SOCK_PATH) != 0) {
        perr("%s\n", strerror(errno));
        return errno;
    }
    pthread_t tid = 0;
    pthread_t thread_pool[config.N_THREAD_WORKERS];
    for (int i = 0; i < config.N_THREAD_WORKERS; i++) {
        pthread_create(&tid, NULL, &worker_function, NULL);
        thread_pool[i] = tid;
    }

    pthread_attr_t thattr = {0};
    pthread_attr_setdetachstate(&thattr, PTHREAD_CREATE_DETACHED);
    if (pthread_create(&tid, &thattr, &stop_server, NULL) != 0) {
        fprintf(stderr, "Errore: impossibile avviare il Server in modo sicuro\n");
        return -1;
    }

    fd_set current_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(fd_sk, &current_sockets);
    FD_SET(pipe_fd[0], &current_sockets);
    sortedlist_insert(&fd_list, fd_sk);
    sortedlist_insert(&fd_list, pipe_fd[0]);

    int sreturn;
    psucc("[Server in Ascolto]\n\n");

    while (server_running) {
        fd_set ready_sockets = current_sockets;
        struct timeval tv = {MASTER_WAKEUP_SECONDS, MASTER_WAKEUP_MS};
        if ((sreturn = select(sortedlist_getMax(fd_list) + 1, &ready_sockets, NULL, NULL, &tv)) < 0) {
            if (errno != EINTR) {
                fprintf(stderr, "Select Error: value < 0\n"
                                "Error code: %s\n\n", strerror(errno));
            }
            server_running = false;
            break;
        }

        if (soft_close && connected_clients == 0) {
            break;
        }

        if (sreturn > 0) {
            sortedlist_iterate();
            for (int i = 0; i <= sortedlist_getMax(fd_list); i++) {
                int setted_fd = sortedlist_getNext(fd_list);
                if (FD_ISSET(setted_fd, &ready_sockets)) {
                    if (setted_fd == fd_sk) {   //nuova connessione
                        int client = socket_accept(fd_sk);
                        if (client != -1) {
                            if (soft_close) {
                                if(config.PRINT_LOG==2){
                                    pwarn("Client %d rifiutato\n", client);
                                }
                                sendInteger(client, CONNECTION_REFUSED);
                                close(client);
                                break;
                            }
                            if(config.PRINT_LOG==2){
                                printf("Client %d connesso\n", client);
                            }
                            sendInteger(client, CONNECTION_ACCEPTED);

                            char *cpid = receiveStr(client);
                            int *n = malloc(sizeof(int));
                            if (n == NULL) {
                                fprintf(stderr, "Impossibile allocare per nuovo client\n");
                                return errno;
                            }
                            *n = 0;
                            hash_insert(&opened_files, cpid, n);
                            free(cpid);
                            connected_clients++;
                        }
                        FD_SET(client, &current_sockets);
                        sortedlist_insert(&fd_list, client);
                        break;

                    } else if (setted_fd == pipe_fd[0]) {
                        int old_fd_c;
                        readn(pipe_fd[0], &old_fd_c, sizeof(int));
                        FD_SET(old_fd_c, &current_sockets);
                        sortedlist_insert(&fd_list, old_fd_c);

                        break;
                    } else {
                        FD_CLR(setted_fd, &current_sockets);
                        sortedlist_remove(&fd_list, setted_fd);
                        queue_insert(&q, setted_fd);
                        break;
                    }
                }
            }
        }
    }

    queue_close(&q);

    if (soft_close) {
        server_running = false;
    }

    for (int i = 0; i < config.N_THREAD_WORKERS; i++) {
        pthread_join(thread_pool[i], NULL);
    }


    printf("\nUscita...\n");
    sortedlist_destroy(&fd_list);

    print_statistics();
    close_server();
    return 0;
}