#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include "monitor.h"

#define PASS(msg)  do { printf("[PASS] %s\n", msg); } while(0)
#define FAIL(msg)  do { fprintf(stderr, "[FAIL] %s\n", msg); exit_code = 1; } while(0)

static long ms_since(struct timespec a, struct timespec b) {
    long sec  = b.tv_sec  - a.tv_sec;
    long nsec = b.tv_nsec - a.tv_nsec;
    return sec * 1000 + nsec / 1000000;
}

/* --- Test 1: remembered signal (signal before wait) --- */
typedef struct { monitor_t *m; long waited_ms; } wait_args_t;

static void* delayed_waiter(void* arg) {
    wait_args_t* wa = (wait_args_t*)arg;
    // simulate doing work before calling wait
    usleep(100 * 1000); // 100ms
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    if (monitor_wait(wa->m) != 0) {
        wa->waited_ms = -1;
        return NULL;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    wa->waited_ms = ms_since(t0, t1);
    return NULL;
}

/* --- Test 2: blocking wait (wait then signal) --- */
static void* blocking_waiter(void* arg) {
    wait_args_t* wa = (wait_args_t*)arg;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    if (monitor_wait(wa->m) != 0) {
        wa->waited_ms = -1;
        return NULL;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    wa->waited_ms = ms_since(t0, t1);
    return NULL;
}

/* --- Test 3: reset requires another signal --- */
static void* reset_then_wait(void* arg) {
    wait_args_t* wa = (wait_args_t*)arg;
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    if (monitor_wait(wa->m) != 0) {
        wa->waited_ms = -1;
        return NULL;
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    wa->waited_ms = ms_since(t0, t1);
    return NULL;
}

int main(void) {
    int exit_code = 0;
    monitor_t m;
    if (monitor_init(&m) != 0) {
        fprintf(stderr, "[FAIL] monitor_init failed\n");
        return 1;
    }

    /* Test 1: remembered signal */
    monitor_signal(&m); // signal before any waiter starts
    wait_args_t wa1 = {.m = &m, .waited_ms = -1};
    pthread_t th1;
    pthread_create(&th1, NULL, delayed_waiter, &wa1);
    pthread_join(th1, NULL);
    if (wa1.waited_ms >= 0 && wa1.waited_ms <= 10) {
        PASS("monitor remembers signal sent before wait()");
    } else {
        FAIL("monitor did not return immediately on remembered signal");
    }

    /* Test 2: blocking wait then signal */
    monitor_reset(&m); // ensure it's not signaled
    wait_args_t wa2 = {.m = &m, .waited_ms = -1};
    pthread_t th2;
    pthread_create(&th2, NULL, blocking_waiter, &wa2);
    // let thread block for ~200ms
    usleep(200 * 1000);
    monitor_signal(&m);
    pthread_join(th2, NULL);
    if (wa2.waited_ms >= 150) {
        PASS("monitor_wait blocks until signal arrives");
    } else {
        FAIL("monitor_wait did not block long enough");
    }

    /* Test 3: reset clears signal and requires a new signal */
    monitor_signal(&m);  // set signaled
    // Immediately reset to clear signaled state
    monitor_reset(&m);
    wait_args_t wa3 = {.m = &m, .waited_ms = -1};
    pthread_t th3;
    pthread_create(&th3, NULL, reset_then_wait, &wa3);
    // give it time to block
    usleep(150 * 1000);
    if (wa3.waited_ms == -1) {
        PASS("after reset, waiter is still blocked (no spurious wake)");
    } else {
        FAIL("wait returned despite reset and no signal");
    }
    // now actually signal to release it
    monitor_signal(&m);
    pthread_join(th3, NULL);
    if (wa3.waited_ms >= 0) {
        PASS("wait returns after signal following reset");
    } else {
        FAIL("wait did not return after final signal");
    }

    monitor_destroy(&m);
    return exit_code;
}
