typedef struct node {
    char* key;
    void *value;
    struct node *next;
} node;

typedef struct list {
    node *head;
    node *tail;
} list;

#ifndef HASH_TABLE_LIST_H
#define HASH_TABLE_LIST_H
#include <stdbool.h>
list *list_create();
bool list_remove(list **l, char* key);
bool list_isEmpty(list* l);
void list_print(list *l);
node* list_getNode(list* l, char* key);
void list_insert(list **l, char* key, void *value);
void list_destroy(list** l);
#endif //HASH_TABLE_LIST_H
