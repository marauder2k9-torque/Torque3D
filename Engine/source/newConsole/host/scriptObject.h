#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#define _NEWCONSOLE_SCRIPTOBJECT_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif

#include <atomic>
#include <memory>

namespace newConsole
{

   class ScriptObject;

   /// Backing block for a lazily-created weak reference. Allocated at
   /// most once per ScriptObject, only if a weak handle is requested.
   /// Shared-owned by the object and every WeakHandle that read it, so a
   /// WeakHandle can safely observe it after the object is gone (only
   /// the object pointer inside goes null; the block itself persists).
   struct ScriptWeakControlBlock
   {
      std::atomic<ScriptObject*> object;
      explicit ScriptWeakControlBlock(ScriptObject* obj) : object(obj) {}
   };

   template<typename T> class WeakHandle;

   /// Universal root of the engine's class hierarchy.
   ///
   /// @note Deliberately minimal - full replacement for the old
   ///   EngineObject/ConsoleObject pair, but pool allocation, RTTI,
   ///   export scope, factory dispatch, field reflection, singleton/
   ///   disposable support all live in separate layers above this class.
   ///   Only destroySelf() and describeSelf() belong here; resist adding
   ///   convenience surface just because the old root had it.
   ///
   /// @note Any class reaching this through more than one inheritance
   ///   path must derive `public virtual ScriptObject`, never plain
   ///   `public ScriptObject` - non-virtual derivation with a diamond
   ///   silently produces two separate ScriptObject subobjects (two
   ///   refcounts, two identities). Deriving virtually everywhere avoids
   ///   that failure mode outright.
   ///
   /// @note Because of the above, ScriptObject's constructor must be
   ///   callable with no arguments from any depth in a diamond hierarchy
   ///   (C++ requires the most-derived class to init a virtual base directly).
   ///
   /// @note Intrusive refcount only, no allocation, no weak-control-block
   ///   by default - only allocated on first getOrCreateWeakControl()
   ///   call, reference-counted independently (shared_ptr) so it
   ///   outlives the object while any WeakHandle holds it.
   class ScriptObject
   {
   public:
      ScriptObject() = default;
      virtual ~ScriptObject();

      ScriptObject(const ScriptObject&) = delete;
      ScriptObject& operator=(const ScriptObject&) = delete;

      void incRefCount() { mRefCount.fetch_add(1, std::memory_order_relaxed); }

      void decRefCount()
      {
         if (mRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
         {
            std::atomic_thread_fence(std::memory_order_acquire);
            tombstoneWeakControl();
            destroySelf();
         }
      }

      U32 getRefCount() const { return mRefCount.load(std::memory_order_relaxed); }

      /// Override to customize teardown. Default deletes this.
      virtual void destroySelf() { delete this; }

      /// Short human-readable description of this instance, for debugging.
      virtual String describeSelf() const;

      /// Reflection descriptor for this instance's actual (most-derived)
      /// type. Default nullptr; SCRIPT_CLASS/SCRIPT_CLASS_ROOT override
      /// automatically (scriptClassMacros.h). Without this, there's no
      /// way to go from a type-erased ScriptObject* back to its fields.
      virtual const class ScriptClassRep* getRuntimeClassRep() const { return nullptr; }

      /// Fires whenever a NotifyField<T> member is written - both a
      /// script-driven set and a plain C++ write go through
      /// NotifyField<T>::operator=, so both call this the same way. Fixes
      /// the legacy engine's gap where onStaticModified only fired for
      /// script writes, letting other C++ code bypass notification.
      ///
      /// @param fieldName as registered via SCRIPT_FIELDS/ADD_FIELD;
      ///   present even for a C++-only write.
      /// @param dirtyMask the field's NetFieldAttribute::dirtyMask (0 if
      ///   non-replicating) - typically ORed into a per-connection dirty
      ///   accumulator by an override.
      ///
      /// Default does nothing - opt-in; a class with no NotifyField<T>
      /// members never has this called.
      virtual void onFieldChanged(StringTableEntry fieldName, U32 dirtyMask) { (void)fieldName; (void)dirtyMask; }

      template<typename T> friend class WeakHandle;

   private:
      std::shared_ptr<ScriptWeakControlBlock> getOrCreateWeakControl();
      void tombstoneWeakControl();

      std::atomic<U32> mRefCount{ 0 };

      // Guards mWeakControl only. Taken at most once in the common case
      // (first getOrCreateWeakControl call) plus once at destruction -
      // never hot, so a plain mutex is fine.
      Mutex mWeakControlLock;
      std::shared_ptr<ScriptWeakControlBlock> mWeakControl;
   };

   /// Strong reference to a ScriptObject - ctor/dtor pair with
   /// incRefCount/decRefCount, same role as StrongRefPtr for ScriptObject.
   template<typename T>
   class StrongHandle
   {
   public:
      StrongHandle() = default;
      StrongHandle(T* obj) : mPtr(obj) { if (mPtr) mPtr->incRefCount(); }
      StrongHandle(const StrongHandle& other) : mPtr(other.mPtr) { if (mPtr) mPtr->incRefCount(); }
      StrongHandle(StrongHandle&& other) noexcept : mPtr(other.mPtr) { other.mPtr = nullptr; }

      ~StrongHandle() { if (mPtr) mPtr->decRefCount(); }

      StrongHandle& operator=(const StrongHandle& other)
      {
         if (this != &other)
         {
            if (other.mPtr) other.mPtr->incRefCount();
            if (mPtr) mPtr->decRefCount();
            mPtr = other.mPtr;
         }
         return *this;
      }

      StrongHandle& operator=(T* obj)
      {
         if (obj) obj->incRefCount();
         if (mPtr) mPtr->decRefCount();
         mPtr = obj;
         return *this;
      }

      T* get() const { return mPtr; }
      T* operator->() const { return mPtr; }
      T& operator*() const { return *mPtr; }
      operator T* () const { return mPtr; }

      bool isNull() const { return mPtr == nullptr; }
      bool isValid() const { return mPtr != nullptr; }

   private:
      T* mPtr = nullptr;
   };

   /// Weak reference to a ScriptObject. Reads nullptr once the object is
   /// destroyed - same contract as WeakRefPtr.
   ///
   /// @note First WeakHandle construction to a given object triggers
   ///   ScriptWeakControlBlock allocation, not ScriptObject construction.
   ///   The block is shared_ptr-owned so it outlives the object if a
   ///   WeakHandle still holds it; only the object pointer inside goes null.
   template<typename T>
   class WeakHandle
   {
   public:
      WeakHandle() = default;
      WeakHandle(T* obj) { set(obj); }
      WeakHandle(const WeakHandle&) = default;
      WeakHandle& operator=(const WeakHandle&) = default;

      WeakHandle& operator=(T* obj) { set(obj); return *this; }

      void set(T* obj)
      {
         mControl = obj ? obj->ScriptObject::getOrCreateWeakControl() : nullptr;
      }

      T* get() const
      {
         std::shared_ptr<ScriptWeakControlBlock> block = mControl.lock();
         if (!block) return nullptr;
         return static_cast<T*>(block->object.load(std::memory_order_acquire));
      }

      T* operator->() const { AssertFatal(get() != nullptr, "WeakHandle::operator-> - stale handle"); return get(); }
      T& operator*() const { AssertFatal(get() != nullptr, "WeakHandle::operator* - stale handle"); return *get(); }
      operator T* () const { return get(); }

      bool isValid() const { return get() != nullptr; }
      bool isNull() const { return get() == nullptr; }

   private:
      std::weak_ptr<ScriptWeakControlBlock> mControl;
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTOBJECT_H_
