#ifndef _NEWCONSOLE_H_
#define _NEWCONSOLE_H_

#ifndef _NEWCONSOLE_SCRIPTHOST_H_
#include "newConsole/host/scriptHost.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#include "newConsole/host/scriptObject.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTCLASSMACROS_H_
#include "newConsole/host/scriptClassMacros.h"
#endif
#ifndef _NEWCONSOLE_OBJECTREGISTRY_H_
#include "newConsole/host/objectRegistry.h"
#endif
#ifndef _NEWCONSOLE_CLASSFACTORY_H_
#include "newConsole/host/classFactory.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTARRAY_H_
#include "newConsole/host/scriptArray.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTSTRUCTTRAITS_H_
#include "newConsole/host/scriptStructTraits.h"
#endif

namespace newConsole
{

   /// @file
   /// Top-level API surface for the engine. Declare classes, register/
   /// resolve objects, run files, call functions - all reachable here.
   /// host/ and torquescript2/ are implementation details, not meant to
   /// be included directly elsewhere.
   ///
   /// Include a runtime's own header (e.g. torquescript2Runtime.h) only
   /// to construct that runtime for registerRuntime() below.

   /// Result of exec(): did the file run, and if not, why.
   struct ExecResult
   {
      bool success = false;
      String errorMessage; ///< empty on success
   };

   /// Runs a script file by path; resolves the runtime by extension
   /// (see ScriptHost::resolve) and loads it through that runtime.
   ExecResult exec(const char* filename);

   /// Calls a named, already-loaded function by whichever runtime last
   /// published it. Prefer a specific runtime's own callFunction when
   /// known; this is for symbol-name-only lookups.
   ScriptValue callFunction(RuntimeId runtimeId, SymbolId functionName, ScriptValueSpan args = ScriptValueSpan());

   /// Registers @a runtime and initializes it against HostBindingRegistry.
   bool registerRuntime(IScriptRuntime* runtime);

   /// Shuts down every registered runtime.
   void shutdownAll();

   /// Registers a heap-allocated object for handle-based script access.
   ObjectHandle registerObject(ScriptObject* object);

   /// @return live object for @a handle, or nullptr if stale/unregistered.
   ScriptObject* resolveObject(ObjectHandle handle);

   /// Releases the reference registerObject() took for @a handle.
   void unregisterObject(ObjectHandle handle);

   /// Constructs @a className via its registered SCRIPT_CLASS constructor,
   /// or nullptr if none is registered. Not auto-registered - call
   /// registerObject() if it needs a handle.
   ScriptObject* construct(StringTableEntry className);

   /// @return reflection descriptor for @a className, or nullptr.
   const ScriptClassRep* findClass(StringTableEntry className);

   /// Indexed access into an array-kinded ScriptValue.
   ScriptValue getIndex(const ScriptValue& base, const ScriptValue& index);
   bool setIndex(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value);

} // namespace newConsole

#endif // !_NEWCONSOLE_H_
