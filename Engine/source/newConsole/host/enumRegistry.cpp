#include "newConsole/host/enumRegistry.h"

namespace newConsole
{

EnumTypeInfo::EnumTypeInfo(const char* typeName, std::initializer_list<Entry> entries)
   : mTypeName(typeName)
{
   for (const Entry& e : entries)
   {
      // e.name is expected to already be a StringTableEntry (see
      // SCRIPT_ENUM in scriptClassMacros.h, which interns every
      // enumerator name before building this list) - inserting it
      // as-is here, rather than re-interning, means mByName's keys are
      // guaranteed canonical, so a lookup only needs to intern its own
      // query string the same way (see findByName) for both sides to
      // agree, without this constructor needing to know how the caller
      // obtained the name.
      mByName[e.name] = e.value;
      // Only the first name registered for a given value is kept as
      // "the" display name for findByValue - later duplicate values
      // (aliases) are still findable by name via mByName, just not
      // chosen as the canonical name for that value.
      if (mByValue.find(e.value) == mByValue.end())
         mByValue[e.value] = e.name;
   }
}

bool EnumTypeInfo::findByName(StringTableEntry name, S64& outValue) const
{
   // name is assumed already interned (StringTableEntry, not a raw
   // const char*) - StringTable's own case-insensitive interning (see
   // stringTable.h) is what makes this a correct case-insensitive
   // lookup: two different-case spellings of the same enumerator intern
   // to the same pointer, so a plain pointer-keyed map lookup already
   // gets case-insensitivity for free, with no per-lookup string
   // comparison at all.
   auto it = mByName.find(name);
   if (it == mByName.end())
      return false;
   outValue = it->second;
   return true;
}

StringTableEntry EnumTypeInfo::findByValue(S64 value) const
{
   auto it = mByValue.find(value);
   return it != mByValue.end() ? it->second : nullptr;
}

EnumRegistry& EnumRegistry::instance()
{
   static EnumRegistry sInstance;
   return sInstance;
}

bool EnumRegistry::registerEnumImpl(std::type_index key, const char* typeName, std::initializer_list<EnumTypeInfo::Entry> entries)
{
   if (mEnums.find(key) != mEnums.end())
      return false;

   mEnums.emplace(key, EnumTypeInfo(typeName, entries));
   return true;
}

const EnumTypeInfo* EnumRegistry::findImpl(std::type_index key) const
{
   auto it = mEnums.find(key);
   return it != mEnums.end() ? &it->second : nullptr;
}

} // namespace newConsole
