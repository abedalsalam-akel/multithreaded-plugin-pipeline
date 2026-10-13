#ifndef PLUGIN_COMMON_H
#define PLUGIN_COMMON_H

#include "plugin_sdk.h"
#include "sync/consumer_producer.h"
#include <pthread.h>

/**
 * All plugins can use this common infrastructure for threading and queue management
 */

/**
 * Plugin context structure - holds all state for a plugin instance
 */
typedef struct {
    const char* name;                                    // Plugin name
    consumer_producer_t* queue;                          // Internal queue for work items
    pthread_t consumer_thread;                           // Thread that processes work
    const char* (*next_place_work)(const char*);         // Next plugin in pipeline
    const char* (*process_function)(const char*);        // Plugin-specific processing function
    int initialized;                                     // Initialization flag
    int finished;                                        // Finished flag
} plugin_context_t;

void* plugin_consumer_thread(void* arg);

const char* common_plugin_init(const char* (*process_function)(const char*), const char* name, int queue_size);

const char* common_plugin_fini(void);

const char* common_plugin_place_work(const char* str);

void common_plugin_attach(const char* (*next_place_work)(const char*));

const char* common_plugin_wait_finished(void);

plugin_context_t* get_plugin_context(void);

#endif // PLUGIN_COMMON_H
