#ifndef TKVUL_MAIN_H
#define TKVUL_MAIN_H

#include <stdint.h>

bool tkvul_initialize(const char *name, uint32_t version,
                      uint32_t framebufferWidth, uint32_t framebufferHeight);

#endif // TKVUL_MAIN_H
