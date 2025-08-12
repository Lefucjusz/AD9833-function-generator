#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
{
    uint32_t start_time;
    uint32_t timeout;
    bool elapsed;
} timer_t;

int timer_init(timer_t *timer, uint32_t timeout);

bool timer_has_elapsed(timer_t *timer);
int timer_reset(timer_t *timer);
