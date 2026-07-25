//-----------------------------------------------------------------------------
// linuxCPU.cpp — Linux implementation of platformCPU / Processor::init().
//
// Fully native: no libraries beyond libc. Core/thread counts come from
// sysconf(_SC_NPROCESSORS_ONLN) plus a /proc/cpuinfo "physical id"/"core id"
// pass to distinguish physical cores from SMT siblings; CPU vendor/brand and
// feature flags come from CPUID directly on x86_64 (same leaves win32CPU.cpp
// reads) or from /proc/cpuinfo's "Features"/"CPU part" fields on Arm64,
// since Arm has no CPUID-equivalent instruction usable from user space.
//
// Uses the shared SetProcessorInfo() helper from platformCPU.cpp (also
// used by macCPU.mm) for vendor/type classification and the
// SystemInfoReady signal trigger, rather than duplicating that logic here.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformCPUCount.h"
#include "core/stringTable.h"
#include "console/console.h"

#include <unistd.h>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <set>
#include <utility>
#include <cstring>
#include <cstdlib>

#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
#include <cpuid.h>
#endif

// Defined once, generically, in platformCPU.cpp (also owns
// Platform::SystemInfoReady, which it triggers internally).
extern void SetProcessorInfo(Platform::SystemInfo_struct::Processor& pInfo, const char* vendor, const char* brand);

// Storage definition for Platform::SystemInfo. Declared as `extern` in
// platform.h; exactly one backend per OS must define the real storage.
Platform::SystemInfo_struct Platform::SystemInfo;

