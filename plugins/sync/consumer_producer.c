#define _POSIX_C_SOURCE 200809L
#include "consumer_producer.h"
#include <stdlib.h>
#include <string.h>

const char* consumer_producer_init(consumer_producer_t* q, int capacity) {
    if (!q) return "Queue pointer is NULL";
    if (capacity <= 0) return "Capacity must be positive";

    q->items    = (char**)calloc(capacity, sizeof(char*));
    if (!q->items) return "Failed to allocate memory for queue items";
    q->capacity = capacity;
    q->count    = 0;
    q->head     = 0;
    q->tail     = 0;
    q->finished = 0;

    if (pthread_mutex_init(&q->lock, NULL) != 0) return "Failed to init mutex";
    if (pthread_cond_init(&q->not_full, NULL) != 0)  return "Failed to init not_full";
    if (pthread_cond_init(&q->not_empty, NULL) != 0) return "Failed to init not_empty";
    return NULL;
}

void consumer_producer_destroy(consumer_producer_t* q) {
    if (!q) return;
    if (q->items) {
        // free remaining items
        for (int i = 0; i < q->count; i++) {
            int idx = (q->head + i) % q->capacity;
            free(q->items[idx]);
        }
        free(q->items);
    }
    pthread_cond_destroy(&q->not_full);
    pthread_cond_destroy(&q->not_empty);
    pthread_mutex_destroy(&q->lock);
}

const char* consumer_producer_put(consumer_producer_t* q, const char* item) {
    if (!q || !item) return "Invalid parameters";
    char* copy = strdup(item);
    if (!copy) return "Failed to allocate memory for item";

    pthread_mutex_lock(&q->lock);
    while (q->count == q->capacity && !q->finished) {
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    if (q->finished) { // no more puts accepted
        pthread_mutex_unlock(&q->lock);
        free(copy);
        return "Queue finished";
    }
    q->items[q->tail] = copy;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    pthread_cond_signal(&q->not_empty);
    pthread_mutex_unlock(&q->lock);
    return NULL;
}

char* consumer_producer_get(consumer_producer_t* q) {
    if (!q) return NULL;
    pthread_mutex_lock(&q->lock);
    while (q->count == 0 && !q->finished) {
        pthread_cond_wait(&q->not_empty, &q->lock);
    }
    if (q->count == 0 && q->finished) {
        pthread_mutex_unlock(&q->lock);
        return NULL; // finished and drained
    }
    char* item = q->items[q->head];
    q->items[q->head] = NULL;
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    pthread_cond_signal(&q->not_full);
    pthread_mutex_unlock(&q->lock);
    return item;
}

void consumer_producer_signal_finished(consumer_producer_t* q) {
    if (!q) return;
    pthread_mutex_lock(&q->lock);
    q->finished = 1;
    pthread_cond_broadcast(&q->not_empty);
    pthread_cond_broadcast(&q->not_full);
    pthread_mutex_unlock(&q->lock);
}

int consumer_producer_wait_finished(consumer_producer_t* q) {
    if (!q) return -1;
    pthread_mutex_lock(&q->lock);
    while (!(q->finished && q->count == 0)) {
        // waits are awakened by consumers signaling not_full after pops, or by signal_finished broadcasts
        pthread_cond_wait(&q->not_full, &q->lock);
    }
    pthread_mutex_unlock(&q->lock);
    return 0;
}
