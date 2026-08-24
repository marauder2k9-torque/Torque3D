#include "newConsole/host/objectRegistry.h"

namespace newConsole
{

   ObjectRegistry& ObjectRegistry::instance()
   {
      static ObjectRegistry sInstance;
      return sInstance;
   }

   ObjectHandle ObjectRegistry::registerObject(ScriptObject* object)
   {
      if (!object)
         return ObjectHandle{};

      for (U32 i = 0; i < mReverseLookup.size(); ++i)
      {
         if (mReverseLookup[i].object == object)
         {
            U32 index = mReverseLookup[i].index;
            ObjectHandle existing;
            existing.id = index + 1;
            existing.generation = mSlots[index].generation;
            return existing;
         }
      }

      object->incRefCount();

      U32 index;
      if (!mFreeIndices.empty())
      {
         index = mFreeIndices[mFreeIndices.size() - 1];
         mFreeIndices.pop_back();
      }
      else
      {
         Slot fresh;
         mSlots.push_back(fresh);
         index = static_cast<U32>(mSlots.size() - 1);
      }

      Slot& slot = mSlots[index];
      slot.object = object;
      slot.occupied = true;
      if (slot.generation == 0)
         slot.generation = 1;

      mReverseLookup.push_back(ReverseEntry{ object, index });

      ObjectHandle handle;
      handle.id = index + 1;
      handle.generation = slot.generation;
      return handle;
   }

   void ObjectRegistry::unregisterObject(ObjectHandle handle)
   {
      if (handle.isNull())
         return;

      U32 index = handle.id - 1;
      if (index >= static_cast<U32>(mSlots.size()))
         return;

      Slot& slot = mSlots[index];
      if (!slot.occupied || slot.generation != handle.generation)
         return;

      ScriptObject* obj = slot.object;
      slot.object = nullptr;
      slot.occupied = false;
      ++slot.generation;
      mFreeIndices.push_back(index);

      for (U32 i = 0; i < mReverseLookup.size(); ++i)
      {
         if (mReverseLookup[i].object == obj)
         {
            mReverseLookup.erase(i);
            break;
         }
      }

      if (obj)
         obj->decRefCount();
   }

   ScriptObject* ObjectRegistry::resolve(ObjectHandle handle) const
   {
      if (handle.isNull())
         return nullptr;

      U32 index = handle.id - 1;
      if (index >= static_cast<U32>(mSlots.size()))
         return nullptr;

      const Slot& slot = mSlots[index];
      if (!slot.occupied || slot.generation != handle.generation)
         return nullptr;

      return slot.object;
   }

} // namespace newConsole
