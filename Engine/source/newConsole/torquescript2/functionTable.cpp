#include "newConsole/torquescript2/functionTable.h"

#ifndef _STRINGFUNCTIONS_H_
#include "core/strings/stringFunctions.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      std::shared_ptr<const BytecodeUnit> FunctionTable::lookup(StringTableEntry qualifiedName) const
      {
         MutexGuard guard(mLock);
         auto it = mCells.find(qualifiedName);
         return it != mCells.end() ? it->second : nullptr;
      }

      void FunctionTable::publish(StringTableEntry qualifiedName, std::shared_ptr<const BytecodeUnit> unit)
      {
         MutexGuard guard(mLock);
         mCells[qualifiedName] = std::move(unit);
      }

      StringTableEntry FunctionTable::qualify(StringTableEntry namespaceNameOrNull, StringTableEntry functionName)
      {
         if (!namespaceNameOrNull || namespaceNameOrNull[0] == '\0')
            return functionName;

         char buffer[512];
         dSprintf(buffer, sizeof(buffer), "%s::%s", namespaceNameOrNull, functionName);
         return StringTable->insert(buffer);
      }

   } // namespace ts2
} // namespace newConsole
