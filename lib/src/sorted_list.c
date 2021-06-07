#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <errno.h>
#include "../sorted_list.h"

sorted_list *sortedlist_create() {
    sorted_list* l = malloc(sizeof(sorted_list));
    if(l==NULL){
        fprintf(stderr, "Impossibile creare una sorted list, malloc error\n");
        exit(errno);
    }
    l->head = NULL;
    l->tail = NULL;
    l->size=0;

    return l;
}

elem* n=NULL;

static void insert_head (elem **head, int value) {
    elem *element = (elem *) malloc(sizeof(elem));
    if(element==NULL){
        fprintf(stderr, "Impossibile aggiungere client nella lista\n");
        return;
    }
    element->value = value;
    element->next = *head;

    *head = element;
}

static void insert_middle(elem **head, int value)
{
    elem *curr = *head;
    elem *succ = (*head)->next;
    elem *element = (elem *)malloc(sizeof(elem));
    if(element==NULL){
        fprintf(stderr, "Impossibile aggiungere client nella lista\n");
        return;
    }
    element->value = value;

    while (!(curr->value <= value && value <= succ->value))
    {
        succ = succ->next;
        curr = curr->next;
    }

    element->next = succ;
    curr->next = element;
}

static void insert_tail (elem **tail, int value) {
    elem *element = (elem *) malloc(sizeof(elem));
    if(element==NULL){
        fprintf(stderr, "Impossibile aggiungere client nella lista\n");
        return;
    }
    element->value = value;
    element->next = NULL;

    (*tail)->next = element;
    *tail = element;
}



bool sortedlist_remove(sorted_list **l, int value)
{
    elem** head=&(*l)->head;

    elem *curr = *head;
    elem *succ = (*head)->next;

    if (curr->value == value)
    {
        *head = curr->next;
        free(curr);
        (*l)->size--;
        return true;
    }

    if(succ == NULL){
        return false;
    }

    while (succ->value != value && curr != NULL)
    {
        curr = curr->next;
        succ = succ->next;
        if(succ == NULL)
            break;
    }

    if(curr != NULL && succ == NULL) {
        if(curr->value == value) {
            free(curr);
            (*l)->size--;
            return true;
        }
    }else if(curr != NULL){
        curr->next = succ->next;
        if(succ->next==NULL)
            (*l)->tail=curr;
        free(succ);
        (*l)->size--;
        return true;
    }

    return false;
}

bool sortedlist_isEmpty(sorted_list* l){
    return l->size==0;
}

void sortedlist_destroy(sorted_list** l){
    while((*l)->head != NULL){
        elem* curr=(*l)->head;
        (*l)->head=curr->next;
        free(curr);
    }

    free((*l));
}

void sortedlist_insert(sorted_list **l, int value) {
    if(sortedlist_isEmpty((*l))){
        insert_head(&(*l)->head, value);
        (*l)->tail = (*l)->head;
    }else {
        if ((*l)->head->value >= value) {
            insert_head(&(*l)->head, value);
        } else if ((*l)->tail->value <= value) {
            insert_tail(&(*l)->tail, value);
        } else {
            insert_middle(&(*l)->head, value);
        }
    }
    (*l)->size++;
}

void sortedlist_iterate(){
    n=NULL;
}

int sortedlist_getNext(sorted_list* l){
    if(n==NULL)
        n=l->head;

    int value=n->value;
    n=n->next;

    return value;
}

int sortedlist_getMax(sorted_list* l){
    return l->tail->value;
}