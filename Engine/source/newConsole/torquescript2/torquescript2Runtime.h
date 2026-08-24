#ifndef _NEWCONSOLE_TS2_RUNTIME_H_
#define _NEWCONSOLE_TS2_RUNTIME_H_

#ifndef _NEWCONSOLE_SCRIPTHOST_H_
#include "newConsole/host/scriptHost.h"
#endif
#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _NEWCONSOLE_CLASSFACTORY_H_
#include "newConsole/host/classFactory.h"
#endif
#ifndef _NEWCONSOLE_OBJECTREGISTRY_H_
#include "newConsole/host/objectRegistry.h"
#endif
#ifndef _NEWCONSOLE_TS2_INTERPRETER_H_
#include "newConsole/torquescript2/interpreter.h"
#endif
#ifndef _NEWCONSOLE_TS2_FUNCTIONTABLE_H_
#include "newConsole/torquescript2/functionTable.h"
#endif
#ifndef _NEWCONSOLE_TS2_DEBUGADAPTER_H_
#include "newConsole/torquescript2/ts2DebugAdapter.h"
#endif
#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif

#include <unordered_map>

namespace newConsole
{
   namespace ts2
   {

      /// The real torquescript2 runtime - implements both IScriptRuntime
      /// (ScriptHost's dispatch interface) and IInterpreterHost (the VM
      /// core's callback interface). One class implements both since
      /// they're the same thing at this layer - unlike Interpreter/
      /// IInterpreterHost's split, which exists so the VM core can be
      /// tested against a fake host.
      class TorqueScript2Runtime : public IScriptRuntime, public IInterpreterHost
      {
      public:
         TorqueScript2Runtime();

         // ---- IScriptRuntime ----
         RuntimeId id() const override;
         bool canHandle(const char* filename) const override;
         bool initialize(const HostBindingRegistry& bindings) override;
         void shutdown() override;
         LoadResult loadSource(const char* originName, const char* source, DiagnosticSink& diags) override;
         ScriptValue callFunction(SymbolId name, ScriptValueSpan args) override;
         ReloadResult reloadSource(const char* originName, const char* source, DiagnosticSink& diags) override;

         // ---- IInterpreterHost ----
         ScriptValue getGlobal(StringTableEntry name) override;
         void setGlobal(StringTableEntry name, const ScriptValue& value) override;
         ScriptValue getField(const ScriptValue& object, StringTableEntry field, bool isInternal) override;
         bool setField(const ScriptValue& object, StringTableEntry field, bool isInternal, const ScriptValue& value) override;
         ScriptValue getIndex(const ScriptValue& base, const ScriptValue& index) override;
         ScriptValue setIndex(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value) override;
         ScriptValue callFunction(StringTableEntry namespaceNameOrNull, StringTableEntry functionName, ScriptValueSpan args) override;
         ScriptValue callMethod(const ScriptValue& object, StringTableEntry methodName, ScriptValueSpan args) override;
         ScriptValue newObject(const BytecodeUnit::ObjectDeclTemplate& tmpl,
            const ScriptValue* classNameOverride,
            const ScriptValue* objectNameOverride) override;
         IteratorHandle iterBegin(const ScriptValue& collection, bool isStringForm) override;
         bool iterNext(IteratorHandle handle, ScriptValue& outValue) override;
         void iterEnd(IteratorHandle handle) override;
         std::shared_ptr<const BytecodeUnit> resolveFunctionUnit(StringTableEntry namespaceNameOrNull, StringTableEntry functionName) override;
         std::shared_ptr<const BytecodeUnit> resolveMethodUnit(const ScriptValue& object, StringTableEntry methodName) override;

         IDebugAdapter* getDebugAdapter() override { return &mDebugAdapter; }

      private:
         /// Backing state for a foreach$ (string-tokenized) iterator.
         /// Object-collection foreach has no backing struct yet (see iterBegin).
         struct StringTokenIterator
         {
            Vector<String> tokens;
            U32 index = 0;
         };

         /// Compiles every function in @a source and publishes each into
         /// mFunctions, optionally running top-level statements once.
         /// Shared by loadSource/reloadSource - see reloadSource for why
         /// it never re-runs top-level statements.
         bool compileAndPublish(const char* originName, const char* source, DiagnosticSink& diags,
            bool runTopLevelStatements, ReloadResult* outReloadResult);

         /// Resolves the ScriptClassRep for an object value via the
         /// registered ScriptObject and HostBindingRegistry.
         const ScriptClassRep* classRepForObject(const ScriptValue& object) const;

         HostBindingRegistry* mBindings = nullptr;
         FunctionTable mFunctions;
         Interpreter mInterpreter;
         TorqueScript2DebugAdapter mDebugAdapter;

         mutable Mutex mGlobalsLock;
         std::unordered_map<StringTableEntry, ScriptValue> mGlobals;

         /// Name -> handle for objects declared with a non-empty name via
         /// singleton/datablock (find-or-create) or new (still recorded
         /// if named). Separate from ObjectRegistry, which is
         /// id/generation-based with no concept of script-visible names -
         /// name lookup is a runtime-level concern layered on top.
         mutable Mutex mNamedObjectsLock;
         std::unordered_map<StringTableEntry, ScriptValue> mNamedObjects;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_RUNTIME_H_
