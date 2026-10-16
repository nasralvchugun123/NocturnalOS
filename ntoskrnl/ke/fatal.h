
#ifndef KE_FATAL_H
#define KE_FATAL_H

#include <stdint.h>

void KeFatalException(uint32_t Code, const char *Message);

#endif