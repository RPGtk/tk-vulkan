#ifndef GERANIUM_MAIN_H
#define GERANIUM_MAIN_H

#define __need_size_t
#include <stddef.h>
#include <stdint.h>

#define GERANIUM_MAJOR_VERSION 0
#define GERANIUM_MINOR_VERSION 0
#define GERANIUM_PATCH_VERSION 0
#define GERANIUM_TWEAK_VERSION 38

#define GERANIUM_CONCURRENT_FRAMES 2

bool geranium_getExtensions(char **storage);

bool geranium_create(const char *name, uint32_t version);
void geranium_destroy(void);

bool geranium_compileShaders(const char **names, size_t count);

bool geranium_render(uint32_t framebufferWidth,
                                 uint32_t framebufferHeight);

bool geranium_sync(void);

#endif // GERANIUM_MAIN_H
