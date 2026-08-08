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

#include <atomic>
#include <memory>
#include <mutex>

namespace newConsole
{

class ScriptObject;

/// Backing block for a lazily-created weak reference. Allocated at most
/// once per ScriptObject, only if something actually asks for a weak
/// handle to it. Kept alive by shared ownership between the owning
/// ScriptObject and every WeakHandle that has read it, so a WeakHandle
/// can safely observe the block after the object itself is gone - only
/// the object pointer inside it goes to nullptr, the block does not
/// disappear out from under a concurrent reader.
struct ScriptWeakControlBlock
{
   std::atomic<ScriptObject*> object;
   explicit ScriptWeakControlBlock(ScriptObject* obj) : object(obj) {}
};

template<typename T> class WeakHandle;

/// Universal root of the engine's class hierarchy.
///
/// @note Deliberately minimal. This is the full replacement for the old
///   EngineObject/ConsoleObject pair, but it does not attempt to be a
///   modernized re-implementation of everything that pair did - pool
///   allocation, RTTI/type-info, export scope, factory dispatch, field
///   reflection, singleton/disposable support all move out into separate,
///   composable layers above this class rather than being baked into the
///   root. What actually needs to live here is only the virtual contract
///   a class needs to participate in lifetime management: destroySelf()
///   (already required by decRefCount()) and describeSelf() (debug
///   description - not lifetime-related, but the one other virtual the
///   old root declared that a subclass is expected to override). Nothing
///   else belongs here; resist the urge to add convenience surface to
///   this class later just because EngineObject used to have it.
///
/// @note Every class that ultimately needs lifetime/refcount management -
///   including classes that reach this through more than one inheritance
///   path - derives from this using `public virtual ScriptObject`, never
///   plain `public ScriptObject`. Non-virtual derivation is only safe for
///   a class with a single, non-branching path to this root; the moment a
///   second path is introduced anywhere in that class's ancestry, a
///   non-virtual derivation silently produces two separate ScriptObject
///   subobjects (two refcounts, two identities) in anything that inherits
///   both paths. Deriving virtually everywhere avoids that failure mode
///   outright rather than requiring every future author to notice when
///   their class's inheritance graph has grown a diamond.
///
/// @note Because of the above, ScriptObject's constructor must be
///   callable with no arguments from any depth in a diamond hierarchy -
///   C++ requires the most-derived class to initialize a virtual base
///   directly, bypassing intermediate classes' constructors for it.
///
/// @note Intrusive refcount only, no allocation, no weak-control-block
///   by default. A weak control block is allocated on first call to
///   getOrCreateWeakControl() and nowhere else - an object that is never
///   weak referenced never pays for one. Once allocated, the block is
///   reference-counted independently of the object (shared_ptr) so it
///   safely outlives the object for as long as any WeakHandle still
///   holds it.
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

   template<typename T> friend class WeakHandle;

private:
   std::shared_ptr<ScriptWeakControlBlock> getOrCreateWeakControl();
   void tombstoneWeakControl();

   std::atomic<U32> mRefCount{0};

   // Guards mWeakControl only. Taken at most once per object in the
   // common case (first getOrCreateWeakControl call) and once more at
   // destruction - never a hot path, so a plain mutex is the right tool
   // here rather than a lock-free scheme.
   std::mutex mWeakControlLock;
   std::shared_ptr<ScriptWeakControlBlock> mWeakControl;
};

/// Strong reference to a ScriptObject. Constructor/destructor pair with
/// incRefCount/decRefCount; equivalent role to StrongRefPtr but targets
/// ScriptObject specifically.
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
   operator T*() const { return mPtr; }

   bool isNull() const { return mPtr == nullptr; }
   bool isValid() const { return mPtr != nullptr; }

private:
   T* mPtr = nullptr;
};

/// Weak reference to a ScriptObject. Reads nullptr once the object has
/// been destroyed, same external contract as WeakRefPtr.
///
/// @note First construction of a WeakHandle to a given object is what
///   triggers ScriptWeakControlBlock allocation for that object - not
///   ScriptObject construction. The block itself is shared_ptr-owned so
///   it outlives the object if a WeakHandle is still holding it; only the
///   object pointer inside the block goes to nullptr on destruction.
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
   T& operator*() const  { AssertFatal(get() != nullptr, "WeakHandle::operator* - stale handle"); return *get(); }
   operator T*() const   { return get(); }

   bool isValid() const { return get() != nullptr; }
   bool isNull() const  { return get() == nullptr; }

private:
   std::weak_ptr<ScriptWeakControlBlock> mControl;
};

} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTOBJECT_H_
