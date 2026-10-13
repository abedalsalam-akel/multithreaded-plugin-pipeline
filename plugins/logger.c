#ifdef _MSC_VER
    #define strdup _strdup
#endif

#define _POSIX_C_SOURCE 200809L
#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Logger Plugin - Logs all strings to stdout with format [logger] <string>
 */

static const char* logger_transform(const char* input) {
    if (!input) {
        return NULL;
    }

    // Print to stdout with logger format
    printf("[logger] %s\n", input);
    fflush(stdout);

    // Return a copy of the input for the next plugin
    return strdup(input);
}

const char* plugin_get_name(void) {
    return "logger";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(logger_transform, "logger", queue_size);
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
