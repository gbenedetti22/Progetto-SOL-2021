#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
typedef struct node{
    int value;
    struct node *next;
} node;

#include "../queue.h"

queue *queue_create() {
    queue *q = malloc(sizeof(queue));
    q->head = NULL;
    q->tail = NULL;
    q->size = 0;

    return q;
}

void queue_insert_head (node **head, int value) {
    node *element = (node *) malloc(sizeof(node));
    element->value = value;
    element->next = *head;

    *head = element;
}

void queue_insert_tail (node **tail, int value) {
    node *element = (node *) malloc(sizeof(node));
    element->value = value;
    element->next = NULL;

    (*tail)->next = element;
    *tail = element;
}

int queue_get(queue **q) {
    node *curr = (*q)->head;
    if(curr == NULL){
        return -1;
    }

    node *succ = (*q)->head->next;

    if (succ == NULL) {
        int r=curr->value;
        free(curr);
        free(*q);
        *q = queue_create();
        return r;
    }

    (*q)->head = curr->next;
    int res = curr->value;
    free(curr);
    (*q)->size--;
    return res;

}

bool queue_isEmpty(queue* q){
    return q->head==NULL;
}

void queue_print(queue *q) {
    node *temp = q->head;
    while (temp != NULL) {
        printf("%d; ", temp->value);
        temp = temp->next;
    }
    printf("\n");
}

int queue_size(queue* q) {
    return q->size;
}

void queue_insert(queue **q, int value) {
    if ((*q)->head == NULL) {
        queue_insert_head(&(*q)->head, value);
        (*q)->tail = (*q)->head;
    } else {
        queue_insert_tail(&(*q)->tail, value);
    }

    (*q)->size += 1;
}

void queue_destroy(queue **q){
    while (!queue_isEmpty((*q))){
        queue_get(q);
    }

    free(*q);
}