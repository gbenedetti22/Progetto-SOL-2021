#define HASH_NULL_PARAM -1
#define HASH_INSERT_SUCCESS 0
#define HASH_DUPLICATE_KEY 2
#define HASH_KEY_NOT_FOUND 4

#ifndef HASH_TABLE_HASH_TABLE_H
#define HASH_TABLE_HASH_TABLE_H
#include "list.h"
#include <pthread.h>

typedef struct {
    int max_size;
    int collisions;
    pthread_mutex_t lock;
    list** buckets;
}hash_table;

int hash_insert(hash_table** table, char* key, void* value);
void* hash_getValue(hash_table* table, char* key);
hash_table* hash_create(int size);
void hash_destroy(hash_table** table);
void hash_iterate(hash_table* table, void (*f)(char *, char *, bool*));
void hash_iteraten(hash_table* table, void (*f)(char*,char*), int n);
bool hash_containsKey(hash_table* table, char* key);
int hash_deleteKey(hash_table** table, char *key);
node *hash_getNode(hash_table* table, char *key);
int hash_updateValue(hash_table** table, char* key, void* newValue);
void hash_print(hash_table* table);
#endif //HASH_TABLE_HASH_TABLE_H
