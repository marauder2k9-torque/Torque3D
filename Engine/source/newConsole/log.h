// =============================================================================
// DESIGN DRAFT - newConsole/log.h
//
// newConsole/'s own logging facility. Goals, directly from conversation:
//   - No dependency on Con:: / the legacy console system at all - confirmed
//     by reading platformAssert.cpp that AssertFatal/AssertWarn already
//     transitively call Con::warnf/Con::errorf, which is exactly the kind
//     of "platform noise" newConsole/ is meant to be free of. This file
//     does not include console.h, platformAssert.h's Con:: calls, or
//     anything from sim/.
//   - Any class can subscribe to log output, mirroring Con::addConsumer's
//     good idea (a plain callback list) - but WITHOUT the legacy system's
//     memory problem.
//
// The legacy memory problem, confirmed directly in console.cpp's _printf
// (not inferred - the engine's own comment says it outright):
//     "this is equivalent to a memory leak, turn it off in ship build"
//   Every logged line gets copied into a chunked allocator and pushed into
//   an ever-growing Vector<ConsoleLogEntry>, unconditionally, for the
//   entire process lifetime, unless a build disables it wholesale
//   (TORQUE_SHIPPING) or logBufferEnabled is turned off (which also gives
//   up the "dump full history into a newly-opened log file" feature this
//   buffer exists for). No cap, no ring buffer, no eviction - the design
//   itself is unbounded, not just under-tuned.
//
// This design's answer: LogSink is the extension point (a class any code
// can subclass and register, same spirit as a ConsumerCallback), and NONE
// of them retain history by default. A sink that wants a log file writes
// straight to a FileStream/fopen'd file per line, the same way
// ConsoleLogger already does today (that part of the legacy design was
// fine - it's not what caused the leak). A sink that wants an in-memory
// scrollback (e.g. an in-game console UI) is free to keep one, but that's
// an opt-in choice made by that ONE sink, not a global, always-on,
// unbounded buffer every process pays for whether anything asked for
// history or not.
// =============================================================================

#ifndef _NEWCONSOLE_LOG_H_
#define _NEWCONSOLE_LOG_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif

#include <cstdarg>

namespace newConsole
{

   /// Severity of a single log line. Kept smaller and flatter than legacy
   /// ConsoleLogEntry::Level/Type's two-axis split (severity x subsystem) -
   /// a sink that cares about subsystem origin can filter on the message
   /// text/prefix convention its callers establish; baking a fixed
   /// subsystem enum (General/Script/GUI/Network/...) into the core log
   /// type couples this header to knowledge of every subsystem that will
   /// ever log anything, which is exactly the kind of coupling this
   /// layer is meant to avoid.
   enum class LogLevel : U32
   {
      Info = 0,
      Warn,
      Error,
   };

   /// Extension point for anything that wants to observe log output -
   /// same role as a legacy ConsumerCallback, but an interface instead of
   /// a bare function pointer so a sink can carry its own state (an open
   /// file handle, a network connection, a ring buffer it owns) without
   /// resorting to a static/global.
   ///
   /// @note A LogSink that wants history keeps it ITSELF, sized however
   ///   that sink needs (a bounded ring buffer, a file, a network
   ///   stream) - Log itself never retains lines after handing them to
   ///   every registered sink. This is the deliberate fix for the legacy
   ///   system's unconditional, unbounded, always-on history buffer.
   class LogSink
   {
   public:
      virtual ~LogSink() = default;

      /// Called for every logged line, already fully formatted (varargs
      /// already resolved) - same as a ConsumerCallback receiving the
      /// final buffer, not the raw format string. Called synchronously,
      /// on whatever thread logged the line - see Log's own threading
      /// note below for what this means for a sink's own implementation.
      virtual void write(LogLevel level, const char* line) = 0;
   };

   /// Registers/unregisters a LogSink. Thread-safe - sinks may be added
   /// or removed from any thread, and write() may be called concurrently
   /// from multiple threads; a sink's own write() must itself be safe to
   /// call concurrently if more than one thread logs (Log does not
   /// serialize calls to write() across sinks - each sink guards its own
   /// state, same expectation as a legacy ConsumerCallback needing to be
   /// reentrant-safe itself).
   ///
   /// @note Dispatch to sinks happens OUTSIDE the sink-list lock (see
   ///   log.cpp) - the list is copied under lock, then iterated
   ///   unlocked. Two reasons, both load-bearing for a genuinely
   ///   threadable design, not just "doesn't crash":
   ///     1. A slow sink (writes to disk, blocks on a socket) must not
   ///        serialize every OTHER thread trying to log anything, which
   ///        holding the lock across every sink's write() would do.
   ///     2. A sink is allowed to call addLogSink/removeLogSink from
   ///        within its own write() (e.g. unregistering itself on a
   ///        fatal error) without deadlocking against a non-reentrant
   ///        lock still held by the dispatch loop that called it.
   ///   The tradeoff: a sink added/removed during a dispatch already in
   ///   flight may or may not see that particular line, depending on
   ///   timing - accepted as the right side of that tradeoff, since the
   ///   alternative (holding the lock through every sink call) risks
   ///   deadlock and cross-thread stalls for a guarantee (every sink
   ///   sees every line with no possible race) that a logging facility
   ///   doesn't actually need.
   void addLogSink(LogSink* sink);
   void removeLogSink(LogSink* sink);

   /// Formats and dispatches a line to every registered sink. Direct
   /// vfprintf(stderr, ...)-based formatting internally - no dependency
   /// on dPrintf/stringFunctions.h, staying self-contained rather than
   /// pulling in even that thin a dependency for one function.
   ///
   /// @note Thread-safe in the sense that matters for a genuinely
   ///   threadable engine: formats into a STACK-LOCAL buffer, never a
   ///   shared/static one - two threads calling logInfo/logWarn/logError
   ///   concurrently never touch the same memory while formatting. This
   ///   directly avoids the legacy _printf's static char buffer[8192],
   ///   which is a plain data race the instant two threads call it at
   ///   once (both would format into the same memory) - not merely a
   ///   memory-shape inefficiency, an actual correctness bug this
   ///   design does not inherit.
   ///
   /// @note These three ALWAYS write to stderr directly, in addition to
   ///   dispatching to sinks - stderr is the unconditional baseline
   ///   output (visible with zero setup, e.g. before any sink has
   ///   registered, or in a headless/test process with no sinks at
   ///   all), while sinks are for anything that wants MORE than that
   ///   (a file, a network console, an in-game overlay). The stderr
   ///   write itself is also safe under concurrent calls - see log.cpp;
   ///   a single fwrite of a fully-formatted line is atomic with respect
   ///   to interleaving with another thread's fwrite at the granularity
   ///   that matters here (lines won't interleave character-by-character,
   ///   though two threads' lines can still appear in either order -
   ///   ordering across threads was never a guarantee this API makes).
   void logInfo(const char* format, ...);
   void logWarn(const char* format, ...);
   void logError(const char* format, ...);

   /// Single entry point taking level as a parameter, for callers that
   /// don't know their severity until runtime (e.g. forwarding another
   /// system's own severity value). Equivalent to calling logInfo/
   /// logWarn/logError directly.
   void log(LogLevel level, const char* format, ...);

   /// va_list forms, for callers that already have one (e.g. a wrapper
   /// that itself takes ...).
   void logInfoV(const char* format, va_list args);
   void logWarnV(const char* format, va_list args);
   void logErrorV(const char* format, va_list args);
   void logV(LogLevel level, const char* format, va_list args);

} // namespace newConsole

#endif // !_NEWCONSOLE_LOG_H_
