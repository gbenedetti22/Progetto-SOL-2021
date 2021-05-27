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

#define PRINT_TABLE(x, flags) hash_iterate(x, &print,(void*)flags)

volatile sig_atomic_t server_running = true;

hash_table *storage;            //contiene tutti i file associati al loro path assoluto
hash_table *opened_files;       //memorizza quanti file un determinato client ha aperto
settings config = DEFAULT_SETTINGS;
queue *q;
list *fifo;
size_t storage_space;
size_t max_storable_files;
int pipe_fd[2];

typedef struct {
    char *path;
    void *content;
    size_t size;
    list *pidlist;
} file_s;

struct args {
    int client;
    size_t fsize;
    char option;
};

static file_s *file_init(char *path) {
    if (str_isEmpty(path))
        return NULL;

    file_s *file = malloc(sizeof(file_s));
    file->path = str_create(path);
    file->content = 0;
    file->size = 0;
    file->pidlist = list_create();

    return file;
}

void file_update(file_s **f, char *newContent, size_t newSize) {
    (*f)->content = newContent;
    (*f)->size = newSize;
}

bool file_removeClient(file_s **f, char *cpid) {
    return list_remove(&(*f)->pidlist, cpid);
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
    (*(int *) hash_getValue(opened_files, cpid))++;
}

void file_destroy(file_s **f, char *cpid) {
    file_removeClient(f, cpid);
    (*(int *) hash_getValue(opened_files, cpid))--;
}

void file_remove(file_s **f) {
    free((*f)->content);
    free((*f)->path);
    list_destroy(&(*f)->pidlist);
//    free((*f));
}

void print(char *key, void *value, bool *exit, void *argv) {
    int n = (int) argv;
    if (n == 0) {
        printf("key: %s | value: %s\n", key, ((file_s *) value)->path);
    } else if (n == 1) {
        printf("key: %s | value: NULL\n", key);
    }
}

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void close_server();

void init_server() {
    settings_load(&config, "../../config.ini");
    storage = hash_create(config.MAX_STORABLE_FILES);
    opened_files = hash_create(config.MAX_STORABLE_FILES);
    q = queue_create();
    max_storable_files = config.MAX_STORABLE_FILES;
    storage_space = config.MAX_STORAGE_SPACE;
    fifo = list_create();
    pipe(pipe_fd);
}

void openFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];

    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        str_clearArray(&array, n);
        return;
    }

    file_s *f = hash_getValue(storage, filepath);
    if (file_isOpenedBy(f, cpid)) {
        sendInteger(client, SFILE_ALREADY_OPENED);
        str_clearArray(&array, n);
        return;
    }

    file_open(&f, cpid);
    sendInteger(client, S_SUCCESS);

    str_clearArray(&array, n);
}

void createFile(int client, char *request) {
    if (max_storable_files == 0) {
        sendInteger(client, S_STORAGE_FULL);
        return;
    }

    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];
    assert(!str_isEmpty(filepath) && filepath != NULL);

    if (hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_ALREADY_EXIST);
    } else {
        file_s *f = file_init(filepath);//creo un nuovo file
        hash_insert(&storage, filepath, f);//lo memorizzo
        file_open(&f, cpid);//lo apro
        max_storable_files--;//aggiorno il numero massimo di file memorizzabili
        list_insert(&fifo, filepath, NULL);// e infine lo aggiungo alla coda fifo
        sendInteger(client, S_SUCCESS);
    }
    str_clearArray(&array, n);
}

