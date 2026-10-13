#ifdef _MSC_VER
    #define strdup _strdup
#endif

#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <unistd.h>
#include "plugins/plugin_sdk.h"

#define MAX_LINE_LENGTH 1024
#define MAX_PLUGINS 32

typedef struct {
    plugin_init_func_t init;
    plugin_fini_func_t fini;
    plugin_place_work_func_t place_work;
    plugin_attach_func_t attach;
    plugin_wait_finished_func_t wait_finished;
    plugin_get_name_func_t get_name;
    char* name;
    void* handle;
} plugin_handle_t;

static plugin_handle_t plugins[MAX_PLUGINS];
static int plugin_count = 0;

void print_usage(const char* program_name) {
    printf("Usage: %s <queue_size> <plugin1> <plugin2> ... <pluginN>\n", program_name);
    printf("  queue_size: positive integer\n");
    printf("  plugins   : one or more of { logger, typewriter, uppercaser, rotator, flipper, expander }\n");
}

int load_plugin(const char* plugin_name, int index) {
    char plugin_path[256];
    snprintf(plugin_path, sizeof(plugin_path), "./output/%s.so", plugin_name);

    void* handle = dlopen(plugin_path, RTLD_NOW | RTLD_LOCAL);
    if (!handle) {
        fprintf(stderr, "Error loading plugin %s: %s\n", plugin_name, dlerror());
        print_usage("analyzer");
        return -1;
    }

    dlerror();

    // Load function pointers
    plugin_handle_t* plugin = &plugins[index];
    plugin->handle = handle;
    plugin->name = strdup(plugin_name);

    plugin->init = (plugin_init_func_t)dlsym(handle, "plugin_init");
    if (!plugin->init) {
        fprintf(stderr, "Error loading plugin_init from %s: %s\n", plugin_name, dlerror());
        dlclose(handle);
        print_usage("analyzer");
        return -1;
    }

    plugin->fini = (plugin_fini_func_t)dlsym(handle, "plugin_fini");
    if (!plugin->fini) {
        fprintf(stderr, "Error loading plugin_fini from %s: %s\n", plugin_name, dlerror());
        dlclose(handle);
        print_usage("analyzer");
        return -1;
    }

    plugin->place_work = (plugin_place_work_func_t)dlsym(handle, "plugin_place_work");
    if (!plugin->place_work) {
        fprintf(stderr, "Error loading plugin_place_work from %s: %s\n", plugin_name, dlerror());
        dlclose(handle);
        print_usage("analyzer");
        return -1;
    }

    plugin->attach = (plugin_attach_func_t)dlsym(handle, "plugin_attach");
    if (!plugin->attach) {
        fprintf(stderr, "Error loading plugin_attach from %s: %s\n", plugin_name, dlerror());
        dlclose(handle);
        print_usage("analyzer");
        return -1;
    }

    plugin->wait_finished = (plugin_wait_finished_func_t)dlsym(handle, "plugin_wait_finished");
    if (!plugin->wait_finished) {
        fprintf(stderr, "Error loading plugin_wait_finished from %s: %s\n", plugin_name, dlerror());
        dlclose(handle);
        print_usage("analyzer");
        return -1;
    }

    plugin->get_name = (plugin_get_name_func_t)dlsym(handle, "plugin_get_name");
    if (!plugin->get_name) {
        fprintf(stderr, "Error loading plugin_get_name from %s: %s\n", plugin_name, dlerror());
        dlclose(handle);
        print_usage("analyzer");
        return -1;
    }

    return 0;
}

int initialize_plugins(int queue_size) {
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].init(queue_size);
        if (error) {
            fprintf(stderr, "Error initializing plugin %s: %s\n", plugins[i].name, error);
            print_usage("analyzer");
            return -1;
        }
    }
    return 0;
}

void attach_pipeline() {
    for (int i = 0; i < plugin_count - 1; i++) {
        plugins[i].attach(plugins[i + 1].place_work);
    }
    // Last plugin is not attached to anything
}

void process_input() {
    char line[MAX_LINE_LENGTH];

    while (fgets(line, sizeof(line), stdin)) {
        // Remove trailing newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // Check for end marker
        if (strcmp(line, "<END>") == 0) {
            if (plugin_count > 0) {
                const char* e = plugins[0].place_work("<END>");
                if (e) fprintf(stderr, "Error placing <END>: %s\n", e);
            }
            break;
        }

        // Send to first plugin
        if (plugin_count > 0) {
            const char* error = plugins[0].place_work(line);
            if (error) {
                fprintf(stderr, "Error placing work in first plugin: %s\n", error);
            }
        }
    }
}

void shutdown_pipeline() {
    // Wait for all plugins to finish (first to last)
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].wait_finished();
        if (error) {
            fprintf(stderr, "Error waiting for plugin %s to finish: %s\n", plugins[i].name, error);
        }
    }

    // Finalize all plugins
    for (int i = 0; i < plugin_count; i++) {
        const char* error = plugins[i].fini();
        if (error) {
            fprintf(stderr, "Error finalizing plugin %s: %s\n", plugins[i].name, error);
        }
    }

    // Close plugin handles and free memory
    for (int i = 0; i < plugin_count; i++) {
        if (plugins[i].handle) {
            dlclose(plugins[i].handle);
        }
        if (plugins[i].name) {
            free(plugins[i].name);
        }
    }

    printf("Pipeline shutdown complete\n");
}

int main(int argc, char* argv[]) {
    // Check minimum arguments
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse queue size
    int queue_size = atoi(argv[1]);
    if (queue_size <= 0) {
        fprintf(stderr, "Error: Queue size must be a positive integer\n");
        print_usage(argv[0]);
        return 1;
    }

    // Parse plugin names
    plugin_count = argc - 2;
    if (plugin_count > MAX_PLUGINS) {
        fprintf(stderr, "Error: Too many plugins (max %d)\n", MAX_PLUGINS);
        print_usage("analyzer");
        return 1;
    }

    // Load all plugins
    for (int i = 0; i < plugin_count; i++) {
        if (load_plugin(argv[i + 2], i) != 0) {
            // Cleanup already loaded plugins
            for (int j = 0; j < i; j++) {
                if (plugins[j].handle) {
                    dlclose(plugins[j].handle);
                }
                if (plugins[j].name) {
                    free(plugins[j].name);
                }
            }
            return 1;
        }
    }

    // Initialize all plugins
    if (initialize_plugins(queue_size) != 0) {
        // Cleanup
        for (int i = 0; i < plugin_count; i++) {
            if (plugins[i].handle) {
                dlclose(plugins[i].handle);
            }
            if (plugins[i].name) {
                free(plugins[i].name);
            }
        }
        return 2;
    }

    // Attach pipeline
    attach_pipeline();

    // Process input
    process_input();

    // Graceful shutdown
    shutdown_pipeline();

    return 0;
}
