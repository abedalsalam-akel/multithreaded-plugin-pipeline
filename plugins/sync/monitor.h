#ifndef MONITOR_H
#define MONITOR_H

#include <pthread.h>

/**
 * Monitor - A "stateful condition variable" to solve race conditions
 * Remembers if a signal was sent even if no thread was waiting
 */
typedef struct {
    pthread_mutex_t mutex;
    pthread_cond_t condition;
    int signaled;  // Remembers if signal was sent
} monitor_t;

/**
 * Initialize a monitor
 * @param monitor Pointer to monitor to initialize
 * @return 0 on success, non-zero on error
 */
int monitor_init(monitor_t* monitor);

/**
 * Destroy a monitor and free its resources
 * @param monitor Pointer to monitor to destroy
 */
void monitor_destroy(monitor_t* monitor);

/**
 * Signal the monitor (wake up waiting threads)
 * @param monitor Pointer to monitor to signal
 */
void monitor_signal(monitor_t* monitor);

/**
 * Reset the monitor's signaled state
 * @param monitor Pointer to monitor to reset
 */
void monitor_reset(monitor_t* monitor);

/**
 * Wait for the monitor to be signaled
 * @param monitor Pointer to monitor to wait on
 * @return 0 if signaled, non-zero on error
 */
int monitor_wait(monitor_t* monitor);

#endif // MONITOR_H