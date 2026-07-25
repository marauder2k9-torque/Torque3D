//-----------------------------------------------------------------------------
// win32Memory.cpp — Windows implementation of the raw allocation primitives
// declared in platform.h.
//
// Fresh C++17 rewrite. Uses _aligned_malloc/_aligned_free (the MSVC CRT's
// own aligned-allocation API) instead of the old x86-only _mm_malloc/
// _mm_free (xmmintrin.h SSE intrinsic), so this works unmodified on both
// x86_64 and Arm64 Windows.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include <cstdlib>
#include <cstring>
#include <malloc.h>

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
    return _aligned_malloc(in_size, static_cast<size_t>(alignment));
}

void dFree_aligned(void* p)
{
    _aligned_free(p);
}
