//-----------------------------------------------------------------------------
// Con::addConsumer/removeConsumer are not documented as thread-safe, so
// init()/destroy() should only be called from the thread that owns
// console startup/shutdown (consistent with the rest of Platform::init/
// shutdown). OutputDebugStringA itself is safe to call from any thread.
//-----------------------------------------------------------------------------
#include "platform/win32/win32Console.h"
#include "console/console.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

namespace Con
{
   extern bool alwaysUseDebugOutput;
}

namespace Win32Console
{
   namespace
   {
      void consumer(U32 level, const char* line)
      {
#ifndef TORQUE_SHIPPING
         if (level == ConsoleLogEntry::Error || Con::alwaysUseDebugOutput)
         {
            // [rene, 04/05/2008] This is incorrect. Should do conversion
            // from UTF8 here. Skipping for the sake of speed. Not meant
            // to be seen by user anyway. (Preserved from the original
            // winConsole.cpp.)
            //
            // Build one buffer with the trailing newline and issue a
            // single OutputDebugStringA call rather than two — with two
            // calls, another thread's output could land in between them
            // and split this line across two debugger-output entries.
            char buffer[2048];
            const int len = _snprintf_s(buffer, sizeof(buffer), _TRUNCATE, "%s\n", line);
            if (len < 0)
            {
               // Line didn't fit; _TRUNCATE already null-terminated the
               // buffer, just make sure the newline survived the cut.
               buffer[sizeof(buffer) - 2] = '\n';
               buffer[sizeof(buffer) - 1] = '\0';
            }

            ::OutputDebugStringA(buffer);
         }
#else
         TORQUE_UNUSED(level);
         TORQUE_UNUSED(line);
#endif
      } 
   }

   void init()
   {
      Con::addConsumer(consumer);
   }

   void destroy()
   {
      Con::removeConsumer(consumer);
   }
}
