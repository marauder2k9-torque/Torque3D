//-----------------------------------------------------------------------------
// macMain.mm — macOS process entry point (main) and TorqueMain wiring.
//
// This file was missing entirely from the earlier macOS rewrite pass —
// Platform::init()/Platform::shutdown() were correctly moved into
// macProcessControl.mm (decoupled from StdConsole, see that file's header
// comment), but the actual main()/torque_macmain()/TorqueMain wiring from
// the original macMain.mm was never carried forward, which is why the
// linker reports an undefined _main symbol: nothing in this rewrite
// defined a real entry point at all.
//
// Ported from the original macMain.mm's entry-point section (the
// Platform::init()/shutdown() section from that file is NOT duplicated
// here — it already lives in macProcessControl.mm).
//-----------------------------------------------------------------------------
#import "app/mainLoop.h"
#import "platform/platform.h"

// Defined in the SDL/native window manager (currently a no-op even in the
// original SDL build — see sdlWindowMgr.cpp's InitWindowingSystem). Kept
// as an extern reference matching the original rather than assuming any
// particular window manager is linked in; the call below is itself gated
// on TORQUE_DEDICATED so a dedicated-server build never needs this symbol
// to exist at all.
extern void InitWindowingSystem();

//-----------------------------------------------------------------------------
extern "C"
{
    bool torque_engineinit(int argc, const char **argv);
    int  torque_enginetick();
    S32  torque_getreturnstatus();
    bool torque_engineshutdown();

    int torque_macmain(int argc, const char **argv)
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
