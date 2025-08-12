#pragma once

#include <stdint.h>

// #include <math.h>

#define UTILS_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define UTILS_MAX(x, y) (((x) > (y)) ? (x) : (y))

#define UTILS_CLAMP(x, lo, hi) UTILS_MIN(hi, UTILS_MAX(lo, x))

/* Use to place RODATA that should not get removed at linking stage (e.g. version string).
 * Keep in sync with linker script, KEEP(*(.rodata_keep)) should be present in .rodata
 * section for this to work.  */
// #define AT_RODATA_KEEP_SECTION(x) x __attribute__((used, section(".rodata_keep")))

/* Custom math functions to avoid linking full math library */

/* Wraps phase value to 0-360 range */
// inline static float wrap_phase_degrees(float phase)
// {
// 	float wrapped = fmodf(phase, 360.0f);
// 	if (wrapped < 0.0f) {
// 		wrapped += 360.0f;
// 	}

// 	return wrapped;
// }

inline static uint32_t utils_upow(uint32_t base, uint32_t exp)
{
	uint32_t result = 1;

	while (exp > 0) {
		if (exp & 0x00000001) {
			result *= base;
		}
		base *= base;
		exp >>= 1;
	}

	return result;
}
