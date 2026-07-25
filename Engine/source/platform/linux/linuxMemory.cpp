//-----------------------------------------------------------------------------
// linuxMemory.cpp — Linux implementation of the raw allocation primitives
// declared in platform.h.
//
// Uses posix_memalign for aligned allocation — standard POSIX, available
// natively via glibc, no external dependency. Matches macMemory.mm's
// approach exactly (POSIX is POSIX; nothing Linux-specific is needed here).
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include <cstdlib>
#include <cstring>

void* dMemcpy(void *dst, const void *src, dsize_t size)
{
    return std::memcpy(dst, src, size);
}

void* dMemmove(void *dst, const void *src, dsize_t size)
{
    return std::memmove(dst, src, size);
}

void* dMemset(void *dst, S32 c, dsize_t size)
{
    return std::memset(dst, c, size);
}

S32 dMemcmp(const void *ptr1, const void *ptr2, dsize_t size)
{
    return std::memcmp(ptr1, ptr2, size);
}

void* dRealMalloc(dsize_t size)
{
    return std::malloc(size);
}

void dRealFree(void* p)
{
    std::free(p);
}

void* dMalloc_aligned(dsize_t in_size, S32 alignment)
{
    // posix_memalign requires alignment to be a power of two and a
    // multiple of sizeof(void*); every caller in this codebase passes a
    // power-of-two alignment (16, typically), so no rounding is needed.
    void* ptr = nullptr;
    if (posix_memalign(&ptr, static_cast<size_t>(alignment), in_size) != 0)
        return nullptr;
    return ptr;
}

void dFree_aligned(void* p)
{
    // Memory from posix_memalign is freed with plain free().
    std::free(p);
}
