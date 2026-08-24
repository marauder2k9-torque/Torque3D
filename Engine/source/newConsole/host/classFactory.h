#ifndef _NEWCONSOLE_CLASSFACTORY_H_
#define _NEWCONSOLE_CLASSFACTORY_H_

#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#include "newConsole/host/scriptObject.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include <type_traits>

namespace newConsole
{

/// Maps a class name string to a constructor function, so an object can
/// be allocated from script by name ("new TestWidget(...)") without the
/// caller needing to know the C++ type at compile time.
///
/// @note Kept separate from ScriptClassRep - a class's field/method
///   reflection and its ability to be constructed by name are different
///   capabilities (a class might be reflected but only ever constructed
///   from C++, e.g. a manager singleton set up at startup). SCRIPT_CLASS
///   registers a constructor here automatically only if the class is
///   default-constructible; a class with no default constructor is
///   still fully reflected, just not factory-constructible.
class ClassFactory
{
public:
   static ClassFactory& instance();

   using ConstructorFn = ScriptObject* (*)();

   /// Registers @a ctor under @a className. @return false if the name
   /// was already registered.
   bool registerConstructor(StringTableEntry className, ConstructorFn ctor);

   /// @return a freshly constructed instance, or nullptr if no
   ///   constructor is registered for @a className.
   ScriptObject* construct(StringTableEntry className) const;

private:
   struct Entry { StringTableEntry name; ConstructorFn ctor; };
   Vector<Entry> mEntries;
};

namespace detail
{

/// Default-constructs a T on the heap. Only ever wired up for a T that
/// is actually default-constructible - see
/// registerDefaultConstructorIfPossible below.
template<typename T>
ScriptObject* defaultConstruct()
{
   return new T();
}

/// Registers T's default constructor with ClassFactory if T actually
/// has one; a no-op for a T that doesn't. SCRIPT_CLASS calls this
/// unconditionally for every class - a class with no default
/// constructor stays reflected but simply never gets a factory entry,
/// rather than failing to compile.
template<typename T>
typename std::enable_if<std::is_default_constructible<T>::value, void>::type
registerDefaultConstructorIfPossible(StringTableEntry className)
{
   ClassFactory::instance().registerConstructor(className, &defaultConstruct<T>);
}

template<typename T>
typename std::enable_if<!std::is_default_constructible<T>::value, void>::type
registerDefaultConstructorIfPossible(StringTableEntry)
{
}

} // namespace detail
} // namespace newConsole

#endif // !_NEWCONSOLE_CLASSFACTORY_H_
