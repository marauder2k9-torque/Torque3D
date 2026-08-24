#ifndef _NEWCONSOLE_TS2_INTERPRETER_H_
#define _NEWCONSOLE_TS2_INTERPRETER_H_

#ifndef _NEWCONSOLE_TS2_INTERPRETERHOST_H_
#include "newConsole/torquescript2/interpreterHost.h"
#endif

#include <memory>

namespace newConsole
{
   namespace ts2
   {

      /// Runtime error - unresolved call, argument-count mismatch on a ts2-to-
      /// ts2 call, an internal VM invariant violation. Distinct from
      /// ScriptValue::Kind::Error (a legitimate script return value); this
      /// means execution itself could not continue.
      struct InterpreterError
      {
         String message;
         U32 line = 0;
      };

      /// One active call's state. Holds a shared_ptr to its BytecodeUnit, not
      /// a raw pointer - this is the hot-reload safety mechanism: a frame pins
      /// the exact version it started executing, so swapping the published
      /// version elsewhere never invalidates a frame already running the old
      /// one. The old unit stays alive as long as any CallFrame references it.
      struct CallFrame
      {
         std::shared_ptr<const BytecodeUnit> unit;
         Vector<ScriptValue> registers;
         U32 ip = 0;

         /// Arguments this call actually supplied - not unit->paramCount
         /// (declared arity). GetArgCount reads this, which is what makes
         /// default-parameter lowering in compileFunction correct.
         U32 argCount = 0;

         /// Last line this frame suspended at, so a single source line
         /// spanning several instructions only breaks once, not once per
         /// instruction. Reset once ip moves to a different line.
         U32 lastBreakLine = ~0u;

         explicit CallFrame(std::shared_ptr<const BytecodeUnit> u) : unit(std::move(u))
         {
            registers.setSize(unit->registerCount);
         }
      };

      /// Interpreter-facing debug hook. Null when no debugger is attached
      /// (the common case) - checked once per instruction in executeFrame.
      class IDebugHook
      {
      public:
         virtual ~IDebugHook() = default;
         virtual bool shouldBreak(StringTableEntry origin, U32 line) = 0;
         virtual void suspend(const Vector<CallFrame*>& stack) = 0;
      };

      /// The register-VM core. Executes one BytecodeUnit at a time, pushing a
      /// CallFrame per nested call and popping on return - a real call stack,
      /// not a flat register file reused across calls.
      ///
      /// @note No dependency on HostBindingRegistry, ScriptObject, or the
      ///   reflection layer - everything goes through IInterpreterHost, which
      ///   is what makes this loop unit-testable against a fake host.
      class Interpreter
      {
      public:
         explicit Interpreter(IInterpreterHost& host);

         /// Runs @a unit from the start with @a args bound to its parameter
         /// registers, to completion (Return/ReturnNull or a fatal
         /// InterpreterError). Nested calls the bytecode makes are handled
         /// internally via callInternal, not by recursive run() calls.
         ScriptValue run(std::shared_ptr<const BytecodeUnit> unit, ScriptValueSpan args);

         /// True after a run() that terminated via InterpreterError rather
         /// than a normal Return - check lastError() for details.
         bool hadFatalError() const { return mHadFatalError; }
         const InterpreterError& lastError() const { return mLastError; }

         void setDebugHook(IDebugHook* hook) { mDebugHook = hook; }

         /// Max call depth before reporting a fatal error instead of
         /// recursing further - guards against runaway script recursion
         /// exhausting the native C++ stack (each nested call is a real C++
         /// recursion in this VM's dispatch loop).
         static constexpr U32 kMaxCallDepth = 256;

      private:
         /// Executes @a frame from its current ip until return, recursing
         /// into a fresh executeFrame for nested calls - kMaxCallDepth bounds
         /// exactly this recursion.
         ScriptValue executeFrame(CallFrame& frame);

         /// Shared by Call/CallNamespaced/MethodCall: resolves the target
         /// (ts2 function via resolveFunctionUnit/resolveMethodUnit, or an
         /// external call via callFunction/callMethod as fallback - see
         /// IInterpreterHost) and either recurses into executeFrame or calls
         /// straight through to the host.
         ScriptValue callInternal(StringTableEntry namespaceNameOrNull, StringTableEntry functionName, ScriptValueSpan args);
         ScriptValue methodCallInternal(const ScriptValue& object, StringTableEntry methodName, ScriptValueSpan args);

         void reportFatal(U32 line, const char* message);

         IInterpreterHost& mHost;
         U32 mCallDepth = 0;
         bool mHadFatalError = false;
         InterpreterError mLastError;

         IDebugHook* mDebugHook = nullptr;
         Vector<CallFrame*> mCallStack;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_INTERPRETER_H_
