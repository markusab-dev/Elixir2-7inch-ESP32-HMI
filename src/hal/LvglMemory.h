#pragma once

#include <stddef.h>
#include "esp_heap_caps.h"

/* C-compatible: LVGL sources are compiled as C, application sources as C++. */
static inline void* elixir_lvgl_malloc(size_t size) {
    void* pointer = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (pointer == NULL) {
        pointer = heap_caps_malloc(size, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return pointer;
}
static inline void elixir_lvgl_free(void* pointer) { heap_caps_free(pointer); }
static inline void* elixir_lvgl_realloc(void* pointer, size_t size) {
    void* resized = heap_caps_realloc(pointer, size,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (resized == NULL && size > 0U) {
        resized = heap_caps_realloc(pointer, size,
                                    MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    }
    return resized;
}
