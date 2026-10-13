#include "plugin_common.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Global plugin context (one per plugin shared object)
static plugin_context_t g_plugin_context = {0};

void* plugin_consumer_thread(void* arg) {
    plugin_context_t* ctx = (plugin_context_t*)arg;

    if (!ctx || !ctx->queue || !ctx->process_function) {
        return NULL;
    }

    char* item;
    while ((item = consumer_producer_get(ctx->queue)) != NULL) {
        if (strcmp(item, "<END>") == 0) {
            if (ctx->next_place_work) ctx->next_place_work("<END>");
            free(item);
            break;
        }

        // Process the item using plugin-specific function
        const char* processed = ctx->process_function(item);

        free(item);

        if (processed) {
            if (ctx->next_place_work) {
                const char* err = ctx->next_place_work(processed);
                (void)err;
            }
            free((void*)processed);
        }
    }

    return NULL;
}

const char* common_plugin_init(const char* (*process_function)(const char*), const char* name, int queue_size) {
    if (g_plugin_context.initialized) {
        return "Plugin already initialized";
    }

    if (!process_function || !name || queue_size <= 0) {
        return "Invalid parameters";
    }

    // Initialize context
    memset(&g_plugin_context, 0, sizeof(plugin_context_t));
    g_plugin_context.name = name;
    g_plugin_context.process_function = process_function;
    g_plugin_context.next_place_work = NULL;
    g_plugin_context.finished = 0;

    // Allocate and initialize queue
    g_plugin_context.queue = malloc(sizeof(consumer_producer_t));
    if (!g_plugin_context.queue) {
        return "Failed to allocate memory for queue";
    }

    const char* queue_error = consumer_producer_init(g_plugin_context.queue, queue_size);
    if (queue_error) {
        free(g_plugin_context.queue);
        g_plugin_context.queue = NULL;
        return queue_error;
    }

    // Create consumer thread
    if (pthread_create(&g_plugin_context.consumer_thread, NULL, plugin_consumer_thread, &g_plugin_context) != 0) {
        consumer_producer_destroy(g_plugin_context.queue);
        free(g_plugin_context.queue);
        g_plugin_context.queue = NULL;
        return "Failed to create consumer thread";
    }

    g_plugin_context.initialized = 1;
    return NULL; // Success
}

const char* common_plugin_fini(void) {
    if (!g_plugin_context.initialized) {
        return "Plugin not initialized";
    }

    // Signal queue finished and wait for thread to complete
    if (g_plugin_context.queue) {
        consumer_producer_signal_finished(g_plugin_context.queue);
    }

    // Wait for consumer thread to finish
    if (pthread_join(g_plugin_context.consumer_thread, NULL) != 0) {
        return "Failed to join consumer thread";
    }

    // Clean up queue
    if (g_plugin_context.queue) {
        consumer_producer_destroy(g_plugin_context.queue);
        free(g_plugin_context.queue);
        g_plugin_context.queue = NULL;
    }

    g_plugin_context.initialized = 0;
    return NULL; // Success
}

const char* common_plugin_place_work(const char* str) {
    if (!g_plugin_context.initialized || !g_plugin_context.queue) {
        return "Plugin not initialized";
    }

    if (!str) {
        return "Invalid string parameter";
    }

    return consumer_producer_put(g_plugin_context.queue, str);
}

void common_plugin_attach(const char* (*next_place_work)(const char*)) {
    if (g_plugin_context.initialized) {
        g_plugin_context.next_place_work = next_place_work;
    }
}

const char* common_plugin_wait_finished(void) {
    if (!g_plugin_context.initialized || !g_plugin_context.queue) {
        return "Plugin not initialized";
    }

    // Signal that no more work will be added
    consumer_producer_signal_finished(g_plugin_context.queue);

    // Wait for queue to be empty
    if (consumer_producer_wait_finished(g_plugin_context.queue) != 0) {
        return "Failed to wait for queue to finish";
    }

    g_plugin_context.finished = 1;
    return NULL; // Success
}

plugin_context_t* get_plugin_context(void) {
    return &g_plugin_context;
}
