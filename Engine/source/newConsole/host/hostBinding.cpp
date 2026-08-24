#include "newConsole/host/hostBinding.h"

namespace newConsole
{

   const ScriptFieldRep* ScriptClassRep::findField(StringTableEntry name) const
   {
      // StringTableEntry equality is pointer equality (interned), not
      // strcmp - caller is expected to have already gone through
      // StringTable->insert() (true for anything from the parser).
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
      // Walks the parent chain too - nothing stops one static-only class
      // reflecting on top of another, so this mirrors findField's walk.
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
