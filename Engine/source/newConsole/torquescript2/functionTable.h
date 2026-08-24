#ifndef _NEWCONSOLE_TS2_FUNCTIONTABLE_H_
#define _NEWCONSOLE_TS2_FUNCTIONTABLE_H_

#ifndef _NEWCONSOLE_TS2_BYTECODE_H_
#include "newConsole/torquescript2/bytecode.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif

#include <memory>
#include <unordered_map>

namespace newConsole
{
   namespace ts2
   {

      /// Name-to-BytecodeUnit table, one indirection cell per function.
      ///
      /// @note publish() atomically replaces a cell's current version rather
      ///   than mutating a BytecodeUnit in place - a CallFrame already
      ///   executing the old version keeps its own shared_ptr to it (see
      ///   interpreter.h's CallFrame comment) and is unaffected by a publish()
      ///   that happens while it is running. A lookup performed after
      ///   publish() sees the new version immediately; nothing already in
      ///   flight is disturbed.
      class FunctionTable
      {
      public:
         /// @return the currently published unit for @a qualifiedName, or
         ///   nullptr if nothing has ever been published under that name.
         std::shared_ptr<const BytecodeUnit> lookup(StringTableEntry qualifiedName) const;

         /// Publishes @a unit as the current version for @a qualifiedName,
         /// replacing whatever was there before. Safe to call while other
         /// threads/frames are calling lookup() or executing a previously
         /// looked-up unit - see this class's own note above.
         void publish(StringTableEntry qualifiedName, std::shared_ptr<const BytecodeUnit> unit);

         /// Builds the qualified lookup key for a namespaced or bare function
         /// name - "Namespace::name" or just "name". Exposed so callers
         /// (TorqueScript2Runtime, the interpreter host bridge) construct keys
         /// identically rather than each hand-rolling the same concatenation.
         static StringTableEntry qualify(StringTableEntry namespaceNameOrNull, StringTableEntry functionName);

      private:
         // std::unordered_map<StringTableEntry, ...> hashes/compares by raw
         // pointer value, not string contents - correct and intentional here
         // specifically because StringTableEntry is interned (StringTable
         // guarantees equal strings share one pointer), so pointer identity
         // already is string equality. This would silently break if a caller
         // ever passed a non-interned const char* as a key.
         mutable Mutex mLock;
         std::unordered_map<StringTableEntry, std::shared_ptr<const BytecodeUnit>> mCells;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_FUNCTIONTABLE_H_
