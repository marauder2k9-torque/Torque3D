//-----------------------------------------------------------------------------
// platformTypes.h — Canonical fixed-width types + OS/CPU/compiler detection.
//
// One OS-detection block, one CPU-detection block, one compiler-quirks
// block — each defined exactly once, rather than duplicated per compiler
// file the way the original types.h/types_gcc.h/types_visualc.h/etc. sprawl
// did.
//-----------------------------------------------------------------------------
#pragma once

#ifndef _TORQUE_TYPES_H_
#define _TORQUE_TYPES_H_

#include <cstdint>
#include <cstddef>
#include <type_traits>

//////////////////////////////////////////////////////////////////////////////
// 1) Compiler identification
//////////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
    #define TORQUE_COMPILER_VISUALC _MSC_VER
    #define TORQUE_COMPILER_STRING "VisualC++"
#elif defined(__clang__)
    #define TORQUE_COMPILER_CLANG (__clang_major__ * 10000 + __clang_minor__ * 100 + __clang_patchlevel__)
    #define TORQUE_COMPILER_STRING "Clang"
#elif defined(__GNUC__)
    #define TORQUE_COMPILER_GCC (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
    #define TORQUE_COMPILER_STRING "GCC"
#else
    #error "platformTypes.h: unrecognized compiler."
#endif

//////////////////////////////////////////////////////////////////////////////
// 2) OS identification — exactly one of these is ever defined. Order
//    matters: Android defines __linux__, and iOS/tvOS define TARGET_OS_MAC
//    via <TargetConditionals.h>, so specific checks come first.
//////////////////////////////////////////////////////////////////////////////
#if defined(__ANDROID__)
    #define TORQUE_OS_STRING "Android"
    #define TORQUE_OS_ANDROID

#elif defined(_WIN64)
    #define TORQUE_OS_STRING "Win64"
    #define TORQUE_OS_WIN
    #define TORQUE_OS_WIN64

#elif defined(_WIN32) || defined(__WIN32__)
    #define TORQUE_OS_STRING "Win32"
    #define TORQUE_OS_WIN
    #define TORQUE_OS_WIN32

#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if TARGET_OS_IOS || TARGET_OS_TV
        #define TORQUE_OS_STRING "iOS"
        #define TORQUE_OS_IOS
    #elif TARGET_OS_MAC
        #define TORQUE_OS_STRING "MacOS X"
        #define TORQUE_OS_MAC
    #else
        #error "platformTypes.h: unrecognized Apple platform."
    #endif

#elif defined(__linux__)
    #define TORQUE_OS_STRING "Linux"
    #define TORQUE_OS_LINUX

#elif defined(__FreeBSD__)
    #define TORQUE_OS_STRING "FreeBSD"
    #define TORQUE_OS_FREEBSD

#elif defined(__OpenBSD__)
    #define TORQUE_OS_STRING "OpenBSD"
    #define TORQUE_OS_OPENBSD

#else
    #error "platformTypes.h: unsupported/undetected operating system."
#endif

#if defined(TORQUE_OS_LINUX) || defined(TORQUE_OS_MAC) || defined(TORQUE_OS_IOS) || \
    defined(TORQUE_OS_ANDROID) || defined(TORQUE_OS_FREEBSD) || defined(TORQUE_OS_OPENBSD)
    #define TORQUE_OS_POSIX_FAMILY
#endif

//////////////////////////////////////////////////////////////////////////////
// 3) CPU identification
//////////////////////////////////////////////////////////////////////////////
#if defined(__x86_64__) || defined(_M_X64)
    #define TORQUE_CPU_STRING "x64"
    #define TORQUE_CPU_X64
    #define TORQUE_LITTLE_ENDIAN

#elif defined(__aarch64__) || defined(_M_ARM64) || (defined(__arm64__) && defined(__APPLE__))
    #define TORQUE_CPU_STRING "Arm64"
    #define TORQUE_CPU_ARM64
    #define TORQUE_LITTLE_ENDIAN

