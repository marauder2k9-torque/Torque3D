//-----------------------------------------------------------------------------
// app/torqueMain.cpp — The single, shared TorqueMain() entry point body.
//
// This replaces the "static exe build" branch that used to live inside
// main.cpp's #ifdef TORQUE_SHARED / #else block. TORQUE_SHARED (the DLL-
// loader mode, where WinMain/macOS main()/Linux main() would dlopen/
// LoadLibrary a separately-built game DLL and call torque_winmain/
// torque_macmain/torque_unixmain through a function pointer) is no longer
// used and hasn't reliably worked in some time — so main.cpp's per-OS
// WinMain/dlopen/LoadLibrary blocks and the TORQUE_SHARED plumbing are
// gone, not preserved-but-disabled. There is now exactly one entry-point
// idea per OS: win32Main.cpp/macMain.mm/linuxMain.cpp each own their own
// real OS entry point (WinMain, main()) and call TorqueMain() directly,
// unconditionally, with no #ifdef branching in any of them.
//
// TorqueMain() itself is genuinely OS-agnostic (it always was — none of
// this function's body ever depended on which OS it's running on) and is
// therefore the correct thing to share across every platform's *Main.cpp
// rather than duplicate. It lives in its own translation unit (app/, not
// platform/) since it's engine bring-up/shutdown orchestration, not
// platform-layer code.
//
// FUTURE MODULE SYSTEM NOTE: EngineModuleManager::initializeSystem()/
// shutdownSystem() (called from within StandardMainLoop::init()/
// shutdown(), not directly here) are the existing, real hook this engine
// already has for module registration — that's the natural seam a future
// "swap graphics backend via a module" system would extend, rather than
// anything added to TorqueMain() itself. TorqueMain() stays a thin,
// stable orchestration shell around StandardMainLoop; it should not grow
// per-backend or per-module branching directly, so that adding/removing
// modules later doesn't require touching (or re-forking per-OS) this file.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "app/mainLoop.h"
#include "T3D/gameFunctions.h"
#include "platform/platformMemory.h"

#ifdef TORQUE_TESTS_ENABLED
#include "testing/unitTesting.h"
#endif

#if defined(TORQUE_OS_WIN)
// Tell switchable-graphics-capable systems (laptops with both an
// integrated and a discrete GPU) to prefer the discrete GPU for this
// process. Windows-specific exported globals that the NVIDIA/AMD drivers
// look for by name; no equivalent mechanism exists (or is needed) on
// macOS/Linux, which is why this whole block stays Windows-only rather
// than being generalized.
#include <windows.h>
extern "C" { __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001; }
extern "C" { __declspec(dllexport) DWORD AmdPowerXpressRequestHighPerformance = 0x00000001; }
#else
extern "C" { int NvOptimusEnablement = 1; }
extern "C" { int AmdPowerXpressRequestHighPerformance = 1; }
#endif

//-----------------------------------------------------------------------------
// Entry point for your game.
//
// This is built by default using the "StandardMainLoop" toolkit. Feel free
// to bring code over directly as you need to modify or extend things. You
// will need to merge against future changes to the SML code if you do this.
//
// Called by exactly one function per OS: win32Main.cpp's WinMain/run(),
// macMain.mm's main(), linuxMain.cpp's main(). None of those files branch
// on TORQUE_SHARED anymore — each is unconditionally a static-executable
// entry point that hands off to this function immediately.
//-----------------------------------------------------------------------------
S32 TorqueMain(S32 argc, const char** argv)
{
   // Some handy debugging code:
   //   if (argc == 1) {
   //      static const char* argvFake[] = { "dtest.exe", "-jload", "test.jrn" };
   //      argc = 3;
   //      argv = argvFake;
   //   }

#if defined( TORQUE_ENABLE_ASSERTS ) && !defined(TORQUE_DISABLE_MEMORY_MANAGER)
   Memory::init();
#endif

   // Initialize the subsystems. Identical for both the normal and testing
   // paths — StandardMainLoop::init() has no knowledge of TORQUE_TESTING
   // and shouldn't need any; it always brings up the same engine.
   StandardMainLoop::init();

   S32 returnStatus;

#ifdef TORQUE_TESTS_ENABLED
   // Test path: RunUnitTests() owns command-line handling itself
   // (calling StandardMainLoop::handleCommandLine() internally
   returnStatus = RunUnitTests(argc, argv);
#else
   if (!StandardMainLoop::handleCommandLine(argc, argv))
   {
      Platform::AlertOK("Error", "Failed to initialize game, shutting down.");
      return 1;
   }

   while (StandardMainLoop::doMainLoop());

   returnStatus = StandardMainLoop::getReturnStatus();
#endif

   // Clean everything up. Identical for both paths.
   StandardMainLoop::shutdown();

   // Do we need to restart?
   if (StandardMainLoop::requiresRestart())
      Platform::restartInstance();

   return returnStatus;
}
