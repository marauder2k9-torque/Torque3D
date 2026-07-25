//-----------------------------------------------------------------------------
// platformMath.cpp — Math::init() and the mathInit console function.
//
// Generic/shared across every OS. Ported from winMath.cpp / POSIXMath.cpp,
// which turned out to be byte-for-byte identical in logic (both only read
// Platform::SystemInfo.processor.properties and call mInstallLibrary_C /
// mInstallLibrary_ASM / math_backend::install_from_cpu_flags — none of
// which are themselves OS-specific at this call site), so this is now one
// file instead of a duplicated-per-OS one.
//
// mInstallLibrary_C() and math_backend::install_from_cpu_flags() are
// defined in the math library module, not the platform layer — this file
// only declares them extern and calls them, matching the original.
// mInstallLibrary_ASM() is defined per-OS (see win32MathASM.cpp /
// linuxMathASM.cpp) since its real content, where it has any, is
// architecture-specific inline assembly.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "math/mMath.h"
#include "math/public/math_backend.h"
#include "core/strings/stringFunctions.h"

extern void mInstallLibrary_C();
extern void mInstallLibrary_ASM();

//-----------------------------------------------------------------------------
DefineEngineStringlyVariadicFunction( mathInit, void, 1, 10, "( ... )"
                "@brief Install the math library with specified extensions.\n\n"
                "Possible parameters are:\n\n"
                "    - 'DETECT' Autodetect math lib settings.\n\n"
                "    - 'C' Enable the C math routines. C routines are always enabled.\n\n"
                "    - 'FPU' Enable floating point unit routines.\n\n"
                "    - 'MMX' Enable MMX math routines.\n\n"
                "    - 'SSE' Enable SSE math routines.\n\n"
                "@ingroup Math")
{
    U32 properties = CPU_PROP_C; // C extensions are always used

    if (argc == 1)
    {
        Math::init(0);
        return;
    }

    for (argc--, argv++; argc; argc--, argv++)
    {
        const char* str = (*argv).getString();

        if (dStricmp(str, "DETECT") == 0) {
            Math::init(0);
            return;
        }
        if (dStricmp(str, "C") == 0) {
            properties |= CPU_PROP_C;
            continue;
        }
        if (dStricmp(str, "FPU") == 0) {
            properties |= CPU_PROP_FPU;
            continue;
        }
        if (dStricmp(str, "MMX") == 0) {
            properties |= CPU_PROP_MMX;
            continue;
        }
        if (dStricmp(str, "SSE") == 0) {
            properties |= CPU_PROP_SSE;
            continue;
        }
        if (dStricmp(str, "SSE2") == 0) {
            properties |= CPU_PROP_SSE2;
            continue;
        }
        Con::printf("Error: MathInit(): ignoring unknown math extension '%s'", str);
    }

    Math::init(properties);
}

//-----------------------------------------------------------------------------
void Math::init(U32 properties)
{
    if (!properties)
        // detect what's available
        properties = Platform::SystemInfo.processor.properties;
    else
        // Make sure we're not asking for anything that's not supported
        properties &= Platform::SystemInfo.processor.properties;

    Con::printf("Math Init:");
    Con::printf("   Installing Standard C extensions");
    mInstallLibrary_C();

    Con::printf("   Installing ISA extensions");
    math_backend::install_from_cpu_flags(properties);

    /*Con::printf("   Installing Assembly extensions");
    mInstallLibrary_ASM();*/

    Con::printf(" ");
}
