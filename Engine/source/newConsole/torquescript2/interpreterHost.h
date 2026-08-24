#ifndef _NEWCONSOLE_TS2_INTERPRETERHOST_H_
#define _NEWCONSOLE_TS2_INTERPRETERHOST_H_

#ifndef _NEWCONSOLE_TS2_BYTECODE_H_
#include "newConsole/torquescript2/bytecode.h"
#endif

#include <memory>

namespace newConsole
{
   namespace ts2
   {

      /// Everything the VM core needs from the outside world. Deliberately
      /// narrow and not tied to HostBindingRegistry/ScriptClassRep - lets
      /// Interpreter be tested in isolation (a test can implement this
      /// with a plain map and canned responses, no reflection machinery).
      /// Production wiring to host/'s reflection layer is a separate class.
      ///
      /// @note Every method can fail (missing global, unresolvable
      ///   function, missing field) - reported via an Error-kind
      ///   ScriptValue, never by throwing or asserting. Malformed/hostile
      ///   script content must not crash the interpreter through this seam.
      class IInterpreterHost
      {
      public:
         virtual ~IInterpreterHost() = default;

         virtual ScriptValue getGlobal(StringTableEntry name) = 0;
         virtual void setGlobal(StringTableEntry name, const ScriptValue& value) = 0;

         virtual ScriptValue getField(const ScriptValue& object, StringTableEntry field, bool isInternal) = 0;
         virtual bool setField(const ScriptValue& object, StringTableEntry field, bool isInternal, const ScriptValue& value) = 0;

         virtual ScriptValue getIndex(const ScriptValue& base, const ScriptValue& index) = 0;

         /// Writes value into base[index], auto-vivifying base into a
         /// fresh array if it was Kind::Null (forwards to scriptArray.h's
         /// scriptArraySet).
         /// @return the (possibly newly-created/grown) base on success,
         ///   or Kind::Error on failure. Caller must always consult this
         ///   result, not just check success - the Interpreter writes it
         ///   back to the base register (see SetIndex in interpreter.cpp)
         ///   and the Emitter propagates it outward recursively (see
         ///   emitStoreTo's IndexAccess case), which is what makes
         ///   %grid[1][2] = 5 work from nothing.
         virtual ScriptValue setIndex(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value) = 0;

         /// Calls a free function by name (optionally namespace-qualified).
         /// Resolution is entirely up to the host - the VM core doesn't
         /// know or care where a call lands, only the returned ScriptValue.
         virtual ScriptValue callFunction(StringTableEntry namespaceNameOrNull, StringTableEntry functionName,
            ScriptValueSpan args) = 0;

         virtual ScriptValue callMethod(const ScriptValue& object, StringTableEntry methodName, ScriptValueSpan args) = 0;

         /// Constructs an object per one ObjectDeclTemplate. classNameValue/
         /// objectNameValue are already-resolved string ScriptValues - the
         /// Interpreter resolves static vs dynamic naming itself first.
         virtual ScriptValue newObject(const BytecodeUnit::ObjectDeclTemplate& tmpl,
            const ScriptValue* classNameValue,
            const ScriptValue* objectNameValue) = 0;

         // ---- iteration (foreach/foreach$) ----
         //
         // Opaque handles - the VM core never inspects one, only passes
         // it back to iterNext/iterEnd. A host can represent this however
         // it likes (a SimSet child-list index, a tokenizer cursor, ...).
         struct IteratorHandle { void* opaque = nullptr; };

         virtual IteratorHandle iterBegin(const ScriptValue& collection, bool isStringForm) = 0;
         /// @return true and writes @a outValue if another element was
         ///   available; false if exhausted (outValue untouched).
         virtual bool iterNext(IteratorHandle handle, ScriptValue& outValue) = 0;
         virtual void iterEnd(IteratorHandle handle) = 0;

         /// Resolves a function name to a BytecodeUnit for torquescript2-
         /// to-torquescript2 calls specifically, distinct from callFunction
         /// (which covers every call target, including non-ts2 ones). The
         /// Interpreter tries this first, falling back to callFunction if
         /// null - lets ts2-to-ts2 calls run the fast register-VM path
         /// while everything else has one uniform fallback.
         ///
         /// @note Returns shared_ptr - the host owns "which version is
         ///   currently published" (hot-reload), and shared ownership
         ///   lets a CallFrame pin the version it started with even if
         ///   the host publishes a new one mid-call.
         virtual std::shared_ptr<const BytecodeUnit> resolveFunctionUnit(StringTableEntry namespaceNameOrNull, StringTableEntry functionName) = 0;
         virtual std::shared_ptr<const BytecodeUnit> resolveMethodUnit(const ScriptValue& object, StringTableEntry methodName) = 0;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_INTERPRETERHOST_H_
