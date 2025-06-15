#ifndef TKVUL_MAIN_H
#define TKVUL_MAIN_H

#include <stdint.h>

bool tkvul_connect(const char *name, uint32_t version);

bool tkvul_connectSurface(void **data);

bool tkvul_initialize(uint32_t framebufferWidth, uint32_t framebufferHeight);

#endif // TKVUL_MAIN_H
