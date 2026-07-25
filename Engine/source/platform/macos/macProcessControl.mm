//-----------------------------------------------------------------------------
// macProcessControl.mm — macOS FPU control state + RNG.
//
// Ported from macMath.mm, with real changes:
//
//   1. Math::init() and the mathInit console function are NOT redefined
//      here. platformMath.cpp (generic, shared across every OS) already
//      defines Math::init() with identical logic to what macMath.mm had —
//      this was actually true of the original Windows/POSIX math files
//      too (see platformMath.cpp's own header comment), and mac's version
//      turned out to be the same shape as well. Redefining it here would
//      be a duplicate-symbol link error (the same class of mistake
//      already caught and fixed for Platform::SystemInfoReady on
//      Windows/Linux/macOS's CPU files).
//
//   2. getMathControlState/setMathControlState/setMathControlStateKnown
//      are genuine no-ops on this target. The original gated its x87
//      inline-asm control-word manipulation on TORQUE_CPU_X86 (32-bit
//      only) and returned/did 0 otherwise — meaning on x86_64 Intel Macs
//      AND Apple Silicon alike, the original's real body never actually
//      ran. There is no x87 FPU on x86_64 (SSE2 handles floating point,
//      which has no control-word precision concept) or on Arm64, so this
//      is not a stub standing in for missing functionality — it's the
//      same "nothing to do here" conclusion already reached for the
//      equivalent Windows functions (see win32ProcessControl.cpp).
//
//   3. getRandom() no longer depends on sgPlatRandom (an engine-level
//      MRandomLCG instance from outside the platform layer) — uses
//      <random> directly instead, matching the Windows/Linux ports of
//      this function.
//-----------------------------------------------------------------------------
#import <Cocoa/Cocoa.h>
#import <unistd.h>

#import "platform/platform.h"
#import "console/console.h"
#import "core/stringTable.h"
#import "platform/platformInput.h"
#import "core/util/journal/process.h"

#import <random>

//-----------------------------------------------------------------------------
// Platform::init()/shutdown()
//
// Ported from macMain.mm, with one real change: the original called
// StdConsole::create() + stdConsole->enable(true) here, which is the
// exact POSIX-console coupling this rewrite has been removing from macOS
// throughout (macOS gets its own native console implementation instead —
// see macConsole.mm — rather than sharing StdConsole with Linux). This
// was also the root cause of macOS's console double-logging: StdConsole's
// own echo, layered on top of the engine's default stdout consumer,
// produced every line twice. Not touching StdConsole here at all removes
// that double-write path entirely rather than just working around it.
//-----------------------------------------------------------------------------
void Platform::init()
{
    Con::printf("Initializing platform...");

    Con::setVariable("$platform", "macos");

    Input::init();
}

void Platform::shutdown()
{
    Input::destroy();
}

//-----------------------------------------------------------------------------
// Completely closes and restarts the simulation.
//-----------------------------------------------------------------------------
void Platform::restartInstance()
{
    @autoreleasepool {
        NSBundle* mainAppBundle = [NSBundle mainBundle];
        NSString* execString = [mainAppBundle executablePath];

        NSMutableString* mut = [[NSMutableString alloc] init];
        [mut appendString:execString];
        [mut insertString:@"\"" atIndex:0];
        [mut appendString:@"\" & "];
        [mut appendString:@"\\0"];

        const char* execCString = [mut UTF8String];

        Con::printf("---- %s -----", execCString);

        system(execCString);
    }
}

void Platform::postQuitMessage(const S32 in_quitVal)
{
    Process::requestShutdown();
}

void Platform::forceShutdown(S32 returnValue)
{
    [NSApp performSelector:@selector(terminate:) withObject:nil afterDelay:0.0];
}

void Platform::debugBreak()
{
    raise(SIGTRAP);
}

//-----------------------------------------------------------------------------
// Various directories.
//-----------------------------------------------------------------------------
const char* Platform::getUserDataDirectory()
{
    // Application Support is the conventional per-user app-data location
    // on macOS — the same role %APPDATA% plays on Windows.
    NSString* nsDataDir = [@"~/Library/Application Support/" stringByStandardizingPath];
    return StringTable->insert([nsDataDir UTF8String]);
}

const char* Platform::getUserHomeDirectory()
{
    return StringTable->insert([[@"~/Documents" stringByStandardizingPath] UTF8String]);
}

StringTableEntry osGetTemporaryDirectory()
{
    NSString* tdir = NSTemporaryDirectory();
    return StringTable->insert([tdir UTF8String]);
}

//-----------------------------------------------------------------------------
// Debug output.
//-----------------------------------------------------------------------------
void Platform::outputDebugString(const char *string, ...)
{
    char buffer[2048];

    va_list args;
    va_start(args, string);
    dVsprintf(buffer, sizeof(buffer), string, args);
    va_end(args);

    U32 length = static_cast<U32>(strlen(buffer));
    if (length == sizeof(buffer) - 1)
        --length;

    buffer[length]     = '\n';
    buffer[length + 1] = '\0';

    fputs(buffer, stderr);
    fflush(stderr);
}

//-----------------------------------------------------------------------------
bool Platform::openWebBrowser(const char* webAddress)
{
    @autoreleasepool {
        NSURL* url = [NSURL URLWithString:[NSString stringWithUTF8String:webAddress]];
        return [[NSWorkspace sharedWorkspace] openURL:url];
    }
}

#ifndef TORQUE_SDL
void Platform::openFolder(const char* path)
{
    @autoreleasepool {
        char filePath[1024];
        Platform::makeFullPathName(path, filePath, sizeof(filePath));

        NSString* nsPath = [NSString stringWithUTF8String:filePath];
        NSURL* url = [NSURL fileURLWithPath:nsPath isDirectory:YES];
        [[NSWorkspace sharedWorkspace] openURL:url];
    }
}

void Platform::openFile(const char* path)
{
    @autoreleasepool {
        char filePath[1024];
        Platform::makeFullPathName(path, filePath, sizeof(filePath));

        NSString* nsPath = [NSString stringWithUTF8String:filePath];
        NSURL* url = [NSURL fileURLWithPath:nsPath isDirectory:NO];
        [[NSWorkspace sharedWorkspace] openURL:url];
    }
}
#endif // !TORQUE_SDL

//-----------------------------------------------------------------------------
bool Platform::getUserIsAdministrator()
{
    // If we can write to /Library, we're probably an admin. Not a rigorous
    // check (chmod could change this), but matches the original's own
    // documented caveat.
    return access("/Library", W_OK) == 0;
}

U32 Platform::getMathControlState()
{
#ifdef TORQUE_CPU_X86
   U16 cw;
   asm("fstcw %0" : "=m" (cw) :);
   return cw;
#else
   return 0;
#endif
}

void Platform::setMathControlState(U32 state)
{
#ifdef TORQUE_CPU_X86
   U16 cw = state;
   asm("fldcw %0" : : "m" (cw));
#endif
}

void Platform::setMathControlStateKnown()
{
#ifdef TORQUE_CPU_X86
   U16 cw = 0x27F;
   asm("fldcw %0" : : "m" (cw));
#endif
}

F32 Platform::getRandom()
{
    static thread_local std::mt19937 engine{ std::random_device{}() };
    static thread_local std::uniform_real_distribution<F32> dist(0.0f, 1.0f);
    return dist(engine);
}
