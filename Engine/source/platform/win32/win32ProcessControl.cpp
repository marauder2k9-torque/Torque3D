//-----------------------------------------------------------------------------
// win32ProcessControl.cpp — Windows implementation of the remaining
// process-control surface declared in platform.h: init/shutdown, debug
// break, debug string output, quit/force-shutdown, restart, FPU control
// word get/set, and a basic RNG.
//
// Fresh C++17 rewrite. Scope notes vs. the original winWindow.cpp/
// winMath_ASM.cpp:
//   - Platform::init()/shutdown() here only do genuinely platform-layer
//     work (console bring-up, Input::init/destroy). The original also
//     touched installRedBookDevices() (CD-audio subsystem),
//     GFXDevice::destroy() (graphics device), and a single-instance
//     mutex handle (gMutexHandle, tied to excludeOtherInstances) — all
//     real, but each belongs to a different subsystem, not the platform
//     layer as scoped for this rewrite (fileIO/volume/timer/CPU/memory/
//     assert/console). Wiring those back in is a job for whatever owns
//     each subsystem's own init sequence, not this file.
//   - getMathControlState/setMathControlState/setMathControlStateKnown
//     use _controlfp_s (the modern, x64-safe MSVC CRT API) exclusively.
//     The original had a legacy branch using raw 32-bit-only MSVC inline
//     assembly (_asm { fstcw cw }), which doesn't compile under x64 MSVC
//     at all and isn't relevant to an x86_64/Arm64 target.
//   - getRandom() no longer depends on sgPlatRandom (an engine-level
//     MRandom/RandomLCG instance from outside the platform layer) —
//     uses <random> directly instead, so this file has no dependency on
//     anything outside platform/ and the CRT.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "platform/platformInput.h"
#include "platform/win32/win32Console.h"
#include "console/console.h"
#include "core/util/journal/process.h"
#include "core/strings/unicode.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <float.h>

#include <random>
#include <cstdarg>
#include <cstring>

static HANDLE gMutexHandle = NULL;

//-----------------------------------------------------------------------------
void Platform::postQuitMessage(const S32 in_quitVal)
{
   if (!Platform::getWebDeployment())
      Process::requestShutdown();
}

void Platform::debugBreak()
{
   ::DebugBreak();
}

void Platform::forceShutdown(S32 returnValue)
{
   // Deliberately not ExitProcess — that skips destructors process-wide
   // and can leave other threads in an inconsistent state.
   exit(returnValue);
}

void Platform::outputDebugString(const char* string, ...)
{
   char buffer[2048];

   va_list args;
   va_start(args, string);
   dVsprintf(buffer, sizeof(buffer), string, args);
   va_end(args);

   // Append a newline directly into the buffer rather than issuing a
   // second OutputDebugStringA call — in a multithreaded process another
   // thread's output could otherwise land between the two calls.
   U32 length = static_cast<U32>(std::strlen(buffer));
   if (length == sizeof(buffer) - 1)
      --length;

   buffer[length] = '\n';
   buffer[length + 1] = '\0';

   ::OutputDebugStringA(buffer);
}

bool Platform::openWebBrowser(const char* webAddress)
{
   const std::wstring wideAddress = [&]() {
      const int len = MultiByteToWideChar(CP_UTF8, 0, webAddress, -1, nullptr, 0);
      std::wstring w(len > 0 ? len - 1 : 0, L'\0');
      if (len > 0)
         MultiByteToWideChar(CP_UTF8, 0, webAddress, -1, w.data(), len);
      return w;
   }();

   // ShellExecuteW's return value is technically an HINSTANCE for
   // historical reasons, but per Microsoft's documented contract, any
   // value greater than 32 indicates success.
   const auto result = reinterpret_cast<INT_PTR>(::ShellExecuteW(nullptr, L"open", wideAddress.c_str(), nullptr, nullptr, SW_SHOWNORMAL));

   return result > 32;
}

#ifndef TORQUE_SDL
namespace
{
   std::wstring utf8ToWideLocal(const char* utf8)
   {
      const int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
      std::wstring w(len > 0 ? len - 1 : 0, L'\0');
      if (len > 0)
         MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w.data(), len);
      return w;
   }
}