namespace
{
    // Reads /proc/cpuinfo once, returning it as a single string for the
    // small helpers below to scan. /proc is a native Linux kernel
    // interface, not a "dependency" in any meaningful sense.
    std::string readProcCpuInfo()
    {
        std::ifstream file("/proc/cpuinfo");
        if (!file.is_open())
            return std::string();

        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    // Counts distinct (physical id, core id) pairs across all "processor"
    // entries in /proc/cpuinfo to get the true physical core count,
    // falling back to sysconf's logical count if the fields are absent
    // (some virtualized/container environments omit them).
    void countLogicalAndPhysicalCores(U32& logicalOut, U32& physicalOut)
    {
        logicalOut = static_cast<U32>(sysconf(_SC_NPROCESSORS_ONLN));
        if (static_cast<S32>(logicalOut) <= 0)
            logicalOut = 1;

        const std::string info = readProcCpuInfo();
        if (info.empty())
        {
            physicalOut = logicalOut;
            return;
        }

        std::set<std::pair<S32, S32>> uniqueCores;
        S32 curPhysicalId = 0;
        S32 curCoreId = -1;
        bool haveCoreId = false;

        std::istringstream stream(info);
        std::string line;
        while (std::getline(stream, line))
        {
            const auto colon = line.find(':');
            if (colon == std::string::npos)
                continue;

            std::string key = line.substr(0, colon);
            std::string value = line.substr(colon + 1);

            // Trim trailing whitespace from key, leading whitespace from value.
            while (!key.empty() && (key.back() == ' ' || key.back() == '\t'))
                key.pop_back();
            size_t vstart = value.find_first_not_of(" \t");
            value = (vstart == std::string::npos) ? std::string() : value.substr(vstart);

            if (key == "physical id")
            {
                curPhysicalId = std::atoi(value.c_str());
            }
            else if (key == "core id")
            {
                curCoreId = std::atoi(value.c_str());
                haveCoreId = true;
                uniqueCores.insert({ curPhysicalId, curCoreId });
            }
        }

        physicalOut = haveCoreId && !uniqueCores.empty()
            ? static_cast<U32>(uniqueCores.size())
            : logicalOut;

        if (physicalOut == 0)
            physicalOut = 1;
    }

#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
    void getVendorAndBrand(char (&vendor)[13], char (&brand)[49])
    {
        std::memset(vendor, 0, sizeof(vendor));
        std::memset(brand, 0, sizeof(brand));

        U32 eax, ebx, ecx, edx;
        if (__get_cpuid(0, &eax, &ebx, &ecx, &edx))
        {
            std::memcpy(vendor + 0, &ebx, 4);
            std::memcpy(vendor + 4, &edx, 4);
            std::memcpy(vendor + 8, &ecx, 4);
        }

        U32 maxExt = 0;
        __get_cpuid(0x80000000, &maxExt, &ebx, &ecx, &edx);
        if (maxExt >= 0x80000004)
        {
            for (U32 i = 0; i < 3; ++i)
            {
                U32 regs[4];
                __get_cpuid(0x80000002 + i, &regs[0], &regs[1], &regs[2], &regs[3]);
                std::memcpy(brand + i * 16, regs, 16);
            }
        }
    }

    U32 parseMhzFromBrandString(const char* brandStr)
    {
        const std::string_view sv(brandStr);
        const auto at = sv.rfind('@');
        if (at == std::string_view::npos)
            return 0;

        std::string_view tail = sv.substr(at + 1);
        const auto ghzPos = tail.find("GHz");
        if (ghzPos == std::string_view::npos)
            return 0;

        std::size_t start = 0;
        while (start < tail.size() && (tail[start] == ' ' || tail[start] == '\t'))
            ++start;

        char* endPtr = nullptr;
        const double ghz = std::strtod(std::string(tail.substr(start, ghzPos - start)).c_str(), &endPtr);
        return static_cast<U32>(ghz * 1000.0);
    }

    enum CpuFlagBits : U32
    {
        BIT_MMX     = 1u << 23, // EDX (leaf 1)
        BIT_SSE     = 1u << 25,
        BIT_SSE2    = 1u << 26,
        BIT_SSE3    = 1u << 0,  // ECX (leaf 1)
        BIT_SSE3ex  = 1u << 9,
        BIT_SSE4_1  = 1u << 19,
        BIT_SSE4_2  = 1u << 20,
        BIT_XSAVE   = 1u << 27,
        BIT_AVX     = 1u << 28,
        BIT_AVX2    = 1u << 5,  // EBX (leaf 7)
        BIT_AVX512F = 1u << 16,
    };

    void detectFeatureFlags(Platform::SystemInfo_struct::Processor& p)
    {
        U32 eax, ebx, ecx, edx;
        if (!__get_cpuid(1, &eax, &ebx, &ecx, &edx))
            return;

        if (edx & BIT_MMX)    p.properties |= CPU_PROP_MMX;
        if (edx & BIT_SSE)    p.properties |= CPU_PROP_SSE;
        if (edx & BIT_SSE2)   p.properties |= CPU_PROP_SSE2;
        if (ecx & BIT_SSE3)   p.properties |= CPU_PROP_SSE3;
        if (ecx & BIT_SSE3ex) p.properties |= CPU_PROP_SSE3ex;
        if (ecx & BIT_SSE4_1) p.properties |= CPU_PROP_SSE4_1;
        if (ecx & BIT_SSE4_2) p.properties |= CPU_PROP_SSE4_2;

        U32 maxLeaf = eax;
        if (maxLeaf >= 7)
        {
            U32 a7, b7, c7, d7;
            __get_cpuid_count(7, 0, &a7, &b7, &c7, &d7);
            if (b7 & BIT_AVX2)    p.properties |= CPU_PROP_AVX2;
            if (b7 & BIT_AVX512F) p.properties |= CPU_PROP_AVX512;
        }

        // AVX also requires OS/XSAVE support, checked via XGETBV — but
        // that instruction needs inline asm since <cpuid.h> doesn't wrap
        // it. Guard on OSXSAVE (ecx bit 27) before trusting the AVX bit.
        if ((ecx & BIT_XSAVE) && (ecx & BIT_AVX))
        {
            U32 xcrLow, xcrHigh;
            asm volatile("xgetbv" : "=a"(xcrLow), "=d"(xcrHigh) : "c"(0));
            if (xcrLow & 0x6)
                p.properties |= CPU_PROP_AVX;
        }
    }
#endif // x86/x64

#if defined(TORQUE_CPU_ARM64) || defined(TORQUE_CPU_ARM32)
    // No user-space CPUID-equivalent exists on Arm; /proc/cpuinfo's
    // "Features" line is the standard native way to query SIMD/crypto
    // extension support on Linux.
    bool procCpuInfoHasFeature(const std::string& info, const char* feature)
    {
        const auto pos = info.find("Features");
        if (pos == std::string::npos)
            return false;
        const auto lineEnd = info.find('\n', pos);
        const std::string featureLine = info.substr(pos, lineEnd - pos);
        return featureLine.find(feature) != std::string::npos;
    }
#endif
}

void Processor::init()
{
    auto& p = Platform::SystemInfo.processor;

    p.properties = CPU_PROP_C | CPU_PROP_FPU | CPU_PROP_LE;
#if TORQUE_ARCH_64BIT
    p.properties |= CPU_PROP_64bit;
#endif

    countLogicalAndPhysicalCores(p.numLogicalProcessors, p.numPhysicalProcessors);
    p.isMultiCore = p.numPhysicalProcessors > 1;
    p.isHyperThreaded = p.numLogicalProcessors > p.numPhysicalProcessors;
    if (p.isMultiCore)
        p.properties |= CPU_PROP_MP;

    char vendor[13] = { 0 };
    char brand[49] = { 0 };

#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
    getVendorAndBrand(vendor, brand);
    p.mhz = parseMhzFromBrandString(brand);
    detectFeatureFlags(p);
#elif defined(TORQUE_CPU_ARM64) || defined(TORQUE_CPU_ARM32)
    std::strncpy(vendor, "Arm", sizeof(vendor) - 1);

    const std::string info = readProcCpuInfo();
    if (procCpuInfoHasFeature(info, "asimd") || procCpuInfoHasFeature(info, "neon"))
        p.properties |= CPU_PROP_NEON;

    // "model name" is present on some Arm distros, "Hardware" on older
    // ones; fall back to a generic label if neither is found.
    for (const char* key : { "model name", "Hardware" })
    {
        const auto pos = info.find(key);
        if (pos == std::string::npos)
            continue;
        const auto colon = info.find(':', pos);
        const auto lineEnd = info.find('\n', pos);
        if (colon == std::string::npos || colon >= lineEnd)
            continue;
        std::string value = info.substr(colon + 1, lineEnd - colon - 1);
        size_t vstart = value.find_first_not_of(" \t");
        if (vstart != std::string::npos)
            std::strncpy(brand, value.c_str() + vstart, sizeof(brand) - 1);
        break;
    }
    p.mhz = 0; // No reliable, dependency-free base-clock source on Arm/Linux.
#else
    #error "linuxCPU.cpp: unsupported CPU architecture"
#endif

    SetProcessorInfo(p, vendor, brand[0] ? brand : nullptr);

    Con::printf("Processor Init:");
    Con::printf("   Processor: %s", p.name);
    Con::printf("   Cores: %u physical / %u logical", p.numPhysicalProcessors, p.numLogicalProcessors);
    if (p.mhz)
        Con::printf("   Base clock: ~%u MHz", p.mhz);
    if (p.properties & CPU_PROP_SSE2)   Con::printf("      SSE2 detected");
    if (p.properties & CPU_PROP_AVX)    Con::printf("      AVX detected");
    if (p.properties & CPU_PROP_AVX2)   Con::printf("      AVX2 detected");
    if (p.properties & CPU_PROP_AVX512) Con::printf("      AVX512 detected");
    if (p.properties & CPU_PROP_NEON)   Con::printf("      NEON detected");
    Con::printf(" ");

    // NOTE: no manual SystemInfoReady.trigger() here — SetProcessorInfo()
    // already did it (see that function in platformCPU.cpp). Triggering
    // it again here would be the exact double-fire bug already caught
    // and fixed on the Windows/macOS ports of this file.
}

namespace CPUInfo
{
    EConfig CPUCount(U32& logical, U32& physical)
    {
        logical = Platform::SystemInfo.processor.numLogicalProcessors;
        physical = Platform::SystemInfo.processor.numPhysicalProcessors;

        const bool ht = Platform::SystemInfo.processor.isHyperThreaded;
        if (ht && physical == 1)
            return CONFIG_SingleCoreHTEnabled;
        if (!ht && physical > 1)
            return CONFIG_MultiCoreAndHTNotCapable;
        if (!ht && physical == 1)
            return CONFIG_SingleCoreAndHTNotCapable;
        return CONFIG_MultiCoreAndHTEnabled;
    }
}
