#include "newConsole/host/scriptObject.h"

namespace newConsole
{

   String ScriptObject::describeSelf() const
   {
      // Minimal default - a subclass with more to report (name, id, class)
      // overrides this. No RTTI lookup; ScriptObject carries no type-info.
      return String("<ScriptObject>");
   }

   ScriptObject::~ScriptObject()
   {
      // decRefCount() already tombstones the weak control block before
      // calling destroySelf(). If destroyed some other way (explicit
      // delete, stack instance), tombstone here too so a live WeakHandle
      // never reads freed memory.
      tombstoneWeakControl();
   }

   std::shared_ptr<ScriptWeakControlBlock> ScriptObject::getOrCreateWeakControl()
   {
      MutexGuard guard(mWeakControlLock);
      if (!mWeakControl)
         mWeakControl = std::make_shared<ScriptWeakControlBlock>(this);
      return mWeakControl;
   }

   void ScriptObject::tombstoneWeakControl()
   {
      std::shared_ptr<ScriptWeakControlBlock> block;
      {
         MutexGuard guard(mWeakControlLock);
         block = mWeakControl;
         mWeakControl.reset();
      }

      // Null the object pointer but let the block itself live on via
      // shared_ptr refcounting - a WeakHandle's weak_ptr locks to nullptr
      // instead of dereferencing freed memory.
      if (block)
         block->object.store(nullptr, std::memory_order_release);
   }

} // namespace newConsole
