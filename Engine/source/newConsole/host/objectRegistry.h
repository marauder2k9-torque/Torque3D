#ifndef _NEWCONSOLE_OBJECTREGISTRY_H_
#define _NEWCONSOLE_OBJECTREGISTRY_H_

#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#include "newConsole/host/scriptObject.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace newConsole
{

   /// Maps ObjectHandle (id/generation) to live ScriptObject instances.
   ///
   /// @note Kept separate from ScriptObject - ScriptObject itself has no
   ///   identity concept, only lifetime/refcount. An object gets a handle
   ///   only by being registered here.
   ///
   /// @note Assumes heap-allocated, refcount-owned objects. registerObject
   ///   calls incRefCount(); the last decRefCount() triggers destroySelf()
   ///   (default `delete this`). Registering a stack or otherwise
   ///   non-heap-owned object is undefined behavior.
   ///
   /// @note generation guards against a stale handle resolving to a later
   ///   object reusing the same id slot - resolve() checks both.
   class ObjectRegistry
   {
   public:
      static ObjectRegistry& instance();

      /// Registers @a object, returning a fresh handle - or the object's
      /// existing handle if already registered. Idempotent per object, not
      /// per call: repeated conversion to ScriptValue reuses one handle.
      ObjectHandle registerObject(ScriptObject* object);

      /// Removes @a handle's mapping and releases the strong reference
      /// registerObject() took.
      void unregisterObject(ObjectHandle handle);

      /// @return the live object for @a handle, or nullptr if stale (wrong
      ///   generation) or never registered.
      ScriptObject* resolve(ObjectHandle handle) const;

   private:
      struct Slot
      {
         ScriptObject* object = nullptr;
         U32 generation = 0;
         bool occupied = false;
      };

      Vector<Slot> mSlots;
      Vector<U32> mFreeIndices;

      /// Reverse lookup for registerObject's idempotency. Linear, not a
      /// hash table, for now - revisit if profiling shows this matters
      /// (registration isn't expected to be hot-path).
      struct ReverseEntry { ScriptObject* object; U32 index; };
      Vector<ReverseEntry> mReverseLookup;
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_OBJECTREGISTRY_H_
