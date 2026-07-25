//-----------------------------------------------------------------------------
// linuxMain.cpp — Linux process entry point (main) and TorqueMain wiring.
//
// Mirrors macMain.mm's shape closely (both are a plain `main()` calling
// TorqueMain, plus the torque_linuxmain/torque_engineinit/tick/shutdown
// wiring used by TORQUE_SHARED builds). InitWindowingSystem() is an
// extern reference only, exactly as macMain.mm treats it — defined by
// whatever window-manager backend is actually linked in (SDL2, per this
// engine's Linux windowing/input direction), not by this file, and its
// call is gated on !TORQUE_DEDICATED so a headless dedicated-server build
// never needs that symbol to exist at all.
//-----------------------------------------------------------------------------
#include "app/mainLoop.h"
#include "platform/platform.h"

// Defined in the SDL window manager backend (see this platform layer's
// windowing direction: SDL2 now, SDL3 later, for window/GL-context/input
// only — everything else in this platform layer stays native). Kept as
// an extern reference rather than assuming any particular window manager
// is linked in, matching macMain.mm.
extern void InitWindowingSystem();

//-----------------------------------------------------------------------------
extern "C"
{
    bool torque_engineinit(int argc, const char **argv);
    int  torque_enginetick();
    S32  torque_getreturnstatus();
    bool torque_engineshutdown();

    int torque_linuxmain(int argc, const char **argv)
    {
        if (!torque_engineinit(argc, argv))
            return 1;

        while (torque_enginetick())
        {
        }

        torque_engineshutdown();

        return torque_getreturnstatus();
    }
}

extern S32 TorqueMain(S32 argc, const char **argv);

#if !defined(TORQUE_SHARED)
int main(int argc, const char **argv)
{
    return TorqueMain(argc, argv);
}
#endif
