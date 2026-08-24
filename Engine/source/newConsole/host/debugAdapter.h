#ifndef _NEWCONSOLE_DEBUGADAPTER_H_
#define _NEWCONSOLE_DEBUGADAPTER_H_

#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif

namespace newConsole
{

   struct StackFrameInfo
   {
      StringTableEntry origin = nullptr;
      StringTableEntry functionName = nullptr;
      U32 line = 0;
   };

   /// Caller-facing debugging API. Read-only stack/local inspection only -
   /// no expression evaluation (see debugAdapter design notes).
   class IDebugAdapter
   {
   public:
      virtual ~IDebugAdapter() = default;

      virtual bool setBreakpoint(const char* origin, U32 line) = 0;
      virtual bool removeBreakpoint(const char* origin, U32 line) = 0;

      /// Blocks the calling thread until execution is suspended, or
      /// timeoutMs elapses. Returns false on timeout.
      virtual bool waitForSuspend(U32 timeoutMs) = 0;

      virtual bool isPaused() const = 0;

      /// Empty if not currently paused. Index 0 is the innermost frame.
      virtual Vector<StackFrameInfo> currentStack() = 0;

      /// Kind::Error if frameIndex is out of range or name isn't a
      /// known local in that frame at its current line.
      virtual ScriptValue getLocal(U32 frameIndex, StringTableEntry name) = 0;

      virtual void resume() = 0;
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_DEBUGADAPTER_H_
