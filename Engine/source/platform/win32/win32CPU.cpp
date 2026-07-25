//-----------------------------------------------------------------------------
// win32CPU.cpp — Windows implementation of platformCPU / Processor::init().
//
// Fresh C++17 rewrite. Two corrections versus the original winCPUInfo.cpp/
// winPlatformCPUCount.cpp:
//
//   1. Core counting uses GetLogicalProcessorInformationEx (not the older,
//      non-Ex GetLogicalProcessorInformation) because the legacy API
//      reports processor affinity as a single ULONG_PTR bitmask, which
//      can only represent up to 64 logical processors per group and
//      silently undercounts on modern many-core CPUs. The Ex variant
//      reports per-processor-group data explicitly, which is the
//      documented-correct way to enumerate cores on anything with more
//      than 64 logical processors (workstation/server CPUs are common
//      enough now that this isn't a hypothetical edge case).
//
//   2. CPU frequency no longer reads
//      HKLM\Hardware\Description\System\CentralProcessor\0\~MHz. That
//      registry value reflects the CPU's speed AT BOOT TIME, not its
//      actual/base frequency, and is explicitly called out by Microsoft
//      as unreliable on modern CPUs with dynamic frequency scaling.
//      Instead, this parses the "@ X.XXGHz" suffix commonly present in
//      the CPUID brand string, which is both more reliable and requires
//      no registry access at all.
//
// ARM64 COMPILE FIX (this revision): getVendorAndBrand(), the CpuFlagBits
// enum, and detectFeatureFlags() are now wrapped in
// "#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)" at file scope.
// Previously only the *call sites* inside Processor::init() were gated by
// CPU architecture — the function bodies themselves were not, and they
// call __cpuid/__cpuidex/_xgetbv unconditionally. Those intrinsics are
// x86/x64-only: <intrin.h> does not declare them when targeting
// ARM64/ARM64EC, so the previous version of this file failed to compile
// there at all (not merely produced dead code) — this was a real,
// current build break for Windows-on-Arm, not a hypothetical. Wrapping
// the definitions themselves, matching the existing Processor::init()
// branching, fixes this without changing any x86/x64 behavior.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformCPUCount.h"
#include "core/stringTable.h"
#include "console/console.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>

#include <vector>
#include <cstring>
#include <cstdlib>
#include <charconv>
#include <string_view>

// Storage definition for Platform::SystemInfo. Declared as `extern` in
// platform.h; every backend that populates it (this file, on Windows)
// must define the actual storage exactly once, or any translation unit
// that references Platform::SystemInfo fails to link (this was missing
// entirely — LNK2001/LNK2019 unresolved external symbol).
Platform::SystemInfo_struct Platform::SystemInfo;

namespace
{
   void countLogicalAndPhysicalCores(U32& logicalOut, U32& physicalOut)
   {
      logicalOut = 0;
      physicalOut = 0;

      DWORD bufferSize = 0;
      GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize);
      if (bufferSize == 0)
      {
         logicalOut = physicalOut = 1;
         return;
      }

      std::vector<U8> buffer(bufferSize);
      auto* info = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data());

      if (!GetLogicalProcessorInformationEx(RelationProcessorCore, info, &bufferSize))
      {
         Con::errorf("Unable to determine CPU topology (GetLogicalProcessorInformationEx failed), assuming 1 core");
         logicalOut = physicalOut = 1;
         return;
      }

      DWORD offset = 0;
      while (offset < bufferSize)
      {
         auto* entry = reinterpret_cast<SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*>(buffer.data() + offset);
         if (entry->Relationship == RelationProcessorCore)
         {
            ++physicalOut;
            for (WORD g = 0; g < entry->Processor.GroupCount; ++g)
               logicalOut += static_cast<U32>(__popcnt64(entry->Processor.GroupMask[g].Mask));
         }
         offset += entry->Size;
      }

      if (physicalOut == 0) physicalOut = 1;
      if (logicalOut == 0)  logicalOut = physicalOut;
   }

   // NOTE: __popcnt64 above is a Windows CRT intrinsic available on both
   // x64 and ARM64 (it's not the x86-CPUID-only kind), so
   // countLogicalAndPhysicalCores() itself needs no architecture gating.

