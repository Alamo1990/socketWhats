#ifndef _QUEUE_H_
#define _QUEUE_H_

#include  <stdio.h>
#include  <stdlib.h>
#include  <string.h>

struct my_struct
{
        void *data;
        struct my_struct* next;
};


struct queue
{
        struct my_struct* head;
        struct my_struct* tail;
};

/* Enqueue an element */
struct queue* enqueue( struct queue*, void * data);
/* Dequeue an element */
void* dequeue( struct queue*);
/* Return 1 if the queue is empty and 0 otherwise*/
int queue_empty ( struct queue* s );
/* If it finds the data in the queue it removes it and returns it. Otherwise it returns NULL */
int queue_find_remove(struct queue* s, char* data );
/* Create an empty queue */
struct queue* queue_new(void);
/* If it finds the data in the queue it returns it. Otherwise it returns NULL */
void* queue_find(struct queue* s, char * username );

void queue_print(struct queue* );
void queue_print_element(struct my_struct* );

#endif
