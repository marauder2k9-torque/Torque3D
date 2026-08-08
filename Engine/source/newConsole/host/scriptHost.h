#ifndef _NEWCONSOLE_SCRIPTHOST_H_
#define _NEWCONSOLE_SCRIPTHOST_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace newConsole
{

   class HostBindingRegistry;
   class IDebugAdapter;

   typedef StringTableEntry RuntimeId;
   typedef StringTableEntry SymbolId;

   /// One parse/compile diagnostic, independent of source language.
   struct Diagnostic
   {
      StringTableEntry origin;
      U32 line = 0;
      U32 column = 0;
      String message;
      bool isError = true;
   };

   /// Collects diagnostics during a load or reload call.
   class DiagnosticSink
   {
   public:
      void report(Diagnostic d) { mDiagnostics.push_back(std::move(d)); }
      bool hasErrors() const;
      const Vector<Diagnostic>& all() const { return mDiagnostics; }

   private:
      Vector<Diagnostic> mDiagnostics;
   };

   /// Result of IScriptRuntime::loadSource.
   struct LoadResult
   {
      bool success = false;
   };

   /// Per-symbol outcome of a reload attempt.
   struct ReloadedSymbol
   {
      SymbolId name;
      bool applied = false;
      String reason; ///< populated when applied == false
   };

   /// Result of IScriptRuntime::reloadSource.
   struct ReloadResult
   {
      Vector<ReloadedSymbol> symbols;
      bool anyApplied() const;
   };

   /// One embedded language runtime (Lua, C#, torquescript2, ...).
   ///
   /// @note The engine never calls a language's native API directly. All
   ///   interaction goes through this interface.
   class IScriptRuntime
   {
   public:
      virtual ~IScriptRuntime() = default;

      virtual RuntimeId id() const = 0;
      virtual bool canHandle(const char* filename) const = 0;

      /// Bring the runtime up. Called once, after every SCRIPT_CLASS has
      /// registered with @a bindings.
      virtual bool initialize(const HostBindingRegistry& bindings) = 0;
      virtual void shutdown() = 0;

      virtual LoadResult loadSource(const char* originName, const char* source, DiagnosticSink& diags) = 0;

      /// Call a global or namespace-qualified function by symbol.
      virtual ScriptValue callFunction(SymbolId name, ScriptValueSpan args) = 0;

      /// Best-effort reload of a single source unit already loaded.
      /// @return per-symbol success; never partially corrupts a running session.
      virtual ReloadResult reloadSource(const char* originName, const char* source, DiagnosticSink& diags) = 0;

      virtual IDebugAdapter* getDebugAdapter() { return nullptr; }
   };

   /// Registry and dispatch point for every IScriptRuntime.
   ///
   /// @note Owns every runtime registered with it (deleted in shutdownAll).
   ///   Vector<T> here is the engine's own container, which grows elements
   ///   by copy - not compatible with a move-only smart pointer - so
   ///   ownership is plain, explicit, and singular: register a
   ///   heap-allocated runtime, ScriptHost deletes it exactly once on
   ///   shutdown, never before.
   class ScriptHost
   {
   public:
      static void registerRuntime(IScriptRuntime* runtime);
      static void shutdownAll();

      static IScriptRuntime* resolve(const char* filename);
      static IScriptRuntime* byId(RuntimeId id);

      static ReloadResult broadcastReload(const char* originName, const char* source);

   private:
      static Vector<IScriptRuntime*>& runtimes();
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTHOST_H_