#elif defined(__arm__) || defined(_M_ARM)
    #define TORQUE_CPU_STRING "Arm32"
    #define TORQUE_CPU_ARM32
    #define TORQUE_LITTLE_ENDIAN

#elif defined(__i386__) || defined(_M_IX86)
    #define TORQUE_CPU_STRING "x86"
    #define TORQUE_CPU_X86
    #define TORQUE_LITTLE_ENDIAN

#else
    #error "platformTypes.h: unrecognized/unsupported target CPU."
#endif

#define TORQUE_ARCH_64BIT (defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_ARM64))

//////////////////////////////////////////////////////////////////////////////
// 4) Compiler-quirks — orthogonal to OS/CPU.
//////////////////////////////////////////////////////////////////////////////
#if defined(TORQUE_COMPILER_VISUALC)
    #include <stdlib.h>
    #define FN_CDECL __cdecl
    #define TORQUE_FORCEINLINE __forceinline
    #define TORQUE_NOINLINE __declspec(noinline)
    #define TORQUE_U16_ENDIANSWAP_BUILTIN _byteswap_ushort
    #define TORQUE_U32_ENDIANSWAP_BUILTIN _byteswap_ulong
    #define TORQUE_U64_ENDIANSWAP_BUILTIN _byteswap_uint64
    #pragma warning(disable: 4291)
    #if defined(TORQUE_CPU_X86) && !defined(__clang__)
        #define TORQUE_SUPPORTS_NASM
        #define TORQUE_SUPPORTS_VC_INLINE_X86_ASM
    #endif

#elif defined(TORQUE_COMPILER_GCC) || defined(TORQUE_COMPILER_CLANG)
    #define FN_CDECL
    #define TORQUE_FORCEINLINE __attribute__((always_inline))
    #define TORQUE_NOINLINE __attribute__((noinline))
    #define TORQUE_U16_ENDIANSWAP_BUILTIN __builtin_bswap16
    #define TORQUE_U32_ENDIANSWAP_BUILTIN __builtin_bswap32
    #define TORQUE_U64_ENDIANSWAP_BUILTIN __builtin_bswap64
    #if defined(TORQUE_CPU_X86) && defined(TORQUE_OS_LINUX)
        #define TORQUE_SUPPORTS_NASM
        #define TORQUE_SUPPORTS_GCC_INLINE_X86_ASM
    #endif
#endif

#if defined(__cplusplus) && __cplusplus >= 201703L
    #define TORQUE_CASE_FALLTHROUGH [[fallthrough]]
    #define TORQUE_UNLIKELY [[unlikely]]
#else
    #define TORQUE_CASE_FALLTHROUGH
    #define TORQUE_UNLIKELY
#endif

#if defined(TORQUE_OS_WIN)
    #define STDCALL __stdcall
#else
    #define STDCALL
#endif

#ifndef NULL
#  define NULL 0
#endif

//////////////////////////////////////////////////////////////////////////////
// 5) Fixed-width integer/float types
//////////////////////////////////////////////////////////////////////////////
typedef std::int8_t    S8;
typedef std::uint8_t   U8;
typedef std::int16_t   S16;
typedef std::uint16_t  U16;
typedef std::int32_t   S32;
typedef std::uint32_t  U32;
typedef std::int64_t   S64;
typedef std::uint64_t  U64;

typedef float  F32;
typedef double F64;

typedef std::size_t dsize_t;

#if defined(TORQUE_ARCH_64BIT)
    typedef U64 MEM_ADDRESS;
#else
    typedef U32 MEM_ADDRESS;
#endif

struct EmptyType {};

#define TORQUE_UNUSED(var) (void)sizeof(var)

//////////////////////////////////////////////////////////////////////////////
// 6) String types
//////////////////////////////////////////////////////////////////////////////
typedef char UTF8;

#if defined(TORQUE_COMPILER_VISUALC) && defined(__clang__)
    typedef wchar_t UTF16;
#else
    typedef std::uint16_t UTF16;
#endif

typedef std::uint32_t UTF32;
typedef const char* StringTableEntry;

