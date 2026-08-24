#include "newConsole/torquescript2/torquescript2Runtime.h"

#ifndef _NEWCONSOLE_TS2_LEXER_H_
#include "newConsole/torquescript2/lexer.h"
#endif
#ifndef _NEWCONSOLE_TS2_PARSER_H_
#include "newConsole/torquescript2/parser.h"
#endif
#ifndef _NEWCONSOLE_TS2_EMITTER_H_
#include "newConsole/torquescript2/emitter.h"
#endif
#ifndef _NEWCONSOLE_TS2_SOURCEFILE_H_
#include "newConsole/torquescript2/sourceFile.h"
#endif
#ifndef _NEWCONSOLE_OBJECTREGISTRY_H_
#include "newConsole/host/objectRegistry.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTARRAY_H_
#include "newConsole/host/scriptArray.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTSTRUCTTRAITS_H_
#include "newConsole/host/scriptStructTraits.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      TorqueScript2Runtime::TorqueScript2Runtime()
         : mInterpreter(*this)
      {
         mInterpreter.setDebugHook(&mDebugAdapter);
      }

      RuntimeId TorqueScript2Runtime::id() const
      {
         return StringTable->insert("torquescript2");
      }

      bool TorqueScript2Runtime::canHandle(const char* filename) const
      {
         return hasScriptExtension(filename);
      }

      bool TorqueScript2Runtime::initialize(const HostBindingRegistry& bindings)
      {
         mBindings = const_cast<HostBindingRegistry*>(&bindings);
         return true;
      }

      void TorqueScript2Runtime::shutdown()
      {
         mBindings = nullptr;
      }

      bool TorqueScript2Runtime::compileAndPublish(const char* originName, const char* source, DiagnosticSink& diags,
         bool runTopLevelStatements, ReloadResult* outReloadResult)
      {
         StringTableEntry internedOrigin = StringTable->insert(originName);

         Lexer lexer(source, originName, [this](StringTableEntry name) -> bool
         {
            return mBindings != nullptr && mBindings->find(name) != nullptr;
         });

         Parser parser(lexer, internedOrigin);
         CompilationUnit unit = parser.parse();

         if (lexer.hasErrors())
         {
            for (const LexDiagnostic& d : lexer.diagnostics())
            {
               Diagnostic diag;
               diag.origin = internedOrigin;
               diag.line = d.line;
               diag.column = d.column;
               diag.message = d.message;
               diag.isError = true;
               diags.report(diag);
            }
         }
         if (parser.hasErrors())
         {
            for (const ParseDiagnostic& d : parser.diagnostics())
            {
               Diagnostic diag;
               diag.origin = internedOrigin;
               diag.line = d.line;
               diag.column = d.column;
               diag.message = d.message;
               diag.isError = true;
               diags.report(diag);
            }
         }
         if (lexer.hasErrors() || parser.hasErrors())
            return false;

         bool anyEmitError = false;

         const ast::StmtHandle* top = CompilationUnit::listData(unit.stmtList, unit.topLevel);
         for (U32 i = 0; i < unit.topLevel.count; ++i)
         {
            const ast::StmtNode& stmt = unit.get(top[i]);
            if (stmt.kind != ast::StmtKind::FunctionDecl)
               continue;

            Emitter emitter(unit);
            BytecodeUnit compiled = emitter.compileFunction(stmt.functionDecl);

            if (emitter.hasErrors())
            {
               anyEmitError = true;
               for (const EmitDiagnostic& d : emitter.diagnostics())
               {
                  Diagnostic diag;
                  diag.origin = internedOrigin;
                  diag.line = d.line;
                  diag.message = d.message;
                  diag.isError = true;
                  diags.report(diag);
               }

               if (outReloadResult)
               {
                  ReloadedSymbol sym;
                  sym.name = FunctionTable::qualify(stmt.functionDecl.namespaceName, stmt.functionDecl.functionName);
                  sym.applied = false;
                  sym.reason = "emit error";
                  outReloadResult->symbols.push_back(sym);
               }
               continue;
            }

            StringTableEntry qualifiedName = FunctionTable::qualify(stmt.functionDecl.namespaceName, stmt.functionDecl.functionName);
            auto published = std::make_shared<BytecodeUnit>(std::move(compiled));
            mFunctions.publish(qualifiedName, published);

            if (outReloadResult)
            {
               ReloadedSymbol sym;
               sym.name = qualifiedName;
               sym.applied = true;
               outReloadResult->symbols.push_back(sym);
            }
         }

         if (anyEmitError)
            return false;

         if (runTopLevelStatements)
         {
            Emitter topEmitter(unit);
            BytecodeUnit topUnit = topEmitter.compileTopLevel();
            if (topEmitter.hasErrors())
            {
               for (const EmitDiagnostic& d : topEmitter.diagnostics())
               {
                  Diagnostic diag;
                  diag.origin = internedOrigin;
                  diag.line = d.line;
                  diag.message = d.message;
                  diag.isError = true;
                  diags.report(diag);
               }
               return false;
            }

            auto topPublished = std::make_shared<BytecodeUnit>(std::move(topUnit));
            ScriptValue result = mInterpreter.run(topPublished, ScriptValueSpan());
            if (mInterpreter.hadFatalError())
            {
               Diagnostic diag;
               diag.origin = internedOrigin;
               diag.line = mInterpreter.lastError().line;
               diag.message = mInterpreter.lastError().message;
               diag.isError = true;
               diags.report(diag);
               return false;
            }
         }

         return true;
      }

      LoadResult TorqueScript2Runtime::loadSource(const char* originName, const char* source, DiagnosticSink& diags)
      {
         LoadResult result;
         result.success = compileAndPublish(originName, source, diags, /*runTopLevelStatements*/ true, nullptr);
         return result;
      }

      ScriptValue TorqueScript2Runtime::callFunction(SymbolId name, ScriptValueSpan args)
      {
         // IScriptRuntime entry point: try a ts2-compiled function via the
         // interpreter's normal path first, fall back to host-exported.
         // Not the same as the IInterpreterHost::callFunction overload
         // below, which is the reverse fallback (only called once
         // resolveFunctionUnit has already failed) - conflating the two
         // was a real bug: it skipped the interpreter entirely and broke
         // every ts2 call from outside the VM.
         std::shared_ptr<const BytecodeUnit> unit = mFunctions.lookup(name);
         if (unit)
            return mInterpreter.run(unit, args);

         return callFunction(nullptr, name, args);
      }

      ReloadResult TorqueScript2Runtime::reloadSource(const char* originName, const char* source, DiagnosticSink& diags)
      {
         // Top-level statements never re-run on reload - re-executing
         // global assignments/object decls against a live world is
         // exactly what hot-reload rules out. Only function bodies republish.
         ReloadResult result;
         compileAndPublish(originName, source, diags, /*runTopLevelStatements*/ false, &result);
         return result;
      }

      // =============================================================================
      // IInterpreterHost
      // =============================================================================

      ScriptValue TorqueScript2Runtime::getGlobal(StringTableEntry name)
      {
         MutexGuard guard(mGlobalsLock);
         auto it = mGlobals.find(name);
         return it != mGlobals.end() ? it->second : ScriptValue::makeNull();
      }

      void TorqueScript2Runtime::setGlobal(StringTableEntry name, const ScriptValue& value)
      {
         MutexGuard guard(mGlobalsLock);
         mGlobals[name] = value;
      }

      const ScriptClassRep* TorqueScript2Runtime::classRepForObject(const ScriptValue& object) const
      {
         ObjectHandle handle;
         if (!object.tryGet<ObjectHandle>(handle))
            return nullptr;

         ScriptObject* obj = ObjectRegistry::instance().resolve(handle);
         if (!obj)
            return nullptr;

         return obj->getRuntimeClassRep();
      }

      ScriptValue TorqueScript2Runtime::getField(const ScriptValue& object, StringTableEntry field, bool isInternal)
      {
         if (object.kind() == ScriptValue::Kind::Struct)
         {
            const StructTypeRep* type = object.structType();
            if (!type)
               return ScriptValue::makeError("getField: struct value has no type info");

            const ScriptStructFieldRep* fieldRep = type->findField(field);
            if (!fieldRep)
               return ScriptValue::makeError("getField: no such struct component");

            // Struct components share array storage with the value itself
            // (see ScriptValue::makeStruct) - a plain indexed read.
            return const_cast<ScriptValue&>(object).arrayRef()[fieldRep->componentIndex];
         }

         const ScriptClassRep* rep = classRepForObject(object);
         if (!rep)
            return ScriptValue::makeError("getField: object has no reflected class");

         const ScriptFieldRep* fieldRep = rep->findField(field);
         if (!fieldRep)
            return ScriptValue::makeError("getField: no such field");

         ObjectHandle handle;
         object.tryGet<ObjectHandle>(handle);
         ScriptObject* obj = ObjectRegistry::instance().resolve(handle);
         if (!obj)
            return ScriptValue::makeError("getField: stale object handle");

         // isInternal ('->' access) not yet distinguished from '.' at the
         // reflection layer - falls through to the same accessor either way.
         (void)isInternal;
         return fieldRep->get(obj);
      }

      bool TorqueScript2Runtime::setField(const ScriptValue& object, StringTableEntry field, bool isInternal, const ScriptValue& value)
      {
         if (object.kind() == ScriptValue::Kind::Struct)
         {
            const StructTypeRep* type = object.structType();
            if (!type)
               return false;

            const ScriptStructFieldRep* fieldRep = type->findField(field);
            if (!fieldRep)
               return false;

            // Writes through shared array storage - every ScriptValue
            // copy of this struct observes the mutation, same sharing
            // contract as ScriptValue::arrayRef() elsewhere.
            const_cast<ScriptValue&>(object).arrayRef()[fieldRep->componentIndex] = value;
            return true;
         }

         const ScriptClassRep* rep = classRepForObject(object);
         if (!rep)
            return false;

         const ScriptFieldRep* fieldRep = rep->findField(field);
         if (!fieldRep)
            return false;

         ObjectHandle handle;
         object.tryGet<ObjectHandle>(handle);
         ScriptObject* obj = ObjectRegistry::instance().resolve(handle);
         if (!obj)
            return false;

         (void)isInternal;
         return fieldRep->set(obj, value);
      }

      ScriptValue TorqueScript2Runtime::getIndex(const ScriptValue& base, const ScriptValue& index)
      {
         return scriptArrayGet(base, index);
      }

      ScriptValue TorqueScript2Runtime::setIndex(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value)
      {
         return scriptArraySet(base, index, value);
      }

      ScriptValue TorqueScript2Runtime::callFunction(StringTableEntry namespaceNameOrNull, StringTableEntry functionName, ScriptValueSpan args)
      {
         // Only reached for a call resolveFunctionUnit couldn't resolve -
         // i.e. not a ts2-implemented function. Falls through to a
         // host-exported static method of the same name, if any.
         if (mBindings)
         {
            for (const HostFunctionDecl& decl : mBindings->globalFunctions())
            {
               if (decl.name == functionName)
                  return decl.invoke(args);
            }
         }
         return ScriptValue::makeError("callFunction: unresolved function");
      }

      ScriptValue TorqueScript2Runtime::callMethod(const ScriptValue& object, StringTableEntry methodName, ScriptValueSpan args)
      {
         if (object.kind() == ScriptValue::Kind::Struct)
         {
            const StructTypeRep* type = object.structType();
            if (!type)
               return ScriptValue::makeError("callMethod: struct value has no type info");

            const ScriptStructMethodRep* methodRep = type->findMethod(methodName);
            if (!methodRep)
               return ScriptValue::makeError("callMethod: no such struct method");

            // Self is the ScriptValue itself, by reference, so an
            // in-place method can rewrite it (see ScriptStructMethodRep).
            return methodRep->invoke(const_cast<ScriptValue&>(object), args);
         }

         const ScriptClassRep* rep = classRepForObject(object);
         if (!rep)
            return ScriptValue::makeError("callMethod: object has no reflected class");

         const ScriptMethodRep* methodRep = rep->findMethod(methodName);
         if (!methodRep)
            return ScriptValue::makeError("callMethod: no such method");

         ObjectHandle handle;
         object.tryGet<ObjectHandle>(handle);
         ScriptObject* obj = ObjectRegistry::instance().resolve(handle);
         if (!obj)
            return ScriptValue::makeError("callMethod: stale object handle");

         return methodRep->invoke(obj, args);
      }

      ScriptValue TorqueScript2Runtime::newObject(const BytecodeUnit::ObjectDeclTemplate& tmpl,
         const ScriptValue* classNameValue,
         const ScriptValue* objectNameValue)
      {
         const char* className = nullptr;
         if (!classNameValue || !classNameValue->tryGet<const char*>(className) || !className)
            return ScriptValue::makeError("newObject: could not resolve class name");

         const char* objectName = "";
         if (objectNameValue)
            objectNameValue->tryGet<const char*>(objectName);

         StringTableEntry internedClassName = StringTable->insert(className);
         StringTableEntry internedObjectName = (objectName && objectName[0]) ? StringTable->insert(objectName) : nullptr;

         // singleton/datablock: find-or-create by name. new: always
         // fresh, but still recorded under its name (if named) so a
         // later singleton/datablock referring to the same name finds it.
         if ((tmpl.isSingleton || tmpl.isDatablock) && internedObjectName)
         {
            MutexGuard guard(mNamedObjectsLock);
            auto it = mNamedObjects.find(internedObjectName);
            if (it != mNamedObjects.end())
               return it->second;
         }

         ScriptObject* obj = ClassFactory::instance().construct(internedClassName);
         if (!obj)
            return ScriptValue::makeError("newObject: no registered constructor for this class");

         ObjectHandle handle = ObjectRegistry::instance().registerObject(obj);
         ScriptValue result = ScriptValue::makeObject(handle);

         if (internedObjectName)
         {
            MutexGuard guard(mNamedObjectsLock);
            mNamedObjects[internedObjectName] = result;
         }

         return result;
      }

      IInterpreterHost::IteratorHandle TorqueScript2Runtime::iterBegin(const ScriptValue& collection, bool isStringForm)
      {
         if (isStringForm)
         {
            // foreach$ tokenizes as whitespace-separated words, same
            // convention as TorqueScript's getWord/getField-style
            // functions. Implemented directly here since it needs
            // nothing beyond string conversion, unlike object-collection
            // foreach below.
            StringTokenIterator* it = new StringTokenIterator();
            String full = collection.toDisplayString();
            const char* s = full.c_str();
            while (*s)
            {
               while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
                  ++s;
               if (!*s)
                  break;
               const char* wordStart = s;
               while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r')
                  ++s;
               it->tokens.push_back(String(wordStart, static_cast<String::SizeType>(s - wordStart)));
            }
            return IteratorHandle{ it };
         }

         // Object-collection iteration (a SimSet-equivalent) has no
         // reflected representation in host/ yet - a known gap.
         // iterNext reports failure immediately for a handle created
         // this way, so foreach over an object collection runs zero
         // iterations rather than crashing or hanging.
         return IteratorHandle{};
      }

      bool TorqueScript2Runtime::iterNext(IteratorHandle handle, ScriptValue& outValue)
      {
         StringTokenIterator* it = static_cast<StringTokenIterator*>(handle.opaque);
         if (!it)
            return false; // object-collection form, or a null/invalid handle - see iterBegin

         if (it->index >= it->tokens.size())
            return false;

         outValue = ScriptValue::makeString(it->tokens[it->index].c_str());
         ++it->index;
         return true;
      }

      void TorqueScript2Runtime::iterEnd(IteratorHandle handle)
      {
         StringTokenIterator* it = static_cast<StringTokenIterator*>(handle.opaque);
         delete it;
      }

      std::shared_ptr<const BytecodeUnit> TorqueScript2Runtime::resolveFunctionUnit(StringTableEntry namespaceNameOrNull, StringTableEntry functionName)
      {
         return mFunctions.lookup(FunctionTable::qualify(namespaceNameOrNull, functionName));
      }

      std::shared_ptr<const BytecodeUnit> TorqueScript2Runtime::resolveMethodUnit(const ScriptValue& object, StringTableEntry methodName)
      {
         const ScriptClassRep* rep = classRepForObject(object);
         if (!rep)
            return nullptr;

         return mFunctions.lookup(FunctionTable::qualify(StringTable->insert(rep->getName()), methodName));
      }

   } // namespace ts2
} // namespace newConsole
