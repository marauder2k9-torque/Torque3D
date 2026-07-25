//-----------------------------------------------------------------------------
// macCPU.mm — macOS implementation of platformCPU / Processor::init().
//
// Ported from macCPU.mm. Uses sysctlbyname throughout — real, native,
// dependency-free (part of libSystem, not an external library).
//
// BUG FIX: the original called Platform::SystemInfoReady.trigger()
// directly at the end of Processor::init(), but SetProcessorInfo()
// (platformCPU.cpp, generic/shared across every OS) already triggers that
// signal internally once classification is done. Calling it a second time
// here meant every registered listener fired twice per init — the same
// bug already found and fixed on the Windows and Linux ports of this file.
// Removed here too.
//
// Platform::SystemInfo's storage definition is kept (matching the
// original) since, as established while fixing this same class of bug on
// Windows/Linux, platformCPU.cpp defines Platform::SystemInfoReady but
// NOT Platform::SystemInfo itself — that genuinely has no other owner and
// must be defined exactly once per-OS.
//-----------------------------------------------------------------------------
#import <sys/types.h>
#import <sys/sysctl.h>
#import <mach/machine.h>
#import <math.h>
#import <Foundation/Foundation.h>

#import "platform/platformAssert.h"
#import "console/console.h"
#import "core/stringTable.h"
#import "platform/platformCPUCount.h"

// Defined once, generically, in platformCPU.cpp (also owns
// Platform::SystemInfoReady, which it triggers internally).
extern void SetProcessorInfo(Platform::SystemInfo_struct::Processor& pInfo, const char* vendor, const char* brand);

Platform::SystemInfo_struct Platform::SystemInfo;

namespace
{
    // Reads a sysctl() string value into dest (max maxlen bytes).
    // Returns 0 on success, matching errno.h conventions.
    int getSysCTLString(const char key[], char* dest, size_t maxlen)
    {
        size_t len = 0;
        int err = sysctlbyname(key, nullptr, &len, nullptr, 0);
        if (err == 0)
        {
            AssertWarn(len <= maxlen, "Insufficient buffer length for sysctl() read. Truncating.");
            if (len > maxlen)
                len = maxlen;
            err = sysctlbyname(key, dest, &len, nullptr, 0);
        }
        return err;
    }

    // Reads a sysctl() integer value of type T into dest. Apple's own
    // guidance: byte-count/frequency values tend to be ULL, most
    // everything else UL — the size is validated against sizeof(T) below
    // rather than assumed.
    template <typename T>
    int getSysCTLValue(const char key[], T* dest)
    {
        size_t len = 0;
        int err = sysctlbyname(key, nullptr, &len, nullptr, 0);
        if (err == 0)
        {
            AssertFatal(len == sizeof(T), "Mismatched destination type for sysctl() read.");
            err = sysctlbyname(key, dest, &len, nullptr, 0);
        }
        return err;
    }

    constexpr U32 kBaseMhzSpeed = 1000;
    constexpr U32 kBaseAppleSiliconMhzSpeed = 3200;

