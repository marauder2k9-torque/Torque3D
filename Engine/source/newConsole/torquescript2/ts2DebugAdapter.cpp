#include "newConsole/torquescript2/ts2DebugAdapter.h"

namespace newConsole
{
   namespace ts2
   {

      TorqueScript2DebugAdapter::TorqueScript2DebugAdapter()
      {
      }

      bool TorqueScript2DebugAdapter::setBreakpoint(const char* origin, U32 line)
      {
         StringTableEntry internedOrigin = StringTable->insert(origin);
         MutexGuard guard(mLock);
         for (U32 i = 0; i < mBreakpoints.size(); ++i)
            if (mBreakpoints[i].origin == internedOrigin && mBreakpoints[i].line == line)
               return true;
         mBreakpoints.push_back(Breakpoint{ internedOrigin, line });
         return true;
      }

      bool TorqueScript2DebugAdapter::removeBreakpoint(const char* origin, U32 line)
      {
         StringTableEntry internedOrigin = StringTable->insert(origin);
         MutexGuard guard(mLock);
         for (U32 i = 0; i < mBreakpoints.size(); ++i)
         {
            if (mBreakpoints[i].origin == internedOrigin && mBreakpoints[i].line == line)
            {
               mBreakpoints.erase(i);
               return true;
            }
         }
         return false;
      }

      bool TorqueScript2DebugAdapter::waitForSuspend(U32 timeoutMs)
      {
         return mSuspended.acquire(true, static_cast<S32>(timeoutMs));
      }

      bool TorqueScript2DebugAdapter::isPaused() const
      {
         MutexGuard guard(mLock);
         return mIsPaused;
      }

      Vector<StackFrameInfo> TorqueScript2DebugAdapter::currentStack()
      {
         Vector<StackFrameInfo> result;
         MutexGuard guard(mLock);
         if (!mIsPaused)
            return result;

         for (S32 i = static_cast<S32>(mPausedStack.size()) - 1; i >= 0; --i)
         {
            const CallFrame* frame = mPausedStack[i];
            StackFrameInfo info;
            info.origin = frame->unit->origin;
            info.functionName = frame->unit->name;
            info.line = (frame->ip < frame->unit->lineTable.size()) ? frame->unit->lineTable[frame->ip] : 0;
            result.push_back(info);
         }
         return result;
      }

      ScriptValue TorqueScript2DebugAdapter::getLocal(U32 frameIndex, StringTableEntry name)
      {
         MutexGuard guard(mLock);
         if (!mIsPaused)
            return ScriptValue::makeError("not paused");

         S32 index = static_cast<S32>(mPausedStack.size()) - 1 - static_cast<S32>(frameIndex);
         if (index < 0 || index >= static_cast<S32>(mPausedStack.size()))
            return ScriptValue::makeError("frame index out of range");

         const CallFrame* frame = mPausedStack[index];
         const BytecodeUnit& unit = *frame->unit;

         for (U32 i = 0; i < unit.localDebugInfo.size(); ++i)
         {
            const BytecodeUnit::LocalDebugInfo& info = unit.localDebugInfo[i];
            if (info.name != name)
               continue;
            if (frame->ip < info.firstValidInstruction || frame->ip > info.lastValidInstruction)
               continue;
            return frame->registers[info.reg];
         }
         return ScriptValue::makeError("unknown local");
      }

      void TorqueScript2DebugAdapter::resume()
      {
         mResume.release();
      }

      bool TorqueScript2DebugAdapter::shouldBreak(StringTableEntry origin, U32 line)
      {
         MutexGuard guard(mLock);
         for (U32 i = 0; i < mBreakpoints.size(); ++i)
            if (mBreakpoints[i].origin == origin && mBreakpoints[i].line == line)
               return true;
         return false;
      }

      void TorqueScript2DebugAdapter::suspend(const Vector<CallFrame*>& stack)
      {
         {
            MutexGuard guard(mLock);
            mPausedStack = stack;
            mIsPaused = true;
         }
         mSuspended.release();
         mResume.acquire();
         {
            MutexGuard guard(mLock);
            mIsPaused = false;
            mPausedStack.clear();
         }
      }

   } // namespace ts2
} // namespace newConsole