#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
    // Everything below this point is x86/x64-only: __cpuid/__cpuidex/
    // _xgetbv are backed by real CPUID/XGETBV instructions that simply
    // don't exist on Arm, and <intrin.h> does not declare them for an
    // ARM64/ARM64EC compilation target. See file header comment.

   void getVendorAndBrand(char(&vendor)[13], char(&brand)[49])
   {
      std::memset(vendor, 0, sizeof(vendor));
      std::memset(brand, 0, sizeof(brand));

      S32 regs[4];
      __cpuid(regs, 0);
      std::memcpy(vendor + 0, &regs[1], 4); // ebx
      std::memcpy(vendor + 4, &regs[3], 4); // edx
      std::memcpy(vendor + 8, &regs[2], 4); // ecx

      S32 extInfo[4];
      __cpuid(extInfo, 0x80000000);
      if (static_cast<U32>(extInfo[0]) >= 0x80000004)
      {
         for (int i = 0; i < 3; ++i)
         {
            S32 b[4];
            __cpuidex(b, 0x80000002 + i, 0);
            std::memcpy(brand + i * 16, b, 16);
         }
      }
   }

   // Parses a base clock speed in MHz out of a CPUID brand string like
   // "Intel(R) Core(TM) i7-12700K CPU @ 3.60GHz". Returns 0 if no such
   // suffix is present (some CPUs, particularly some AMD parts, omit it).
   U32 parseMhzFromBrandString(const char* brand)
   {
      const std::string_view sv(brand);
      const auto at = sv.rfind('@');
      if (at == std::string_view::npos)
         return 0;

      std::string_view tail = sv.substr(at + 1);
      const auto ghzPos = tail.find("GHz");
      if (ghzPos == std::string_view::npos)
         return 0;

      // Trim leading spaces before the number.
      std::size_t start = 0;
      while (start < tail.size() && (tail[start] == ' ' || tail[start] == '\t'))
         ++start;

      double ghz = 0.0;
      const auto result = std::from_chars(tail.data() + start, tail.data() + ghzPos, ghz);
      if (result.ec != std::errc())
         return 0;

      return static_cast<U32>(ghz * 1000.0);
   }

   enum CpuFlagBits : U32
   {
        BIT_MMX    = 1u << 23, // EDX (leaf 1)
        BIT_SSE    = 1u << 25,
        BIT_SSE2   = 1u << 26,
        BIT_SSE3   = 1u << 0,  // ECX (leaf 1)
      BIT_SSE3ex = 1u << 9,
      BIT_SSE4_1 = 1u << 19,
      BIT_SSE4_2 = 1u << 20,
        BIT_XSAVE  = 1u << 27,
        BIT_AVX    = 1u << 28,
        BIT_AVX2   = 1u << 5,  // EBX (leaf 7)
      BIT_AVX512F = 1u << 16,
   };

   void detectFeatureFlags(Platform::SystemInfo_struct::Processor& p)
   {
      S32 leaf1[4];
      __cpuid(leaf1, 1);
      const U32 edx = static_cast<U32>(leaf1[3]);
      const U32 ecx = static_cast<U32>(leaf1[2]);

      if (edx & BIT_MMX)  p.properties |= CPU_PROP_MMX;
      if (edx & BIT_SSE)  p.properties |= CPU_PROP_SSE;
      if (edx & BIT_SSE2) p.properties |= CPU_PROP_SSE2;
      if (ecx & BIT_SSE3)   p.properties |= CPU_PROP_SSE3;
      if (ecx & BIT_SSE3ex) p.properties |= CPU_PROP_SSE3ex;
      if (ecx & BIT_SSE4_1) p.properties |= CPU_PROP_SSE4_1;
      if (ecx & BIT_SSE4_2) p.properties |= CPU_PROP_SSE4_2;

      if (leaf1[0] >= 7)
      {
         S32 leaf7[4];
         __cpuidex(leaf7, 7, 0);
         const U32 ebx = static_cast<U32>(leaf7[1]);
         if (ebx & BIT_AVX2)    p.properties |= CPU_PROP_AVX2;
         if (ebx & BIT_AVX512F) p.properties |= CPU_PROP_AVX512;
      }

      // AVX requires OS support via XSAVE/XGETBV, not just the CPUID bit.
      if ((ecx & BIT_XSAVE) && (ecx & BIT_AVX))
      {
         if (_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6)
            p.properties |= CPU_PROP_AVX;
      }
   }
#endif // defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
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

#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_X86)
   char vendor[13];
   char brand[49];
   getVendorAndBrand(vendor, brand);

   if (std::strcmp(vendor, "GenuineIntel") == 0)
      p.type = CPU_Intel;
   else if (std::strcmp(vendor, "AuthenticAMD") == 0)
      p.type = CPU_AMD;
   else
      p.type = CPU_X86Compatible;

   p.name = StringTable->insert(brand[0] ? brand : "Unknown x86_64 Processor");
    p.mhz  = parseMhzFromBrandString(brand);

   detectFeatureFlags(p);
#elif defined(TORQUE_CPU_ARM64)
   p.type = CPU_ArmCompatible;
   p.name = StringTable->insert("Unknown Arm64 Processor");
   p.properties |= CPU_PROP_NEON;
   p.mhz = 0;
#else
#error "win32CPU.cpp: unsupported CPU architecture"
#endif

   Con::printf("Processor Init:");
   Con::printf("   Processor: %s", p.name);
   Con::printf("   Cores: %u physical / %u logical", p.numPhysicalProcessors, p.numLogicalProcessors);
   if (p.mhz)
      Con::printf("   Base clock: ~%u MHz (from CPUID brand string)", p.mhz);
   if (p.properties & CPU_PROP_SSE2)   Con::printf("      SSE2 detected");
   if (p.properties & CPU_PROP_AVX)    Con::printf("      AVX detected");
   if (p.properties & CPU_PROP_AVX2)   Con::printf("      AVX2 detected");
   if (p.properties & CPU_PROP_AVX512) Con::printf("      AVX512 detected");
   if (p.properties & CPU_PROP_NEON)   Con::printf("      NEON detected");
   Con::printf(" ");

   Platform::SystemInfoReady.trigger();
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