    void detectCpuFeatures(U32& procflags)
    {
        U32 lraw;

        procflags = CPU_PROP_C | CPU_PROP_FPU;

#if defined(TORQUE_CPU_X86) || defined(TORQUE_CPU_X64)
        if (getSysCTLValue<U32>("hw.optional.mmx", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_MMX;
        if (getSysCTLValue<U32>("hw.optional.sse", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_SSE;
        if (getSysCTLValue<U32>("hw.optional.sse2", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_SSE2;
        if (getSysCTLValue<U32>("hw.optional.sse3", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_SSE3;
        if (getSysCTLValue<U32>("hw.optional.supplementalsse3", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_SSE3ex;
        if (getSysCTLValue<U32>("hw.optional.sse4_1", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_SSE4_1;
        if (getSysCTLValue<U32>("hw.optional.sse4_2", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_SSE4_2;
        if (getSysCTLValue<U32>("hw.optional.avx1_0", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_AVX;
#elif defined(TORQUE_CPU_ARM64)
        if (getSysCTLValue<U32>("hw.optional.neon", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_NEON;
#endif

        if (getSysCTLValue<U32>("hw.ncpu", &lraw) == 0 && lraw > 1) procflags |= CPU_PROP_MP;
        if (getSysCTLValue<U32>("hw.cpu64bit_capable", &lraw) == 0 && lraw == 1) procflags |= CPU_PROP_64bit;
        if (getSysCTLValue<U32>("hw.byteorder", &lraw) == 0 && lraw == 1234) procflags |= CPU_PROP_LE;
    }
}

void Processor::init()
{
    U32 procflags = 0;
    U64 llraw = 0;

    NSString* osVersionStr = [[NSProcessInfo processInfo] operatingSystemVersionString];

    S32 ramMB;
    if (getSysCTLValue<U64>("hw.memsize", &llraw) != 0)
        ramMB = 512;
    else
        ramMB = static_cast<S32>(llraw >> 20);

    char brandString[256];
    if (getSysCTLString("machdep.cpu.brand_string", brandString, sizeof(brandString)) != 0)
        brandString[0] = '\0';

    char vendor[256];
    if (getSysCTLString("machdep.cpu.vendor", vendor, sizeof(vendor)) != 0)
        vendor[0] = '\0';

    // hw.cpufrequency is missing on Apple Silicon; assume the M1's base
    // frequency there, and a generic default for Intel Macs where the
    // sysctl also happens to be unavailable.
    if (getSysCTLValue<U64>("hw.cpufrequency", &llraw) != 0)
    {
#if defined(TORQUE_CPU_ARM64)
        llraw = kBaseAppleSiliconMhzSpeed;
#else
        llraw = kBaseMhzSpeed;
#endif
    }
    else
    {
        llraw /= 1000000;
    }
    Platform::SystemInfo.processor.mhz = static_cast<U32>(llraw);

    detectCpuFeatures(procflags);

    Platform::SystemInfo.processor.properties = procflags;
    SetProcessorInfo(Platform::SystemInfo.processor, vendor, brandString);

    Con::printf("System & Processor Information:");
    Con::printf("   MacOS Version: %s", [osVersionStr UTF8String]);
    Con::printf("   Physical memory installed: %d MB", ramMB);
    Con::printf("   Processor: %s", Platform::SystemInfo.processor.name);
    if (Platform::SystemInfo.processor.properties & CPU_PROP_MMX)    Con::printf("      MMX detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE)    Con::printf("      SSE detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE2)   Con::printf("      SSE2 detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE3)   Con::printf("      SSE3 detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE3ex) Con::printf("      SSE3ex detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE4_1) Con::printf("      SSE4.1 detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_SSE4_2) Con::printf("      SSE4.2 detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_AVX)    Con::printf("      AVX detected");
    if (Platform::SystemInfo.processor.properties & CPU_PROP_NEON)   Con::printf("      Neon detected");

    if (Platform::SystemInfo.processor.properties & CPU_PROP_MP)
        Con::printf("   MultiCore CPU detected [%i cores, %i logical]",
                    Platform::SystemInfo.processor.numPhysicalProcessors,
                    Platform::SystemInfo.processor.numLogicalProcessors);

    Con::printf("");

    // NOTE: no manual SystemInfoReady.trigger() here — SetProcessorInfo()
    // already did it. See file header comment.
}

namespace CPUInfo
{
    EConfig CPUCount(U32& logical, U32& physical)
    {
        U32 lraw;

        physical = (getSysCTLValue<U32>("hw.physicalcpu", &lraw) == 0) ? lraw : 1;

        if (getSysCTLValue<U32>("hw.logicalcpu", &lraw) == 0)
        {
            logical = lraw;
        }
        else if (getSysCTLValue<U32>("hw.ncpu", &lraw) == 0)
        {
            logical = lraw;
        }
        else
        {
            logical = physical;
        }

        const bool smtEnabled = logical > physical;

        if (physical == 1)
            return smtEnabled ? CONFIG_SingleCoreHTEnabled : CONFIG_SingleCoreAndHTNotCapable;

        return smtEnabled ? CONFIG_MultiCoreAndHTEnabled : CONFIG_MultiCoreAndHTNotCapable;
    }
}