void Platform::openFolder(const char* path)
{
   char filePath[1024];
   Platform::makeFullPathName(path, filePath, sizeof(filePath));

   std::wstring wpath = utf8ToWideLocal(filePath);
   for (wchar_t& c : wpath) if (c == L'/') c = L'\\';

   ::ShellExecuteW(nullptr, L"explore", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void Platform::openFile(const char* path)
{
   char filePath[1024];
   Platform::makeFullPathName(path, filePath, sizeof(filePath));

   std::wstring wpath = utf8ToWideLocal(filePath);
   for (wchar_t& c : wpath) if (c == L'/') c = L'\\';

   ::ShellExecuteW(nullptr, L"open", wpath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}
#endif // !TORQUE_SDL

bool Platform::excludeOtherInstances(const char* mutexName)
{
#ifdef UNICODE
   UTF16 b[512];
   convertUTF8toUTF16((UTF8*)mutexName, b);
   gMutexHandle = CreateMutex(NULL, true, b);
#else
   gMutexHandle = CreateMutex(NULL, true, mutexName);
#endif
   if (!gMutexHandle)
      return false;

   if (GetLastError() == ERROR_ALREADY_EXISTS)
   {
      CloseHandle(gMutexHandle);
      gMutexHandle = NULL;
      return false;
   }

   return true;
}


void Platform::restartInstance()
{
   wchar_t exePath[MAX_PATH];
   if (::GetModuleFileNameW(nullptr, exePath, MAX_PATH) > 0)
   {
      STARTUPINFOW si{};
      si.cb = sizeof(si);
      PROCESS_INFORMATION pi{};

      if (::CreateProcessW(exePath, nullptr, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi))
      {
         ::CloseHandle(pi.hThread);
         ::CloseHandle(pi.hProcess);
      }
   }

   exit(0);
}

//-----------------------------------------------------------------------------
// FPU/SSE control word.
//
// IMPORTANT (this is the actual fix for a real crash, not a style choice):
// _MCW_PC (precision control) and _MCW_IC (infinity control) are BOTH
// documented by Microsoft as unsupported on x64 — passing either of them
// to _controlfp_s triggers the CRT's invalid-parameter handler, which by
// default calls __debugbreak(). That is exactly the crash seen in
// setMathControlStateKnown() (which passed _PC_64/_MCW_PC directly) — and
// setMathControlState() below was ALSO affected, since an earlier version
// of this file included _MCW_PC and _MCW_IC in its mask on the theory that
// a full round-trip restore should cover everything getMathControlState()
// read back. That reasoning doesn't hold on x64: the x87 FPU (which
// precision/infinity control historically applied to) isn't used at all
// on x64 — floating point goes through SSE2 instead, and SSE2 has no
// precision-control concept (precision is embedded per-instruction, not
// carried in a control word), so there is nothing there to restore.
//
// Fixed mask, used by both getters/setters below: _MCW_EM (exception
// mask) | _MCW_RC (rounding control) | _MCW_DN (denormal control) — the
// three control-word fields that are genuinely meaningful and supported
// on x64/SSE2.
//-----------------------------------------------------------------------------
namespace
{
   constexpr U32 kSupportedX64Mask = _MCW_EM | _MCW_RC | _MCW_DN;
}

U32 Platform::getMathControlState()
{
   U32 controlWord = 0;
   const errno_t error = _controlfp_s(&controlWord, 0, 0);
   return error ? 0 : controlWord;
}

void Platform::setMathControlState(U32 state)
{
   U32 controlWord = 0;
   _controlfp_s(&controlWord, state, kSupportedX64Mask);
}

void Platform::setMathControlStateKnown()
{
   // No x87 precision/infinity control exists to set on x64 (see comment
   // above) — this is a genuine no-op on this target, not a stub standing
   // in for missing functionality. If a "known" FPU state is ever needed
   // here in practice, it should set _MCW_RC/_MCW_EM/_MCW_DN to specific
   // known values instead of touching precision control at all.
}


//-----------------------------------------------------------------------------
F32 Platform::getRandom()
{
   // Simple, self-contained RNG with no dependency on any engine-level
   // random number generator instance. thread_local so this is safe to
   // call from multiple threads without external synchronization.
   static thread_local std::mt19937 engine{ std::random_device{}() };
   static thread_local std::uniform_real_distribution<F32> dist(0.0f, 1.0f);
   return dist(engine);
}

//-----------------------------------------------------------------------------
void Platform::init()
{
   Con::printf("Initializing platform...");

   Con::setVariable("$platform", "windows");

   // Mirror console output to the IDE's debug output window (VS "Output"
   // pane). See win32Console.cpp for the behavior this reproduces.
   Win32Console::init();

   Input::init();

   Con::printf("Done");
}

void Platform::shutdown()
{
   Win32Console::destroy();

   if (gMutexHandle)
      CloseHandle(gMutexHandle);

   Input::destroy();
}
