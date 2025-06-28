#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void lcm_bootcount_init(void);
uint32_t lcm_get_bootcount(void);
void lcm_clear_bootcount(void);

#ifdef __cplusplus
}
#endif
