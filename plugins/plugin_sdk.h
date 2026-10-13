#ifndef PLUGIN_SDK_H
#define PLUGIN_SDK_H

/**
 * Plugin SDK - Interface that ALL plugins must implement
 * This defines the contract between the main application and plugins
 */

const char* plugin_get_name(void);

const char* plugin_init(int queue_size);

const char* plugin_fini(void);

const char* plugin_place_work(const char* str);

void plugin_attach(const char* (*next_place_work)(const char*));

const char* plugin_wait_finished(void);

typedef const char* (*plugin_get_name_func_t)(void);
typedef const char* (*plugin_init_func_t)(int queue_size);
typedef const char* (*plugin_fini_func_t)(void);
typedef const char* (*plugin_place_work_func_t)(const char* str);
typedef void (*plugin_attach_func_t)(const char* (*next_place_work)(const char*));
typedef const char* (*plugin_wait_finished_func_t)(void);

#endif // PLUGIN_SDK_H
