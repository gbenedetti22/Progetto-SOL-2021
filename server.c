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

volatile sig_atomic_t server_running = true;

hash_table *storage;
hash_table *opened_files;
settings config = DEFAULT_SETTINGS;
queue *q;
int pipe_fd[2];

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

void close_server();

void init_server() {
    storage = hash_create(100);
    opened_files = hash_create(100);
    q = queue_create();
    settings_load(&config, "../../config.ini");
    pipe(pipe_fd);
}

void openFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filename = array[0];

    if (!hash_containsKey(storage, filename)) {
        sendInteger(client, S_FILE_NOT_FOUND);
    } else if (hash_containsKey(opened_files, request)) {
        sendInteger(client, S_FILE_ALREADY_OPENED);
    } else {
        hash_insert(&opened_files, request, NULL);
    }

    str_clearArray(&array, n);
}

void createFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filename = array[0];

    if (hash_containsKey(opened_files, request)) {
        sendInteger(client, S_FILE_ALREADY_OPENED);
    } else if (hash_containsKey(storage, filename)) {
        sendInteger(client, S_FILE_ALREADY_EXIST);
    } else {
        hash_insert(&storage, filename, NULL);    //memorizzo il file
        hash_insert(&opened_files, request, NULL);//e lo apro
        sendInteger(client, S_SUCCESS);
        printf("File creato!\n");
    }

    str_clearArray(&array, n);
}

void readFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, ":");
    char *filename = array[0];

    if (!hash_containsKey(opened_files, request)) {
        if (hash_containsKey(storage, filename)) {
            sendInteger(client, S_FILE_NOT_OPENED);
            return;
        } else {
            sendInteger(client, S_FILE_NOT_FOUND);
            return;
        }
    }

    sendInteger(client, S_SUCCESS);
    void *file = hash_getValue(storage, filename);
    sendn(client, file, str_length(file));

    str_clearArray(&array, n);
}

void readNFile(int client, char *cmd) {

}

void writeFile(int client, char *request) {
    char **array = NULL;
    int n = str_split(&array, request, "?;");
    assert(n == 3);
    char *filepid = array[0];
    void *file_content = array[1];
    char option = (array[2])[0];
    printf("%c\n", option);

    if (!hash_containsKey(opened_files, filepid)) {
        sendInteger(client, S_FILE_NOT_OPENED);
        return;
    }
    char **temp = NULL;
    int n1 = str_split(&temp, filepid, ":");
    char *filepath = temp[0];

    if (hash_containsKey(storage, filepath)) {
        void *value = hash_getValue(storage, filepath);
        if (value == NULL || str_isEmpty(value)) {
            hash_updateValue(&storage, filepath, file_content);
            sendInteger(client, S_SUCCESS);
        } else {
            sendInteger(client, S_FILE_NOT_EMPTY);
        }
    } else {
        sendInteger(client, S_FILE_NOT_FOUND);
    }
    str_clearArray(&array, n);
    str_clearArray(&temp, n1);
}

void *worker_function(void *argv) {
    while (true) {
        pthread_mutex_lock(&lock);
        pthread_cond_wait(&cond, &lock);
        if (!server_running) {
            break;
        }

        int client = queue_get(&q);

        char *request = (char *) receiven(client);
        switch (request[0]) {
            case 'o': {
                char *cmd = str_cut(request, 2, str_length(request) - 2);
                openFile(client, cmd);
                free(cmd);
                break;
            }
            case 'c': {
                printf("REQUEST: %s\n", request);
                char *cmd = str_cut(request, 2, str_length(request) - 2);
                createFile(client, cmd);
                free(cmd);

                hash_print(storage);
                break;
            }

            case 'r': {
                char *cmd;
                if (request[1] == 'n') {
                    cmd = str_cut(request, 3, str_length(request) - 3);
                    readNFile(client, cmd);
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

                printf("\ndopo write:\n");
                hash_print(storage);
            }
        }

        free(request);
        if (writen(pipe_fd[1], &client, sizeof(int)) == -1) {
            fprintf(stderr, "An error occurred on write back the pipe\n");
            exit(errno);
        }
        pthread_mutex_unlock(&lock);
    }
    return argv;
}

//int updatemax(fd_set set, int fdmax) {
//    for(int i=(fdmax-1);i>=0;--i)
//        if (FD_ISSET(i, &set)) return i;
//    assert(1==0);
//}

int main() {
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

//    int max_socket;
//    if (fd_sk > pipe_fd[0]) {
//        max_socket =fd_sk;
//    } else {
//        max_socket = pipe_fd[0];
//    }


    while (server_running) {
        fd_set ready_sockets = current_sockets;
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
                    FD_SET(client, &current_sockets);
                    sortedlist_insert(&fd_list, client);
//                    if (client->fd_sk > max_socket)
//                        max_socket = client->fd_sk;
                    break;

                } else if (setted_fd == pipe_fd[0]) {
                    int old_fd_c;
                    readn(pipe_fd[0], &old_fd_c, sizeof(int));
                    FD_SET(old_fd_c, &current_sockets);
                    sortedlist_insert(&fd_list, old_fd_c);

//                    if (old_fd_c > max_socket)
//                        max_socket = old_fd_c;
                    break;
                } else {
                    FD_CLR(setted_fd, &current_sockets);
                    sortedlist_remove(&fd_list, setted_fd);
                    queue_insert(&q, setted_fd);
//                    if (i == max_socket)
//                        max_socket = updatemax(current_sockets, max_socket);

                    pthread_cond_signal(&cond);
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
