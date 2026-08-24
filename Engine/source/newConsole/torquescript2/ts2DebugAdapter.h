#ifndef _NEWCONSOLE_TS2_DEBUGADAPTER_H_
#define _NEWCONSOLE_TS2_DEBUGADAPTER_H_

#ifndef _NEWCONSOLE_DEBUGADAPTER_H_
#include "newConsole/host/debugAdapter.h"
#endif
#ifndef _NEWCONSOLE_TS2_INTERPRETER_H_
#include "newConsole/torquescript2/interpreter.h"
#endif
#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif
#ifndef _PLATFORM_THREAD_SEMAPHORE_H_
#include "platform/threads/semaphore.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      class TorqueScript2DebugAdapter : public IDebugAdapter, public IDebugHook
      {
      public:
         TorqueScript2DebugAdapter();

         // ---- IDebugAdapter ----
         bool setBreakpoint(const char* origin, U32 line) override;
         bool removeBreakpoint(const char* origin, U32 line) override;
         bool waitForSuspend(U32 timeoutMs) override;
         bool isPaused() const override;
         Vector<StackFrameInfo> currentStack() override;
         ScriptValue getLocal(U32 frameIndex, StringTableEntry name) override;
         void resume() override;

         // ---- IDebugHook ----
         bool shouldBreak(StringTableEntry origin, U32 line) override;
         void suspend(const Vector<CallFrame*>& stack) override;

      private:
         struct Breakpoint { StringTableEntry origin; U32 line; };

         mutable Mutex mLock;
         Vector<Breakpoint> mBreakpoints;

         bool mIsPaused = false;
         Vector<CallFrame*> mPausedStack;

         Semaphore mSuspended{ 0 };
         Semaphore mResume{ 0 };
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_DEBUGADAPTER_H_