//////////////////////////////////////////////////////////////////////////////
// 7) FileTime — genuinely platform-dependent representation.
//
// This is NOT flattened to a single typedef across all platforms: Windows
// file-time APIs (FILETIME, GetFileTime, FileTimeToLocalFileTime, etc.)
// operate on a real two-DWORD structure (100ns ticks since 1601, split
// into low/high 32-bit halves), and existing Windows platform code
// (winTime.cpp's fileToLocalTime) constructs a Win32 FILETIME directly
// from this struct's two fields. POSIX-family platforms (Linux, macOS,
// iOS, Android) only ever deal with a plain 64-bit epoch-seconds value
// (time_t-derived, from stat()'s st_mtime/st_ctime/st_atime), so a single
// S64 is both correct and sufficient there.
//////////////////////////////////////////////////////////////////////////////
#if defined(TORQUE_OS_WIN)
    struct FileTime
    {
        U32 v1; ///< low  32 bits (matches FILETIME::dwLowDateTime)
        U32 v2; ///< high 32 bits (matches FILETIME::dwHighDateTime)
    };
#else
    typedef S64 FileTime;
#endif

/// Platform-dependent file date-time value used for on-disk change history
/// (LocalTime, calendar breakdown) — distinct from FileTime above.
struct LocalTimeTag {};

//////////////////////////////////////////////////////////////////////////////
// 8) Constants
//////////////////////////////////////////////////////////////////////////////
#define __EQUAL_CONST_F F32(0.000001)

extern const F32 Float_Inf;
static const F32 Float_One  = F32(1.0);
static const F32 Float_Half = F32(0.5);
static const F32 Float_Zero = F32(0.0);
static const F32 Float_Pi   = F32(3.14159265358979323846);
static const F32 Float_2Pi  = F32(2.0 * 3.14159265358979323846);
static const F32 Float_InversePi = F32(1.0 / 3.14159265358979323846);
static const F32 Float_HalfPi = F32(0.5 * 3.14159265358979323846);
static const F32 Float_2InversePi = F32(2.0 / 3.14159265358979323846);
static const F32 Float_Inverse2Pi = F32(0.5 / 3.14159265358979323846);
static const F32 Float_Sqrt2 = F32(1.41421356237309504880f);
static const F32 Float_SqrtHalf = F32(0.7071067811865475244008443f);

static const S8  S8_MIN  = S8(-128);
static const S8  S8_MAX  = S8(127);
static const U8  U8_MAX  = U8(255);

static const S16 S16_MIN = S16(-32768);
static const S16 S16_MAX = S16(32767);
static const U16 U16_MAX = U16(65535);

static const S32 S32_MIN = S32(-2147483647 - 1);
static const S32 S32_MAX = S32(2147483647);
static const U32 U32_MAX = U32(0xffffffff);

static const F32 F32_MIN_EX = F32(-3.40282347e+38);
static const F32 F32_MIN = F32(1.175494351e-38F);
static const F32 F32_MAX = F32(3.402823466e+38F);

#include <cstddef>
#ifndef Offset
    #define Offset(x, cls) offsetof(cls, x)
    #define OffsetNonConst(x, cls) offsetof(cls, x)
#endif

//////////////////////////////////////////////////////////////////////////////
// 9) General math helpers
//////////////////////////////////////////////////////////////////////////////
inline bool isPow2(const U32 num)
{
    return (num != 0) && ((num & (num - 1)) == 0);
}

inline U32 getBinLog2(U32 value)
{
    F32 floatValue = F32(value);
    return (*reinterpret_cast<U32*>(&floatValue) >> 23) - 127;
}

inline U32 getNextBinLog2(U32 number)
{
    return getBinLog2(number) + (isPow2(number) ? 0 : 1);
}

inline U32 getNextPow2(U32 value)
{
    return isPow2(value) ? value : (1 << (getBinLog2(value) + 1));
}

#define DeclareTemplatizedMinMax(type) \
    inline type getMin(type a, type b) { return a > b ? b : a; } \
    inline type getMax(type a, type b) { return a > b ? a : b; }

