#pragma once

#include <stdint.h>

#define UTILS_MIN(x, y) (((x) < (y)) ? (x) : (y))
#define UTILS_MAX(x, y) (((x) > (y)) ? (x) : (y))

#define UTILS_CLAMP(x, lo, hi) UTILS_MIN(hi, UTILS_MAX(lo, x))

/* Use to place RODATA that should not get removed at linking stage (e.g. version string).
 * Keep in sync with linker script, KEEP(*(.rodata_keep)) should be present in .rodata
 * section for this to work.  */
// #define AT_RODATA_KEEP_SECTION(x) x __attribute__((used, section(".rodata_keep")))

/* Custom math functions to avoid linking full math library */
inline static float utils_fmodf(float x, float y)
{
	if (y == 0.0f) {
		return 0.0f;
	}

	return (int32_t)(x / y) * y;
}

inline static float utils_roundf(float x)
{
	if (x > 0.0f) {
		return (uint32_t)(x + 0.5f);
	}
	else {
		return (uint32_t)(x - 0.5f);
	}
}

inline static uint32_t utils_powu(uint32_t base, uint32_t exp)
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

/* Wraps phase value to 0-360 range */
inline static float wrap_phase_degrees(float phase)
{
	float wrapped = utils_fmodf(phase, 360.0f);
	if (wrapped < 0.0f) {
		wrapped += 360.0f;
	}

	return wrapped;
}
