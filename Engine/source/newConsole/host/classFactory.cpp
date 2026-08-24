#include "newConsole/host/classFactory.h"

namespace newConsole
{

ClassFactory& ClassFactory::instance()
{
   static ClassFactory sInstance;
   return sInstance;
}

bool ClassFactory::registerConstructor(StringTableEntry className, ConstructorFn ctor)
{
   for (U32 i = 0; i < mEntries.size(); ++i)
   {
      if (mEntries[i].name == className)
         return false;
   }
   mEntries.push_back(Entry{ className, ctor });
   return true;
}

ScriptObject* ClassFactory::construct(StringTableEntry className) const
{
   for (U32 i = 0; i < mEntries.size(); ++i)
   {
      if (mEntries[i].name == className)
         return mEntries[i].ctor();
   }
   return nullptr;
}

} // namespace newConsole
