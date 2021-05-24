#include <stdio.h>
#include <stdlib.h>
#include "../my_string.h"
#include "../list.h"

list *list_create() {
    list* l = malloc(sizeof(list));
    l->head = NULL;
    l->tail = NULL;
    l->length = 0;

    return l;
}

void insert_head (node **head, char* key, void *value) {
    node *element = (node *) malloc(sizeof(node));
    element->key=key;
    element->value = value;
    element->next = *head;

    *head = element;
}

void insert_tail (node **tail, char* key, void *value) {
    node *element = (node *) malloc(sizeof(node));
    element->key=key;
    element->value = value;
    element->next = NULL;

    (*tail)->next = element;
    *tail = element;
}

bool list_remove(list **l, char* key)
{
    node** head=&(*l)->head;

    node *curr = *head;
    node *succ = (*head)->next;

    if (str_equals(curr->key, key))
    {
        *head = curr->next;
        free(curr->key);
        free(curr->value);
        free(curr);
        (*l)->length--;
        return true;
    }

    if(succ == NULL){
        return false;
    }

    while (!str_equals(succ->key, key) && curr != NULL)
    {
        curr = curr->next;
        succ = succ->next;
        if(succ == NULL)
            break;
    }

    if(curr != NULL && succ == NULL) {
        if(str_equals(curr->key, key)) {
            free(curr->key);
            free(curr->value);
            free(curr);
            (*l)->length--;
            return true;
        }
    }else if(curr != NULL){
        curr->next = succ->next;
        free(curr->key);
        free(curr->value);
        free(succ);
        (*l)->length--;
        return true;
    }

    return false;
}

bool list_isEmpty(list* l){
    return l->length==0;
}

void list_print(list *l) {
    node *temp = l->head;
    while (temp != NULL) {
        printf("key: %s | value: %s\n", temp->key,temp->value);
        temp = temp->next;

    }
    printf("\n");
}

void list_destroy(list** l){
    while((*l)->head != NULL){
        node* curr=(*l)->head;
        free(curr->key);
        free(curr->value);
        (*l)->head=curr->next;
        free(curr);
    }

    free((*l));
}

void list_insert(list **l, char* key, void *value) {
    char* dup_key= str_create(key);
    void* dup_value= value;

    if ((*l)->head == NULL) {
        insert_head(&(*l)->head, dup_key, dup_value);
        (*l)->tail = (*l)->head;
    } else {
        insert_tail(&(*l)->tail, dup_key, dup_value);
    }
    (*l)->length++;
}

node* list_getNode(list* l, char* key){
    node* head = l->head;
    node* tail=l->tail;
    if(head==NULL){
        return NULL;
    }

    if(str_equals(head->key,key)){
        return head;
    }else if(str_equals(tail->key,key)){
        return tail;
    }

    while (head != NULL && !str_equals(head->key,key)) {
        head = head->next;
    }

    if(head != NULL){
        return head;
    }

    return NULL;
}

int list_getlength(list* l){
    return l->length;
}

bool list_conteinsKey(list* l, char* key){
    return list_getNode(l,key)!=NULL;
}