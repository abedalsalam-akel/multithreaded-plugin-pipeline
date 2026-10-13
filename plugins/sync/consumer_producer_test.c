#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "consumer_producer.h"

#define PASS(msg)  do { printf("[PASS] %s\n", msg); } while(0)
#define FAIL(msg)  do { fprintf(stderr, "[FAIL] %s\n", msg); exit_code = 1; } while(0)

static long ms_since(struct timespec a, struct timespec b) {
    long sec  = b.tv_sec  - a.tv_sec;
    long nsec = b.tv_nsec - a.tv_nsec;
    return sec * 1000 + nsec / 1000000;
}

/* Helper: producer that measures how long put() blocked */
typedef struct {
    consumer_producer_t* q;
    const char* item;
    long blocked_ms;
    const char* err; // NULL on success
} put_args_t;

static void* timed_put(void* arg) {
    put_args_t* pa = (put_args_t*)arg;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    pa->err = consumer_producer_put(pa->q, pa->item);
    clock_gettime(CLOCK_MONOTONIC, &t1);
    pa->blocked_ms = ms_since(t0, t1);
    return NULL;
}

int main(void) {
    int exit_code = 0;

    consumer_producer_t q;
    const char* err = consumer_producer_init(&q, 2);
    if (err) {
        fprintf(stderr, "[FAIL] init: %s\n", err);
        return 1;
    }

    /* Test 1: FIFO order */
    err = consumer_producer_put(&q, "A");
    if (err) { fprintf(stderr, "[FAIL] put A: %s\n", err); return 1; }
    err = consumer_producer_put(&q, "B");
    if (err) { fprintf(stderr, "[FAIL] put B: %s\n", err); return 1; }

    char* s1 = consumer_producer_get(&q);
    char* s2 = consumer_producer_get(&q);
    if (s1 && s2 && strcmp(s1, "A")==0 && strcmp(s2, "B")==0) {
        PASS("FIFO order preserved (A then B)");
    } else {
        FAIL("FIFO order broken");
    }
    free(s1); free(s2);

    /* Test 2: put blocks when full until a get occurs */
    // Fill queue to capacity again
    consumer_producer_put(&q, "X");
    consumer_producer_put(&q, "Y");

    put_args_t pa = {.q = &q, .item = "Z", .blocked_ms = -1, .err = NULL};
    pthread_t put_thread;
    pthread_create(&put_thread, NULL, timed_put, &pa);

    // Let producer block for ~200ms
    usleep(200 * 1000);
    char* got = consumer_producer_get(&q); // frees a slot
    (void)got; free(got);

    pthread_join(put_thread, NULL);
    if (pa.err == NULL && pa.blocked_ms >= 150) {
        PASS("producer blocked on full queue and resumed after consumer get");
    } else if (pa.err != NULL) {
        FAIL("put failed unexpectedly while queue was full");
    } else {
        FAIL("put did not block long enough on full queue");
    }

    // Drain any remaining
    char* dn;
    while ((dn = consumer_producer_get(&q)) != NULL) free(dn);

    /* Test 3: finished semantics: get returns NULL once drained */
    consumer_producer_put(&q, "LAST");
    consumer_producer_signal_finished(&q);
    char* last = consumer_producer_get(&q);
    char* nul1 = consumer_producer_get(&q);
    if (last && strcmp(last, "LAST")==0 && nul1 == NULL) {
        PASS("after finished: last item delivered then NULL");
    } else {
        FAIL("finished semantics not respected for consumers");
    }
    free(last);

    // Optional: wait_finished should return 0 after drain
    if (consumer_producer_wait_finished(&q) == 0) {
        PASS("wait_finished returns after drain/finish");
    } else {
        FAIL("wait_finished did not return 0");
    }

    consumer_producer_destroy(&q);
    return exit_code;
}
