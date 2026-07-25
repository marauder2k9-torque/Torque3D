//-----------------------------------------------------------------------------
// win32Exec.cpp — Windows implementation of the shellExecute console
// function: launches an external program on a background Thread via
// ShellExecuteEx, and polls WaitForSingleObject without blocking the sim
// until it completes, then posts an ExecuteCleanupEvent so
// "onExecuteDone" fires on the main thread.
//
// Rewritten from winExec.cpp. Real, working functional logic (fork a
// background Thread, ShellExecuteEx, poll completion, post a SimEvent
// back) is unchanged — what's different from the original:
//   - No longer includes platformWin32/platformWin32.h. That header was
//     only ever used here for its #include (no Win32PlatState/winState
//     member was actually touched anywhere in this file), matching the
//     same decoupling already applied throughout this platform-layer
//     rewrite (see win32MsgBox.cpp/win32Main.cpp's own header comments
//     for the same change and rationale).
//   - backslash()/forwardslash() (from platformWin32.h) are replaced with
//     a small local std::wstring helper, since that header is no longer
//     included. UTF8->UTF16 conversion uses MultiByteToWideChar directly,
//     matching win32ProcessControl.cpp's own convention (this file no
//     longer branches on #ifdef UNICODE/ANSI the way the original did —
//     every rewritten win32 file in this platform layer now targets the
//     wide Win32 API unconditionally).
//   - TempAlloc<TCHAR>/TempAlloc<WCHAR> (a util/tempAlloc.h helper not
//     otherwise used elsewhere in this rewritten platform layer) is
//     replaced with plain std::wstring, avoiding that extra dependency.
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/engineAPI.h"
#include "sim/simBase.h"
#include "platform/threads/thread.h"
#include "core/util/safeDelete.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#include <string>

namespace
{
   std::wstring utf8ToWide(const char* utf8)
   {
      if (!utf8 || !*utf8)
         return std::wstring();
      const int len = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, nullptr, 0);
      std::wstring w(len > 0 ? len - 1 : 0, L'\0');
      if (len > 0)
         MultiByteToWideChar(CP_UTF8, 0, utf8, -1, w.data(), len);
      return w;
   }

   void toBackslashes(std::wstring& s)
   {
      for (wchar_t& c : s)
         if (c == L'/') c = L'\\';
   }
}

//-----------------------------------------------------------------------------
// Thread for executing in
//-----------------------------------------------------------------------------

class ExecuteThread : public Thread
{
   // [tom, 12/14/2006] mProcess is only used in the constructor before the thread
   // is started and in the thread itself so we should be OK without a mutex.
   HANDLE mProcess;

public:
   ExecuteThread(const char *executable, const char *args = nullptr, const char *directory = nullptr);

   void run(void *arg = 0) override;
};

//-----------------------------------------------------------------------------
// Event for cleanup
//-----------------------------------------------------------------------------

class ExecuteCleanupEvent : public SimEvent
{
   ExecuteThread *mThread;
   bool mOK;

public:
   ExecuteCleanupEvent(ExecuteThread *thread, bool ok)
   {
      mThread = thread;
      mOK = ok;
   }

   void process(SimObject *object) override
   {
      if( Con::isFunction( "onExecuteDone" ) )
         Con::executef( "onExecuteDone", Con::getIntArg( mOK ) );
      SAFE_DELETE(mThread);
   }
};

//-----------------------------------------------------------------------------

ExecuteThread::ExecuteThread(const char *executable, const char *args /* = nullptr */, const char *directory /* = nullptr */)
   : Thread(0, NULL, false), mProcess(nullptr)
{
   SHELLEXECUTEINFOW shl;
   ZeroMemory(&shl, sizeof(shl));

   shl.cbSize = sizeof(shl);
   shl.fMask = SEE_MASK_NOCLOSEPROCESS;

   char exeBuf[1024];
   Platform::makeFullPathName(executable, exeBuf, sizeof(exeBuf));

   std::wstring exe = utf8ToWide(exeBuf);
   std::wstring wargs = utf8ToWide(args);
   std::wstring wdir = utf8ToWide(directory);

   toBackslashes(exe);
   toBackslashes(wdir);

   shl.lpVerb = L"open";
   shl.lpFile = exe.c_str();
   shl.lpParameters = wargs.empty() ? nullptr : wargs.c_str();
   shl.lpDirectory = wdir.empty() ? nullptr : wdir.c_str();
   shl.nShow = SW_SHOWNORMAL;

   if (::ShellExecuteExW(&shl) && shl.hProcess)
   {
      mProcess = shl.hProcess;
      start();
   }
}

void ExecuteThread::run(void *arg /* = 0 */)
{
   if (mProcess == nullptr)
      return;

   DWORD wait = WAIT_OBJECT_0 - 1; // i.e., not WAIT_OBJECT_0
   while (!checkForStop() && (wait = ::WaitForSingleObject(mProcess, 200)) != WAIT_OBJECT_0)
      ;

   Sim::postEvent(Sim::getRootGroup(), new ExecuteCleanupEvent(this, wait == WAIT_OBJECT_0), -1);
}

//-----------------------------------------------------------------------------
// Console Functions
//-----------------------------------------------------------------------------

DefineEngineFunction( shellExecute, bool, (const char * executable, const char * args, const char * directory), ("", ""), "(string executable, string args, string directory)"
                "@brief Launches an outside executable or batch file\n\n"
                "@param executable Name of the executable or batch file\n"
                "@param args Optional list of arguments, in string format, to pass to the executable\n"
                "@param directory Optional string containing path to output or shell\n"
                "@return true if executed, false if not\n"
                "@ingroup Platform")
{
   ExecuteThread *et = new ExecuteThread( executable, args, directory );
   if(! et->isAlive())
   {
      delete et;
      return false;
   }

   return true;
}
