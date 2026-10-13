#ifdef _MSC_VER
    #define strdup _strdup
#endif

#define _POSIX_C_SOURCE 200809L
#define _GNU_SOURCE
#include "plugin_sdk.h"
#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * Flipper Plugin - Reverses the order of characters
 */

static const char* flipper_transform(const char* input) {
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

    // Reverse the string
    for (size_t i = 0; i < len; i++) {
        result[i] = input[len - 1 - i];
    }

    result[len] = '\0';
    printf("%s\n", result);
    return result;
}

// Plugin interface implementation
const char* plugin_get_name(void) {
    return "flipper";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(flipper_transform, "flipper", queue_size);
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
