#ifndef TKVULKAN_MAIN_H
#define TKVULKAN_MAIN_H

#include <stdint.h>
#define __need_size_t
#include <stddef.h>

bool tkvul_connect(const char *name, uint32_t version);

bool tkvul_connectSurface(void **data);

bool tkvul_initialize(uint32_t framebufferWidth, uint32_t framebufferHeight);

bool tkvul_compileShaders(const char **names, size_t count);

#endif // TKVULKAN_MAIN_H
