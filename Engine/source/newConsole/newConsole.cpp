#include "newConsole/newConsole.h"

#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _NEWCONSOLE_FILEIO_H_
#include "newConsole/fileIO.h"
#endif

namespace newConsole
{

   ExecResult exec(const char* filename)
   {
      ExecResult result;

      IScriptRuntime* runtime = ScriptHost::resolve(filename);
      if (!runtime)
      {
         result.success = false;
         result.errorMessage = "no registered runtime handles this file extension";
         return result;
      }

      String source;
      String readError;
      if (!readScriptFile(filename, source, &readError))
      {
         result.success = false;
         result.errorMessage = readError;
         return result;
      }

      DiagnosticSink diags;
      LoadResult loadResult = runtime->loadSource(filename, source.c_str(), diags);
      result.success = loadResult.success;

      if (!result.success)
      {
         // Concatenate all diagnostics; caller only sees ExecResult.
         for (const Diagnostic& d : diags.all())
         {
            if (!result.errorMessage.isEmpty())
               result.errorMessage += "\n";
            result.errorMessage += d.message;
         }
         if (result.errorMessage.isEmpty())
            result.errorMessage = "load failed with no reported diagnostics";
      }

      return result;
   }

   ScriptValue callFunction(RuntimeId runtimeId, SymbolId functionName, ScriptValueSpan args)
   {
      IScriptRuntime* runtime = ScriptHost::byId(runtimeId);
      if (!runtime)
         return ScriptValue::makeError("callFunction: no runtime registered under this id");

      return runtime->callFunction(functionName, args);
   }

   bool registerRuntime(IScriptRuntime* runtime)
   {
      if (!runtime)
         return false;

      ScriptHost::registerRuntime(runtime);
      return runtime->initialize(HostBindingRegistry::instance());
   }

   void shutdownAll()
   {
      ScriptHost::shutdownAll();
   }

   ObjectHandle registerObject(ScriptObject* object)
   {
      return ObjectRegistry::instance().registerObject(object);
   }

   ScriptObject* resolveObject(ObjectHandle handle)
   {
      return ObjectRegistry::instance().resolve(handle);
   }

   void unregisterObject(ObjectHandle handle)
   {
      ObjectRegistry::instance().unregisterObject(handle);
   }

   ScriptObject* construct(StringTableEntry className)
   {
      return ClassFactory::instance().construct(className);
   }

   const ScriptClassRep* findClass(StringTableEntry className)
   {
      return HostBindingRegistry::instance().find(className);
   }

   ScriptValue getIndex(const ScriptValue& base, const ScriptValue& index)
   {
      return scriptArrayGet(base, index);
   }

   bool setIndex(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value)
   {
      // Bool contract for direct C++ callers; the register VM path
      // (IInterpreterHost::setIndex) needs the vivified array back
      // instead, so it uses scriptArraySet's ScriptValue result directly.
      return scriptArraySet(base, index, value).kind() != ScriptValue::Kind::Error;
   }

} // namespace newConsole
