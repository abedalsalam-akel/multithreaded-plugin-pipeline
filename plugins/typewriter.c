#ifdef _MSC_VER
    #define strdup _strdup
#endif

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include "plugin_sdk.h"
#include "plugin_common.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/**
 * Typewriter Plugin - Prints each character with 1s delay using usleep()
 */

static const char* typewriter_transform(const char* input) {
    if (!input) {
        return NULL;
    }

    printf("[typewriter] ");

    fflush(stdout);
    const char* p = input;

    while (*p) {
        putchar(*p++);
        fflush(stdout);
        usleep(1000000);
    }

    putchar('\n');
    fflush(stdout);

    return strdup(input);
}

// Plugin interface implementation
const char* plugin_get_name(void) {
    return "typewriter";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(typewriter_transform, "typewriter", queue_size);
}

const char* plugin_fini(void) {
    return common_plugin_fini();
}

const char* plugin_place_work(const char* str) {
    return common_plugin_place_work(str);
}

void plugin_attach(const char* (*next_place_work)(const char*)) {
    common_plugin_attach(next_place_work);
}

const char* plugin_wait_finished(void) {
    return common_plugin_wait_finished();
}
