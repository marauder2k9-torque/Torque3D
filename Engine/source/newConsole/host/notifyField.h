#ifndef _NEWCONSOLE_NOTIFYFIELD_H_
#define _NEWCONSOLE_NOTIFYFIELD_H_

#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#include "newConsole/host/scriptObject.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif

namespace newConsole
{

   /// A field wrapper that fires ScriptObject::onFieldChanged on every
   /// write - both a script-driven set (via ScriptFieldRep) and a plain
   /// C++ write go through the same operator=. Closes the legacy engine's
   /// gap where onStaticModified only fired for script-driven writes (see
   /// onFieldChanged in scriptObject.h).
   ///
   /// @note Not a template specialized per field name/mask - those are
   ///   ordinary constructor args, supplied once at declaration:
   ///   @code
   ///   NotifyField<S32> mHealth{ this, "mHealth", 0x01 };
   ///   @endcode
   ///   ADD_FIELD/ADD_FIELD_NET generate this construction automatically
   ///   (see scriptClassMacros.h); this class is the underlying mechanism
   ///   and an escape hatch for a field that needs notification outside
   ///   the macro-driven path.
   /// @note operator= returns *this (NotifyField&), not T&, so chained
   ///   assignment (a = b = 5) works - code needing a real T& (an
   ///   old-style out-parameter API) should use get()/set() explicitly.
   template<typename T>
   class NotifyField
   {
   public:
      NotifyField(ScriptObject* owner, StringTableEntry fieldName, U32 dirtyMask, T initial = T{})
         : mOwner(owner), mFieldName(fieldName), mDirtyMask(dirtyMask), mValue(std::move(initial))
      {
      }

      NotifyField(const NotifyField&) = delete;
      NotifyField& operator=(const NotifyField&) = delete;

      const T& get() const { return mValue; }
      operator const T& () const { return mValue; }

      NotifyField& operator=(T value)
      {
         mValue = std::move(value);
         notify();
         return *this;
      }

      /// Direct access to owner/field identity - used by ScriptFieldRep's
      /// generated get/set so a script-driven set goes through this exact
      /// operator=, not a parallel path that could drift out of sync.
      ScriptObject* owner() const { return mOwner; }
      StringTableEntry fieldName() const { return mFieldName; }
      U32 dirtyMask() const { return mDirtyMask; }

   private:
      void notify()
      {
         if (mOwner)
            mOwner->onFieldChanged(mFieldName, mDirtyMask);
      }

      ScriptObject* mOwner;
      StringTableEntry mFieldName;
      U32 mDirtyMask;
      T mValue;
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_NOTIFYFIELD_H_
