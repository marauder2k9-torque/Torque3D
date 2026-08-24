#ifndef _NEWCONSOLE_ENUMREGISTRY_H_
#define _NEWCONSOLE_ENUMREGISTRY_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include <typeindex>
#include <unordered_map>

namespace newConsole
{

/// One registered enum type's name<->value lookup tables. Built once,
/// at registration time (see SCRIPT_ENUM in scriptClassMacros.h), not
/// re-scanned per access - this is the concrete fix for the legacy
/// EngineEnumTable's cost: that type had no lookup method at all, so
/// every field/method touching an enum had to hand-write its own linear
/// scan and string comparison against the raw {int,name} array. Here,
/// both lookup directions are O(1) (hash map), built exactly once per
/// enum type regardless of how many fields/methods/parameters use it.
class EnumTypeInfo
{
public:
   struct Entry { S64 value; StringTableEntry name; };

   EnumTypeInfo(const char* typeName, std::initializer_list<Entry> entries);

   /// @return true and fills @a outValue if @a name (case-insensitive,
   ///   matching StringTable's own default - see stringTable.h) matches
   ///   a registered enumerator.
   bool findByName(StringTableEntry name, S64& outValue) const;

   /// @return the registered name for @a value, or nullptr if no
   ///   enumerator has that value.
   StringTableEntry findByValue(S64 value) const;

   const char* typeName() const { return mTypeName; }

private:
   const char* mTypeName;
   std::unordered_map<StringTableEntry, S64> mByName;
   std::unordered_map<S64, StringTableEntry> mByValue;
};

/// Maps a C++ type (via std::type_index) to its EnumTypeInfo. One
/// registry entry per enum class, populated once by SCRIPT_ENUM's
/// auto-registration - see scriptClassMacros.h.
class EnumRegistry
{
public:
   static EnumRegistry& instance();

   /// @return false if @a T is already registered (checked in every
   ///   build, matching HostBindingRegistry::registerClass's own
   ///   reasoning - enum registration happens once at startup and a
   ///   silently-dropped duplicate must not depend on assert builds to
   ///   catch).
   template<typename T>
   bool registerEnum(const char* typeName, std::initializer_list<EnumTypeInfo::Entry> entries)
   {
      return registerEnumImpl(std::type_index(typeid(T)), typeName, entries);
   }

   template<typename T>
   const EnumTypeInfo* find() const
   {
      return findImpl(std::type_index(typeid(T)));
   }

private:
   bool registerEnumImpl(std::type_index key, const char* typeName, std::initializer_list<EnumTypeInfo::Entry> entries);
   const EnumTypeInfo* findImpl(std::type_index key) const;

   std::unordered_map<std::type_index, EnumTypeInfo> mEnums;
};

} // namespace newConsole

#endif // !_NEWCONSOLE_ENUMREGISTRY_H_
