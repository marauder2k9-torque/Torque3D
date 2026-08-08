#include "newConsole/host/scriptObject.h"

namespace newConsole
{

String ScriptObject::describeSelf() const
{
   // Deliberately minimal default - a subclass with anything more useful
   // to report (name, id, class name) overrides this. No RTTI/type-name
   // lookup here since ScriptObject itself carries no type-info concept.
   return String("<ScriptObject>");
}

ScriptObject::~ScriptObject()
{
   // If decRefCount() drove the refcount to zero, it already tombstoned
   // the weak control block before calling destroySelf(), which is what
   // got us here. If the object is being destroyed some other way (a
   // derived class' own explicit delete, a stack instance going out of
   // scope), tombstone here too so a live WeakHandle never reads a
   // pointer into freed memory regardless of how destruction happened.
   tombstoneWeakControl();
}

std::shared_ptr<ScriptWeakControlBlock> ScriptObject::getOrCreateWeakControl()
{
   std::lock_guard<std::mutex> guard(mWeakControlLock);
   if (!mWeakControl)
      mWeakControl = std::make_shared<ScriptWeakControlBlock>(this);
   return mWeakControl;
}

void ScriptObject::tombstoneWeakControl()
{
   std::shared_ptr<ScriptWeakControlBlock> block;
   {
      std::lock_guard<std::mutex> guard(mWeakControlLock);
      block = mWeakControl;
      mWeakControl.reset();
   }

   // Null the object pointer inside the block, but let the block itself
   // live on via shared_ptr refcounting - any WeakHandle already holding
   // a weak_ptr to it will lock() to nullptr from here on rather than
   // dereference freed memory.
   if (block)
      block->object.store(nullptr, std::memory_order_release);
}

} // namespace newConsole
