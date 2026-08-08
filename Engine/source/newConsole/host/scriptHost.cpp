#include "newConsole/host/scriptHost.h"

#include "core/strings/stringFunctions.h"

namespace newConsole
{

   bool DiagnosticSink::hasErrors() const
   {
      for (U32 i = 0; i < mDiagnostics.size(); ++i)
      {
         if (mDiagnostics[i].isError)
            return true;
      }
      return false;
   }

   bool ReloadResult::anyApplied() const
   {
      for (U32 i = 0; i < symbols.size(); ++i)
      {
         if (symbols[i].applied)
            return true;
      }
      return false;
   }

   Vector<IScriptRuntime*>& ScriptHost::runtimes()
   {
      static Vector<IScriptRuntime*> sRuntimes;
      return sRuntimes;
   }

   void ScriptHost::registerRuntime(IScriptRuntime* runtime)
   {
      AssertFatal(runtime != nullptr, "ScriptHost::registerRuntime - null runtime");
      runtimes().push_back(runtime);
   }

   void ScriptHost::shutdownAll()
   {
      Vector<IScriptRuntime*>& all = runtimes();
      for (U32 i = 0; i < all.size(); ++i)
      {
         all[i]->shutdown();
         delete all[i];
      }
      all.clear();
   }

   IScriptRuntime* ScriptHost::resolve(const char* filename)
   {
      Vector<IScriptRuntime*>& all = runtimes();
      for (U32 i = 0; i < all.size(); ++i)
      {
         if (all[i]->canHandle(filename))
            return all[i];
      }
      return nullptr;
   }

   IScriptRuntime* ScriptHost::byId(RuntimeId id)
   {
      Vector<IScriptRuntime*>& all = runtimes();
      for (U32 i = 0; i < all.size(); ++i)
      {
         if (all[i]->id() == id)
            return all[i];
      }
      return nullptr;
   }

   ReloadResult ScriptHost::broadcastReload(const char* originName, const char* source)
   {
      IScriptRuntime* runtime = resolve(originName);
      if (!runtime)
      {
         ReloadResult result;
         ReloadedSymbol symbol;
         symbol.name = StringTable->insert(originName);
         symbol.applied = false;
         symbol.reason = "no runtime registered for this file type";
         result.symbols.push_back(symbol);
         return result;
      }

      DiagnosticSink diags;
      return runtime->reloadSource(originName, source, diags);
   }

} // namespace newConsole
