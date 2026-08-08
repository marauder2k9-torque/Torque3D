#include "newConsole/host/hostBinding.h"

namespace newConsole
{

   const ScriptFieldRep* ScriptClassRep::findField(StringTableEntry name) const
   {
      // StringTableEntry is an interned pointer - equality is pointer
      // equality, not strcmp. Caller is expected to have gone through
      // StringTable->insert() already (true for anything reaching here via
      // the parser, since identifiers intern at lex time).
      for (const ScriptClassRep* rep = this; rep != nullptr; rep = rep->mParent)
      {
         const Vector<ScriptFieldRep>& fields = rep->mFields;
         for (U32 i = 0; i < fields.size(); ++i)
         {
            if (fields[i].name == name)
               return &fields[i];
         }
      }
      return nullptr;
   }

   const ScriptMethodRep* ScriptClassRep::findMethod(StringTableEntry name) const
   {
      for (const ScriptClassRep* rep = this; rep != nullptr; rep = rep->mParent)
      {
         const Vector<ScriptMethodRep>& methods = rep->mMethods;
         for (U32 i = 0; i < methods.size(); ++i)
         {
            if (methods[i].name == name)
               return &methods[i];
         }
      }
      return nullptr;
   }

   const ScriptStaticFieldRep* ScriptClassRep::findStaticField(StringTableEntry name) const
   {
      // Static fields walk the parent chain too - a SCRIPT_CLASS_ROOT class
      // is not required to be a hierarchy root in practice (nothing stops
      // one static-only class reflecting "on top of" another), so this
      // mirrors findField's walk rather than assuming static classes are
      // always single-level.
      for (const ScriptClassRep* rep = this; rep != nullptr; rep = rep->mParent)
      {
         const Vector<ScriptStaticFieldRep>& fields = rep->mStaticFields;
         for (U32 i = 0; i < fields.size(); ++i)
         {
            if (fields[i].name == name)
               return &fields[i];
         }
      }
      return nullptr;
   }

   const ScriptStaticMethodRep* ScriptClassRep::findStaticMethod(StringTableEntry name) const
   {
      for (const ScriptClassRep* rep = this; rep != nullptr; rep = rep->mParent)
      {
         const Vector<ScriptStaticMethodRep>& methods = rep->mStaticMethods;
         for (U32 i = 0; i < methods.size(); ++i)
         {
            if (methods[i].name == name)
               return &methods[i];
         }
      }
      return nullptr;
   }

   HostBindingRegistry& HostBindingRegistry::instance()
   {
      static HostBindingRegistry sInstance;
      return sInstance;
   }

   bool HostBindingRegistry::registerClass(const ScriptClassRep* rep)
   {
      AssertFatal(rep != nullptr, "HostBindingRegistry::registerClass - null rep");
      if (find(rep->getName()) != nullptr)
         return false;
      mClasses.push_back(rep);
      return true;
   }

   void HostBindingRegistry::registerFunction(HostFunctionDecl decl)
   {
      AssertFatal(decl.invoke != nullptr,
         "HostBindingRegistry::registerFunction - null invoke pointer");
      mFunctions.push_back(decl);
   }

   const ScriptClassRep* HostBindingRegistry::find(StringTableEntry className) const
   {
      for (U32 i = 0; i < mClasses.size(); ++i)
      {
         if (mClasses[i]->getName() == className)
            return mClasses[i];
      }
      return nullptr;
   }

} // namespace newConsole
