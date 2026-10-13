#ifdef _MSC_VER
    #define strdup _strdup
#endif

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Expander Plugin - Inserts single space between each character
 */

static const char* expander_transform(const char* input) {
    if (!input) {
        return NULL;
    }

    size_t len = strlen(input);
    if (len == 0) {
        return strdup(input);
    }

    // Calculate result length: original length + (length-1) spaces
    size_t result_len = len + (len > 0 ? len - 1 : 0);

    // Allocate memory for the result
    char* result = malloc(result_len + 1);
    if (!result) {
        return NULL;
    }

    // Build the expanded string
    size_t result_idx = 0;
    for (size_t i = 0; i < len; i++) {
        result[result_idx++] = input[i];

        // Add space after each character except the last one
        if (i < len - 1) {
            result[result_idx++] = ' ';
        }
    }

    result[result_len] = '\0';
    printf("%s\n", result);
    return result;
}

// Plugin interface implementation
const char* plugin_get_name(void) {
    return "expander";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(expander_transform, "expander", queue_size);
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
