#include "newConsole/log.h"

#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include <cstdio>
#include <cstring>

namespace newConsole
{
   namespace
   {
      // Function-local statics for lazy, thread-safe-by-the-standard
      // initialization (C++11 magic statics - exactly-once, blocking
      // concurrent first-callers) - same pattern already used by
      // ObjectRegistry::instance() elsewhere in this layer, not a new
      // assumption introduced here.
      //
      // Guards mSinks only. Not held during dispatch (write() calls) -
      // see addLogSink's own comment on why that matters for threading.
      Mutex& sinkListMutex()
      {
         static Mutex sMutex;
         return sMutex;
      }

      Vector<LogSink*>& sinkList()
      {
         static Vector<LogSink*> sSinks;
         return sSinks;
      }

      // Fixed ceiling on a single formatted line, matching the legacy
      // engine's own buffer[8192] sizing - but STACK-LOCAL here, not
      // static/shared, which is the actual fix (see log.h's note on
      // this). A line vsnprintf'd past this length is truncated by
      // vsnprintf itself (it never overruns the buffer), not silently
      // corrupted - the same truncate-not-corrupt guarantee vsnprintf
      // always gives, just called out explicitly since a fixed ceiling
      // deserves that.
      constexpr size_t kLineBufferSize = 8192;

      void dispatch(LogLevel level, const char* line)
      {
         // Always write to stderr directly, regardless of how many
         // sinks (if any) are registered - see log.h's note on why this
         // is the unconditional baseline. One fwrite of the whole line
         // (not vfprintf directly against stderr) so two threads
         // logging concurrently each get their own fully-formatted
         // buffer written in one call - avoids interleaving a partial
         // line from one thread with a partial line from another at
         // the libc buffering layer.
         size_t len = std::strlen(line);
         std::fwrite(line, 1, len, stderr);
         std::fwrite("\n", 1, 1, stderr);
         std::fflush(stderr);

         // Copy the sink list under lock, then dispatch unlocked - see
         // addLogSink's comment for why dispatch must not happen while
         // holding sinkListMutex.
         Vector<LogSink*> snapshot;
         {
            MutexGuard guard(sinkListMutex());
            snapshot = sinkList(); // Vector copy-assign, not a reference
         }

         for (U32 i = 0; i < snapshot.size(); ++i)
            snapshot[i]->write(level, line);
      }

      void logImpl(LogLevel level, const char* format, va_list args)
      {
         char buffer[kLineBufferSize];
         std::vsnprintf(buffer, sizeof(buffer), format, args);
         dispatch(level, buffer);
      }
   } // namespace

   void addLogSink(LogSink* sink)
   {
      MutexGuard guard(sinkListMutex());
      sinkList().push_back(sink);
   }

   void removeLogSink(LogSink* sink)
   {
      MutexGuard guard(sinkListMutex());
      Vector<LogSink*>& sinks = sinkList();
      for (U32 i = 0; i < sinks.size(); ++i)
      {
         if (sinks[i] == sink)
         {
            sinks.erase(i);
            return;
         }
      }
   }

   void logInfoV(const char* format, va_list args) { logImpl(LogLevel::Info, format, args); }
   void logWarnV(const char* format, va_list args) { logImpl(LogLevel::Warn, format, args); }
   void logErrorV(const char* format, va_list args) { logImpl(LogLevel::Error, format, args); }
   void logV(LogLevel level, const char* format, va_list args) { logImpl(level, format, args); }

   void logInfo(const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      logImpl(LogLevel::Info, format, args);
      va_end(args);
   }

   void logWarn(const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      logImpl(LogLevel::Warn, format, args);
      va_end(args);
   }

   void logError(const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      logImpl(LogLevel::Error, format, args);
      va_end(args);
   }

   void log(LogLevel level, const char* format, ...)
   {
      va_list args;
      va_start(args, format);
      logImpl(level, format, args);
      va_end(args);
   }

} // namespace newConsole
