/* pc_gbi_ptr.h - 64-bit pointer recovery for GBI display list commands.
 * GBI commands store addresses in 32-bit w1 fields. On 64-bit, pointers
 * are truncated. This module recovers the full address from the truncated
 * value using the executable/arena base addresses. */
#ifndef PC_GBI_PTR_H
#define PC_GBI_PTR_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(TARGET_PC) && UINTPTR_MAX > 0xFFFFFFFFu

/* Recover a full 64-bit pointer from a truncated 32-bit GBI w1 value.
 * Strategy: all game pointers come from one of two regions:
 *   1. Static data in the executable (__DATA segment) — share pc_image_base upper bits
 *   2. Arena memory (mmap'd heap) — share arena_memory upper bits
 * We check which region the low 32 bits fall into and reconstruct accordingly. */
extern uintptr_t pc_image_base;
extern uintptr_t pc_image_end;
static inline uintptr_t pc_gbi_recover_in_range(unsigned int truncated,
                                                uintptr_t range_begin,
                                                uintptr_t range_end) {
    if (range_begin == 0 || range_end <= range_begin ||
        range_end - range_begin > (uintptr_t)0x100000000ULL) {
        return (uintptr_t)truncated;
    }

    uintptr_t candidate = (range_begin & ~(uintptr_t)0xFFFFFFFFu) |
                          (uintptr_t)truncated;
    if (candidate < range_begin) {
        candidate += (uintptr_t)0x100000000ULL;
    }
    if (candidate >= range_begin && candidate < range_end) {
        return candidate;
    }
    return (uintptr_t)truncated;
}

extern u8* arena_memory;

static inline uintptr_t pc_gbi_recover_ptr(unsigned int truncated) {
    if (truncated == 0) return 0;

    /* Check if it's an arena pointer (game heap allocations) */
    uintptr_t arena_base = (uintptr_t)arena_memory;
    uintptr_t recovered = pc_gbi_recover_in_range(
        truncated, arena_base, arena_base + 48u * 1024u * 1024u);
    if (recovered != (uintptr_t)truncated) {
        return recovered;
    }

    /* Check if it's a static data pointer (executable image) */
    recovered = pc_gbi_recover_in_range(
        truncated, pc_image_base, pc_image_end);
    if (recovered != (uintptr_t)truncated) {
        return recovered;
    }

    /* Unknown region — return as-is (may be a segment address or small offset) */
    return (uintptr_t)truncated;
}

/* Truncate a pointer for storage in a GBI w1 field */
#define GBI_PTR_TRUNC(p) ((unsigned int)(uintptr_t)(p))

#else
/* 32-bit: no recovery needed */
#define pc_gbi_recover_ptr(t) ((uintptr_t)(t))
#define GBI_PTR_TRUNC(p) ((unsigned int)(uintptr_t)(p))
#endif

#ifdef __cplusplus
}
#endif

#endif /* PC_GBI_PTR_H */
