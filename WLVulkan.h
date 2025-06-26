#ifndef WLVULKAN_MAIN_H
#define WLVULKAN_MAIN_H

#include <stdint.h>
#define __need_size_t
#include <stddef.h>

bool waterlily_vulkanCreate(const char *name, uint32_t version);

bool waterlily_vulkanCreateSurface(void **data);

bool waterlily_vulkanInitialize(uint32_t framebufferWidth,
                                uint32_t framebufferHeight);

bool waterlily_vulkanCompileShaders(const char **names, size_t count);

bool waterlily_vulkanGetShader(const char *name, char **contents, size_t *size);

#endif // WLVULKAN_MAIN_H
