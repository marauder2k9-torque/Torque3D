//-----------------------------------------------------------------------------
// macMemory.mm — macOS implementation of the raw allocation primitives
// declared in platform.h.
//
// This did not exist as its own file in the original uploads — macOS
// previously compiled POSIXMemory.cpp directly (the same "mac reuses the
// POSIX/Linux file" coupling already found and removed from MacFileSystem/
// PosixFileSystem). Per the "each platform owns its implementation, mac
// should not depend on POSIX-shared code" direction, this is a genuinely
// new, native file rather than a port.
//
// Uses posix_memalign for aligned allocation — this is standard POSIX
// (available on macOS as on Linux), not Linux-specific, so using it here
// isn't reintroducing the coupling being removed; it's just the same
// correct standard C library call macOS itself provides.
//-----------------------------------------------------------------------------
#import "platform/platform.h"
#import <cstdlib>
#import <cstring>

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
    // posix_memalign requires the alignment to be a power of two and a
    // multiple of sizeof(void*); callers in this codebase always pass
    // power-of-two alignments (16, typically), so no rounding is needed
    // here the way the size sometimes needs rounding for other aligned-
    // alloc APIs.
    void* ptr = nullptr;
    if (posix_memalign(&ptr, static_cast<size_t>(alignment), in_size) != 0)
        return nullptr;
    return ptr;
}

void dFree_aligned(void* p)
{
    // Memory from posix_memalign is freed with plain free(), unlike
    // _aligned_malloc on Windows, which requires _aligned_free.
    std::free(p);
}
