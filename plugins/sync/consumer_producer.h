#ifndef CONSUMER_PRODUCER_H
#define CONSUMER_PRODUCER_H

#include <pthread.h>

typedef struct {
    char** items;
    int capacity;
    int count;
    int head;
    int tail;

    // single lock for all queue state
    pthread_mutex_t lock;
    pthread_cond_t  not_full;
    pthread_cond_t  not_empty;

    int finished;
} consumer_producer_t;

const char* consumer_producer_init(consumer_producer_t* q, int capacity);
void        consumer_producer_destroy(consumer_producer_t* q);
const char* consumer_producer_put(consumer_producer_t* q, const char* item); // strdup inside, queue owns
char*       consumer_producer_get(consumer_producer_t* q);                    // caller owns
void        consumer_producer_signal_finished(consumer_producer_t* q);
int         consumer_producer_wait_finished(consumer_producer_t* q);

#endif