DeclareTemplatizedMinMax(U32)
DeclareTemplatizedMinMax(S32)
DeclareTemplatizedMinMax(U16)
DeclareTemplatizedMinMax(S16)
DeclareTemplatizedMinMax(U8)
DeclareTemplatizedMinMax(S8)
DeclareTemplatizedMinMax(F32)
DeclareTemplatizedMinMax(F64)

// getMin/getMax — real function templates (the old DeclareTemplatizedMinMax
// macro below was misleadingly named: it expanded to a fixed set of
// same-type overloads, not an actual template.
// 
// This version is a genuine template. The return/comparison type is
// std::common_type_t<A, B>, so mixed-width or mixed-signedness arguments
// (U32 vs U64, S32 vs U32, etc.) promote the same way a plain comparison
// or arithmetic expression would, rather than requiring the caller to
// pre-cast both arguments to match.
template <typename A, typename B>
inline auto getMin(A a, B b) -> std::common_type_t<A, B>
{
   using C = std::common_type_t<A, B>;
   return (static_cast<C>(a) > static_cast<C>(b)) ? static_cast<C>(b) : static_cast<C>(a);
}

template <typename A, typename B>
inline auto getMax(A a, B b) -> std::common_type_t<A, B>
{
   using C = std::common_type_t<A, B>;
   return (static_cast<C>(a) > static_cast<C>(b)) ? static_cast<C>(a) : static_cast<C>(b);
}

//////////////////////////////////////////////////////////////////////////////
// 10) FourCC
//////////////////////////////////////////////////////////////////////////////
#if defined(TORQUE_BIG_ENDIAN)
    #define makeFourCCTag(c0,c1,c2,c3) \
        ((U32)((((U32)(U8)(c0))<<24) + (((U32)(U8)(c1))<<16) + (((U32)(U8)(c2))<<8) + ((U32)(U8)(c3))))
#elif defined(TORQUE_LITTLE_ENDIAN)
    #define makeFourCCTag(c3,c2,c1,c0) \
        ((U32)((((U32)(U8)(c0))<<24) + (((U32)(U8)(c1))<<16) + (((U32)(U8)(c2))<<8) + ((U32)(U8)(c3))))
#else
    #error "platformTypes.h: byte order not defined"
#endif

#define BIT(x) (1 << (x))

//////////////////////////////////////////////////////////////////////////////
// 11) Compile-time size/alignment contract
//////////////////////////////////////////////////////////////////////////////
static_assert(sizeof(S8)  == 1, "S8 must be 1 byte");
static_assert(sizeof(S16) == 2, "S16 must be 2 bytes");
static_assert(sizeof(S32) == 4, "S32 must be 4 bytes");
static_assert(sizeof(S64) == 8, "S64 must be 8 bytes");

static_assert(sizeof(U8)  == 1, "U8 must be 1 byte");
static_assert(sizeof(U16) == 2, "U16 must be 2 bytes");
static_assert(sizeof(U32) == 4, "U32 must be 4 bytes");
static_assert(sizeof(U64) == 8, "U64 must be 8 bytes");

static_assert(sizeof(F32) == 4, "F32 must be 4 bytes");
static_assert(sizeof(F64) == 8, "F64 must be 8 bytes");
static_assert(std::is_same<F32, float>::value,  "F32 must map to IEEE-754 float");
static_assert(std::is_same<F64, double>::value, "F64 must map to IEEE-754 double");

static_assert(sizeof(bool) == 1,
    "This platform layer assumes sizeof(bool)==1; audit any ABI-sensitive struct containing bool.");

#if defined(TORQUE_OS_WIN)
    static_assert(sizeof(FileTime) == 8, "Windows FileTime must be 8 bytes (two U32 fields, matching FILETIME).");
#else
    static_assert(sizeof(FileTime) == 8, "POSIX-family FileTime must be 8 bytes (S64).");
#endif

#if defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__)
    static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__,
        "This platform layer assumes little-endian byte order.");
#endif


#endif