void searchFile() {
    //coming soon...
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

void free_space(char *key, void *value, bool *exit, void *arg1) {
    struct args a = *(struct args *) arg1;
    if (a.fsize < storage_space && max_storable_files > 0) {
        sendInteger(a.client, EOS_F);
        *exit = true;
        return;
    }

    file_s *f = (file_s *) value;
    if (a.option == 'y') {
        sendStr(a.client, key);
        sendn(a.client, f->content, f->size);
    }

    if (!file_isOpened(f)) {
        storage_space += f->size;
        free(f->content);
        list_destroy(&f->pidlist);
        hash_deleteKey(&storage, key);
        max_storable_files++;
    }
}

void appendFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filepath = array[0];
    char *cpid = array[1];
    char option = (array[2])[0];

    void *fcontent;
    size_t fsize;
    receivefile(client, &fcontent, &fsize);
    if (fsize > config.MAX_STORAGE_SPACE) {
        sendInteger(client, SFILE_TOO_LARGE);
        free(fcontent);
        return;
    }

    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    file_s *f = hash_getValue(storage, filepath);
    if (!file_isOpenedBy(f, cpid)) {
        sendInteger(client, SFILE_NOT_OPENED);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    //da qui in poi il file viene inserito
    if (fsize >= storage_space || max_storable_files == 0) {  //se non ho spazio
        struct args *a = malloc(sizeof(struct args));
        a->client = client;
        a->option = option;
        a->fsize = fsize;
        hash_iterate(storage, free_space, (void *) a);
        free(a);
    }


    size_t newSize = f->size + fsize;
    void *newContent = malloc(newSize);

    memcpy(newContent, f->content, f->size);
    memcpy(newContent + f->size, fcontent, fsize);

    free(f->content);
    f->content = newContent;
    f->size = newSize;
    sendInteger(client, S_SUCCESS);
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
        sendInteger(client, SFILE_TOO_LARGE);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    file_s *f = hash_getValue(storage, filepath);
    if (!file_isOpenedBy(f, cpid)) {
        sendInteger(client, SFILE_NOT_OPENED);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    } else if (!file_isEmpty(f)) {
        sendInteger(client, SFILE_NOT_EMPTY);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    //da qui in poi il file viene inserito
    if (fsize >= storage_space || max_storable_files == 0) {  //se non ho spazio
        struct args *a = malloc(sizeof(struct args));
        a->client = client;
        a->option = option;
        a->fsize = fsize;
        hash_iterate(storage, free_space, (void *) a);
        free(a);

        if (fsize >= storage_space || max_storable_files == 0) {
            free(fcontent);
            str_clearArray(&array, n);
            sendInteger(client, S_FREE_ERROR);
            return;
        }

        sendInteger(client, S_STORAGE_FULL);
    }
    if (fsize == 0) {   //se il file è vuoto
        sendInteger(client, S_SUCCESS);
        free(fcontent);
        str_clearArray(&array, n);
        return;
    }

    assert(f->path != NULL && !str_isEmpty(f->path));
    assert(storage_space <= config.MAX_STORAGE_SPACE);

    file_update(&f, fcontent, fsize);
    storage_space -= fsize;

    if (option == 'n')
        sendInteger(client, S_SUCCESS);

    str_clearArray(&array, n);
}

void clear_openedFiles(char *key, void *value, bool *exit, void *cpid) {
    file_s *f = (file_s *) value;
    if (file_isOpenedBy(f, (char *) cpid)) {
        file_removeClient(&f, (char *) cpid);
    }
}

void closeConnection(int client, char *cpid) {
    int nfiles = *((int *) hash_getValue(opened_files, cpid));

    if (nfiles == 0) {
        sendInteger(client, S_SUCCESS);
        hash_deleteKey(&opened_files, cpid);
    } else {
        sendInteger(client, SFILES_FOUND_ON_EXIT);
        hash_iterate(storage, &clear_openedFiles, (void *) cpid);
    }

    if (close(client) != 0) {
        fprintf(stderr, "ATTENZIONE: errore nella chiusura del Socket con il client %d\n", client);
    }
}

void closeFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    assert(n == 2);

    char *filepath = array[0];
    char *cpid = array[1];

    if (!hash_containsKey(storage, filepath)) {
        sendInteger(client, SFILE_NOT_FOUND);
        str_clearArray(&array, n);
        return;
    }
    file_s *f = hash_getValue(storage, filepath);

    if (!file_isOpenedBy(f, cpid)) {
        sendInteger(client, SFILE_NOT_OPENED);
        str_clearArray(&array, n);
        return;
    }

    file_destroy(&f, cpid);
    sendInteger(client, S_SUCCESS);
    str_clearArray(&array, n);
}

void removeFile(int client, char *request) {
    //il file non esiste
    if (!hash_containsKey(storage, request)) {
        sendInteger(client, SFILE_NOT_FOUND);
        return;
    }

    file_s *f = hash_getValue(storage, request);
    //elimino il file solo se non è paerto da qualcuno
    if (file_isOpened(f)) {
        sendInteger(client, SFILE_OPENED);
        return;
    }

    storage_space += f->size;
    hash_deleteKey(&storage, request);
    file_remove(&f);
    max_storable_files++;
    sendInteger(client, S_SUCCESS);


    assert(storage_space <= config.MAX_STORAGE_SPACE);
}

void *worker_function(void *argv) {
    while (true) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        if (!server_running) {
            break;
        }

        int client = queue_get(&q);

        char *request = receiveStr(client);

        if (!str_isEmpty(request)) {
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
                    break;
                }

                default: {
                    assert(1 == 0);
                }
            }
        }

        if (request[0] != 'e' || str_isEmpty(request)) {
            if (writen(pipe_fd[1], &client, sizeof(int)) == -1) {
                fprintf(stderr, "An error occurred on write back client to the pipe\n");
                exit(errno);
            }
        }
        free(request);
        pthread_mutex_unlock(&lock);
    }
    return argv;
}

