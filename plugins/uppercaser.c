#include "plugin_common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/**
 * Uppercaser Plugin - Converts all alphabetic characters to uppercase
 */

static const char* uppercaser_transform(const char* input) {
    if (!input) {
        return NULL;
    }

    // Allocate memory for the result
    size_t len = strlen(input);
    char* result = malloc(len + 1);
    if (!result) {
        return NULL;
    }

    // Convert each character to uppercase
    for (size_t i = 0; i < len; i++) {
        result[i] = toupper(input[i]);
    }
    result[len] = '\0';
    printf("%s\n", result);

    return result;
}

// Plugin interface implementation
const char* plugin_get_name(void) {
    return "uppercaser";
}

const char* plugin_init(int queue_size) {
    return common_plugin_init(uppercaser_transform, "uppercaser", queue_size);
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
