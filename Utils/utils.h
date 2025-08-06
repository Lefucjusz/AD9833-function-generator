#pragma once

#define MIN(x, y) (((x) < (y)) ? (x) : (y))
#define MAX(x, y) (((x) > (y)) ? (x) : (y))

#define CLAMP(x, lo, hi) MIN(hi, MAX(lo, x))