int main(void) {
    init_server();
    sorted_list *fd_list = sortedlist_create();

    int fd_sk = unix_socket(config.SOCK_PATH);
    if (socket_bind(fd_sk, config.SOCK_PATH) != 0) {
        return errno;
    }
    pthread_t tid;
    pthread_t thread_pool[config.N_THREAD_WORKERS];
    for (int i = 0; i < config.N_THREAD_WORKERS; i++) {
        pthread_create(&tid, NULL, &worker_function, NULL);
        thread_pool[i] = tid;
    }
    printf("[Server in Ascolto]\n");

    fd_set current_sockets;

    FD_ZERO(&current_sockets);
    FD_SET(fd_sk, &current_sockets);
    FD_SET(pipe_fd[0], &current_sockets);
    sortedlist_insert(&fd_list, fd_sk);
    sortedlist_insert(&fd_list, pipe_fd[0]);

    while (server_running) {
        fd_set ready_sockets = current_sockets;
        assert((sortedlist_getMax(fd_list) + 1) < FD_SETSIZE);

        if (select(sortedlist_getMax(fd_list) + 1, &ready_sockets, NULL, NULL, NULL) < 0) {
            if (errno != EINTR)
                fprintf(stderr, "Error on select\n");
            break;
        }
        sortedlist_iterate();
        for (int i = 0; i <= sortedlist_getMax(fd_list); i++) {
            int setted_fd = sortedlist_getNext(fd_list);
            if (FD_ISSET(setted_fd, &ready_sockets)) {
                if (setted_fd == fd_sk) {
                    int client = socket_accept(fd_sk);
                    if (client != -1) {
                        char *cpid = receiveStr(client);

                        int *n = malloc(sizeof(int));
                        if (n == NULL) {
                            fprintf(stderr, "Impossibile allocare per nuovo client\n");
                            return errno;
                        }
                        *n = 0;
                        hash_insert(&opened_files, cpid, n);
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

                    pthread_cond_signal(&cond);
                    break;
                }
            }
        }
    }

    return 0;
}

void close_server() {
    settings_free(&config);
    hash_destroy(&storage);
    hash_destroy(&opened_files);
}