#include "newConsole/host/scriptStructTraits.h"

namespace newConsole
{

   StructTypeRegistry& StructTypeRegistry::instance()
   {
      static StructTypeRegistry sInstance;
      return sInstance;
   }

   const StructTypeRep* StructTypeRegistry::registerTypeImpl(std::type_index key, StructTypeRep rep)
   {
      if (mTypes.find(key) != mTypes.end())
         return nullptr;

      auto result = mTypes.emplace(key, std::move(rep));
      return &result.first->second;
   }

   const StructTypeRep* StructTypeRegistry::findImpl(std::type_index key) const
   {
      auto it = mTypes.find(key);
      return it != mTypes.end() ? &it->second : nullptr;
   }

} // namespace newConsole
