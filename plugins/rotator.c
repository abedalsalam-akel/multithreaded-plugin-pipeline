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
 * Rotator Plugin - Moves every character one position right (last wraps to front)
 */

static const char* rotator_transform(const char* input) {
    if (!input) {
        return NULL;
    }

    size_t len = strlen(input);
    if (len == 0) {
        return strdup(input);
    }

    // Allocate memory for the result
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    // Rotate: move each character one position right
    // Last character goes to the front
    if (len == 1) {
        // Single character, no rotation needed
        result[0] = input[0];
    } else {
        // Move last character to front
        result[0] = input[len - 1];

        // Move all other characters one position right
        for (size_t i = 0; i < len - 1; i++) {
            result[i + 1] = input[i];
        }
    }

    result[len] = '\0';
    printf("%s\n", result);
    return result;
}

// Plugin interface implementation
const char* plugin_get_name(void) {
    return "rotator";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(rotator_transform, "rotator", queue_size);
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
