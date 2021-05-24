#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include "../hash_table.h"

int hash(char *str, int t_size) {
    int hash, i;
    int len = str_length(str);
    for (hash = i = 0; i < len; i++) {
        hash += str[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);

    hash %= t_size;
    if (hash < 0)
        hash += t_size;
    return hash;
}


bool containsKey(hash_table table, char *key, int index) {
    if (index < 0) {
        index = hash(key, table.max_size);
    }
    if (table.buckets[index]==NULL){
        return false;
    }

    if (list_getNode(table.buckets[index], key) != NULL) {
        return true;
    }
    return false;
}

bool hash_containsKey(hash_table* table, char *key) {
    return containsKey((*table), key, -1);
}

int hash_insert(hash_table** table, char *key, void *value) {
    if (table == NULL || key == NULL)
        return HASH_NULL_PARAM;

    pthread_mutex_lock(&(*table)->lock);

    int index = hash(key, (*table)->max_size);

    if ((*table)->buckets[index] == NULL) {
        (*table)->buckets[index] = list_create();
    } else {
        if (containsKey(*(*table), key, index)) {
            pthread_mutex_unlock(&(*table)->lock);
            return HASH_DUPLICATE_KEY;
        }

        (*table)->collisions++;
    }

    list_insert(&(*table)->buckets[index], key, value);
    (*table)->lenght++;

    pthread_mutex_unlock(&(*table)->lock);

    return HASH_INSERT_SUCCESS;
}

int hash_updateValue(hash_table** table, char* key, void* newValue){
    pthread_mutex_lock(&(*table)->lock);
    int index = hash(key, (*table)->max_size);
    if ((*table)->buckets[index] == NULL) {
        pthread_mutex_unlock(&(*table)->lock);
        return -1;
    } else {
        node* n=list_getNode((*table)->buckets[index],key);
        if(n==NULL) {
            pthread_mutex_unlock(&(*table)->lock);
            return -1;
        }
        free(n->value);
        n->value = newValue;
    }

    pthread_mutex_unlock(&(*table)->lock);
    return 0;
}

void *hash_getValue(hash_table* table, char *key) {
    int index = hash(key, table->max_size);
    if (table->buckets[index] == NULL) {
        return NULL;
    } else {
        return list_getNode(table->buckets[index], key)->value;
    }
}

hash_table* hash_create(int size) {
    list **buckets = calloc(size, sizeof(list *));
    if (buckets == NULL) {
        fprintf(stderr, "Errore nella creazione della tabella: out of memory");
        exit(-1);
    }

    hash_table* table= malloc(sizeof(hash_table));
    table->max_size = size;
    table->buckets = buckets;
    table->collisions = 0;
    table->lenght = 0;
    if(pthread_mutex_init(&table->lock,NULL) != 0){
        fprintf(stderr,"Impossibile rendere la tabella atomica\n");
    }

    return table;
}

void hash_destroy(hash_table** table) {
    for (int i = 0; i < (*table)->max_size; ++i) {
        if ((*table)->buckets[i] != NULL)
            list_destroy(&(*table)->buckets[i]);
    }

    free((*table)->buckets);
    free(*table);
}

void hash_iterate(hash_table* table, void (*f)(char *, void *, bool*, void*), void* args) {
    bool exit=false;
    for (int i = 0; i < table->max_size; i++) {
        if (table->buckets[i] != NULL) {
            node *head = table->buckets[i]->head;
            while (head != NULL && !exit) {
                f(head->key, head->value, &exit, args);
                head = head->next;
            }

            if(exit)
                break;
        }
    }
}

bool hash_isEmpty(hash_table* table){
    return table->lenght==0;
}

void hash_iteraten(hash_table* table, void (*f)(char *, void *, bool*, void*), void* args , int n) {
    if(n>table->max_size || n < 0){
        hash_iterate(table,f, args);
        return;
    }

    int i=0;
    bool exit=false;
    while(n>0) {
        if (table->buckets[i] != NULL) {
            node *head = table->buckets[i]->head;
            while (head != NULL && n>0 && !exit) {
                f(head->key, head->value,&exit,args);
                head = head->next;
                n--;
            }
        }
        i++;
        if(exit)
            break;
    }
}

int hash_deleteKey(hash_table** table, char *key) {
    pthread_mutex_lock(&(*table)->lock);
    int index = hash(key, (*table)->max_size);

    if ((*table)->buckets[index] == NULL) {
        pthread_mutex_unlock(&(*table)->lock);
        return HASH_KEY_NOT_FOUND;
    } else {
        bool deleted = list_remove(&(*table)->buckets[index], key);
        if(list_isEmpty((*table)->buckets[index])){
            free((*table)->buckets[index]);
            (*table)->buckets[index]=NULL;
        }
        if (!deleted) {
            pthread_mutex_unlock(&(*table)->lock);
            return HASH_KEY_NOT_FOUND;
        }
    }
    (*table)->lenght--;
    pthread_mutex_unlock(&(*table)->lock);

    return 0;
}