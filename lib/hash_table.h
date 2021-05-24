#define HASH_NULL_PARAM -1
#define HASH_INSERT_SUCCESS 0
#define HASH_DUPLICATE_KEY 2
#define HASH_KEY_NOT_FOUND 4

#ifndef HASH_TABLE_HASH_TABLE_H
#define HASH_TABLE_HASH_TABLE_H
#include "list.h"
#include "my_string.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct {
    int max_size;
    int collisions;
    pthread_mutex_t lock;
    int lenght;
    list** buckets;
}hash_table;

int hash_insert(hash_table** table, char* key, void* value);
void* hash_getValue(hash_table* table, char* key);
hash_table* hash_create(int size);
void hash_destroy(hash_table** table);

/* Iterate over ALL keys and values of the table, passed to f function.
 * If exit is setted to true, then the iteration stops.
 * args is passed to the last parameter of the function.
 *
 * Example (assuming key and value of type String):
 *
 * //print keys and values unless a key equals to value is found
 * void print_table(char* key, void* value, bool* exit, void* args){
 *      printf("key: %s | value: %s\n", key, value);
 *      if(strcmp(key,value)==0){
 *          *exit=true; //stop the iteration
 *      }
 * }
 * */
void hash_iterate(hash_table* table, void (*f)(char *, void *, bool*, void*), void* args);

/* Same of hash_iterate(), but the iteration stops
 * when exit is setted to true or n is equals to 0.
 * If n is greater then hash size, then hash_iterate() is called.
 * */
void hash_iteraten(hash_table* table, void (*f)(char *, void *, bool*, void*), void* args , int n);
bool hash_containsKey(hash_table* table, char* key);

/* Delete and free the key inside the table.
 * Return 0 on success
 * */
int hash_deleteKey(hash_table** table, char *key);
//node *hash_getNode(hash_table* table, char *key);

/* Update the value of the key, with newValue.
 * Old value is free'd
 *
 * return 0 on succes, -1 if key is not found
 * */
int hash_updateValue(hash_table** table, char* key, void* newValue);
bool hash_isEmpty(hash_table* table);
#endif //HASH_TABLE_HASH_TABLE_H
