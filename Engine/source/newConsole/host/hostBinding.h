#ifndef _NEWCONSOLE_HOSTBINDING_H_
#define _NEWCONSOLE_HOSTBINDING_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif
#ifndef _NEWCONSOLE_NETFIELDATTRIBUTE_H_
#include "newConsole/host/netFieldAttribute.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace newConsole
{

   class ScriptObject;

   /// Read/write accessors for one exported instance field, type-erased
   /// at the ScriptValue boundary but generated from a real member
   /// pointer. For a static-only class (SCRIPT_CLASS_ROOT, no instance),
   /// use ScriptStaticFieldRep instead.
   struct ScriptFieldRep
   {
      StringTableEntry name;
      const char* usage;
      ScriptValue(*get)(const ScriptObject* self);
      bool (*set)(ScriptObject* self, const ScriptValue& value);

      /// Absent (default-constructed, dirtyMask == 0) for non-replicating
      /// fields. See NetFieldAttribute::replicates().
      NetFieldAttribute network;
   };

   /// Read/write accessors for one exported static field - no self
   /// pointer, since a static-only reflected class has no instance.
   ///
   /// @note Separate type from ScriptFieldRep rather than a
   ///   self-parameter-unused variant - a caller holding this has no
   ///   self to pass, and the type makes that visible at the call site.
   /// @note No NetFieldAttribute - no per-instance dirty state exists
   ///   for something with no instance.
   struct ScriptStaticFieldRep
   {
      StringTableEntry name;
      const char* usage;
      ScriptValue(*get)();
      bool (*set)(const ScriptValue& value);
   };

   /// One exported instance method. argCount is fixed arity for v1;
   /// variadic export not yet supported. Use ScriptStaticMethodRep for
   /// a static-only class's method.
   struct ScriptMethodRep
   {
      StringTableEntry name;
      const char* usage;
      U32 argCount;
      ScriptValue(*invoke)(ScriptObject* self, ScriptValueSpan args);
   };

   /// One exported static method - no self pointer, same reasoning as
   /// ScriptStaticFieldRep. Same shape as HostFunctionDecl but its own
   /// type so it's discoverable via ScriptClassRep::getStaticMethods(),
   /// not just the flat global-function list.
   struct ScriptStaticMethodRep
   {
      StringTableEntry name;
      const char* usage;
      U32 argCount;
      ScriptValue(*invoke)(ScriptValueSpan args);
   };

   /// Reflection descriptor for one SCRIPT_CLASS or SCRIPT_CLASS_ROOT type.
   ///
   /// @note A SCRIPT_CLASS_ROOT class (static-only, no instance)
   ///   populates only the static lists; an ordinary SCRIPT_CLASS
   ///   populates only the instance lists. A class can populate both if
   ///   it genuinely has both.
   /// @note Built once per class via ScriptClassRepBuilder, never mutated after.
   class ScriptClassRep
   {
   public:
      ScriptClassRep(StringTableEntry name,
         const ScriptClassRep* parent,
         Vector<ScriptFieldRep> fields,
         Vector<ScriptMethodRep> methods,
         Vector<ScriptStaticFieldRep> staticFields = {},
         Vector<ScriptStaticMethodRep> staticMethods = {})
         : mName(name)
         , mParent(parent)
         , mFields(std::move(fields))
         , mMethods(std::move(methods))
         , mStaticFields(std::move(staticFields))
         , mStaticMethods(std::move(staticMethods))
      {
      }

      StringTableEntry getName() const { return mName; }
      const ScriptClassRep* getParent() const { return mParent; }
      const Vector<ScriptFieldRep>& getFields() const { return mFields; }
      const Vector<ScriptMethodRep>& getMethods() const { return mMethods; }
      const Vector<ScriptStaticFieldRep>& getStaticFields() const { return mStaticFields; }
      const Vector<ScriptStaticMethodRep>& getStaticMethods() const { return mStaticMethods; }

      const ScriptFieldRep* findField(StringTableEntry name) const;
      const ScriptMethodRep* findMethod(StringTableEntry name) const;
      const ScriptStaticFieldRep* findStaticField(StringTableEntry name) const;
      const ScriptStaticMethodRep* findStaticMethod(StringTableEntry name) const;

   private:
      StringTableEntry mName;
      const ScriptClassRep* mParent;
      Vector<ScriptFieldRep> mFields;
      Vector<ScriptMethodRep> mMethods;
      Vector<ScriptStaticFieldRep> mStaticFields;
      Vector<ScriptStaticMethodRep> mStaticMethods;
   };

   /// One exported free function, not tied to a class.
   struct HostFunctionDecl
   {
      StringTableEntry name;
      const char* usage;
      U32 argCount;
      ScriptValue(*invoke)(ScriptValueSpan args);
   };

   /// Read-only view every language's binding generator consumes to
   /// produce its own native bindings.
   class HostBindingRegistry
   {
   public:
      static HostBindingRegistry& instance();

      /// @return false if a class with this name is already registered
      ///   (checked in every build - class registration happens once at
      ///   startup, and a silently-dropped duplicate must not depend on
      ///   assert builds to catch).
      bool registerClass(const ScriptClassRep* rep);
      void registerFunction(HostFunctionDecl decl);
      const Vector<const ScriptClassRep*>& allClasses() const { return mClasses; }
      const Vector<HostFunctionDecl>& globalFunctions() const { return mFunctions; }
      const ScriptClassRep* find(StringTableEntry className) const;

   private:
      Vector<const ScriptClassRep*> mClasses;
      Vector<HostFunctionDecl> mFunctions;
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_HOSTBINDING_H_
