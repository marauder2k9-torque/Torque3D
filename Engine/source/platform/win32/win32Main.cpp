//-----------------------------------------------------------------------------
// win32Main.cpp — Windows process entry point (WinMain) and TorqueMain
// wiring.
//
// Fresh rewrite. Scope/dependency notes vs. the original winWindow.cpp:
//   - The original stored the process HINSTANCE in winState.appInstance
//     (Win32PlatState, from platformWin32.h — the native window/render
//     stack's global state). This file no longer touches winState at all,
//     consistent with the rest of this platform-layer rewrite being
//     decoupled from that struct (see win32MsgBox.cpp for the same
//     change and rationale). The HINSTANCE is stored locally here instead
//     and exposed via getProcessInstance() for anything that specifically
//     needs it (e.g. window creation code, which is outside this file's
//     scope).
//   - createFontInit()/createFontShutdown() belong to the font subsystem
//     (platformFont.h and friends), not the platform layer as scoped for
//     this rewrite — but they're still called here, via extern
//     declarations only, because TorqueMain's linked engine genuinely
//     needs them bracketing its run for text rendering to work. This file
//     does not implement them.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "core/util/tVector.h"
#include "core/strings/stringFunctions.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace
{
    HINSTANCE sProcessInstance = nullptr;
}

/// Accessor for the process HINSTANCE captured in WinMain. Anything that
/// needs it (native window creation, etc.) should call this rather than
/// reach into any platform-layer-external global state.
HINSTANCE getProcessInstance()
{
    return sProcessInstance;
}

extern bool LinkConsoleFunctions;
extern S32 TorqueMain(S32 argc, const char **argv);
extern void createFontInit();
extern void createFontShutdown();

//-----------------------------------------------------------------------------
#if !defined(TORQUE_SHARED)

static S32 run(S32 argc, const char **argv)
{
    // Ensures TorqueScript console functions get linked in rather than
    // stripped by the linker as apparently-unused.
    LinkConsoleFunctions = true;

    createFontInit();
    const S32 ret = TorqueMain(argc, argv);
    createFontShutdown();

    return ret;
}

S32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpszCmdLine, S32)
{
    sProcessInstance = hInstance;

    Vector<char*> argv;

    constexpr size_t moduleNameSize = 256;
    char moduleName[moduleNameSize];
    wchar_t wideModuleName[moduleNameSize];
    ::GetModuleFileNameW(nullptr, wideModuleName, static_cast<DWORD>(moduleNameSize));
    ::WideCharToMultiByte(CP_UTF8, 0, wideModuleName, -1, moduleName, static_cast<int>(moduleNameSize), nullptr, nullptr);
    argv.push_back(moduleName);

    // Split lpszCmdLine into whitespace-separated arguments. Deliberately
    // simple (no quoted-argument handling) — matches the original's
    // behavior; a more complete command-line parser is a separate concern
    // from this platform-layer entry-point wiring.
    for (const char *word, *ptr = lpszCmdLine; *ptr; )
    {
        while (dIsspace(*ptr) && *ptr)
            ++ptr;

        for (word = ptr; !dIsspace(*ptr) && *ptr; ++ptr)
            ;

        if (*word)
        {
            const S32 len = static_cast<S32>(ptr - word);
            char* arg = static_cast<char*>(dMalloc(len + 1));
            dStrncpy(arg, word, len);
            arg[len] = 0;
            argv.push_back(arg);
        }
    }

    const S32 retVal = run(argv.size(), const_cast<const char**>(argv.address()));

    for (U32 j = 1; j < argv.size(); ++j)
        dFree(argv[j]);

    return retVal;
}

#else // TORQUE_SHARED

extern "C"
{
    bool torque_engineinit(S32 argc, const char **argv);
    S32  torque_enginetick();
    S32  torque_getreturnstatus();
    bool torque_engineshutdown();
}

S32 TorqueMain(S32 argc, const char **argv)
{
    if (!torque_engineinit(argc, argv))
        return 1;

    while (torque_enginetick())
    {
    }

    torque_engineshutdown();

    return torque_getreturnstatus();
}

#endif // TORQUE_SHARED
