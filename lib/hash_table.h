#ifndef HASH_TABLE_HASH_TABLE_H
#define HASH_TABLE_HASH_TABLE_H
#include "list.h"
#include "my_string.h"
#include <pthread.h>
#include <stdbool.h>

typedef struct {
    unsigned int max_size;
    int collisions;
    pthread_mutex_t lock;
    int lenght;
    list** buckets;
}hash_table;

int hash_insert(hash_table** table, char* key, void* value);
void* hash_getValue(hash_table* table, char* key);
hash_table* hash_create(unsigned int size);
/* Destroy the hash table using the delete_value() function to remove the value.
 * If the caller has removed all the keys using the hash_deleteKey() function,
 * then he can pass NULL to avoid looping on all the table.*/
void hash_destroy(hash_table** table, void (*delete_value)(void* value));

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

/* Removes the mapping for the specified key from this table if present.
 * Function delete_value(value) is called to remove the value (that is passed to the function).
 * If the caller pass NULL to the parameter delete_value(),
 * then it's up to him to properly delete the value.
 * */
int hash_deleteKey(hash_table** table, char *key, void (*delete_value)(void* value));
node *hash_getNode(hash_table* table, char *key);
int hash_updateValue(hash_table** table, char* key, void* newValue, void (*delete_value)(void* value));
bool hash_isEmpty(hash_table* table);
#endif //HASH_TABLE_HASH_TABLE_H
