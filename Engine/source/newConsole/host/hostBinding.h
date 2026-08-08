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

   /// Read/write accessors for one exported instance field, type-erased at
   /// the ScriptValue boundary but generated from a real C++ member pointer.
   /// For a class with no instance at all (SCRIPT_CLASS_ROOT over a
   /// static-only class, e.g. a GFXInit-style adapter registry with no
   /// object to hold a self pointer), use ScriptStaticFieldRep instead - see
   /// its own comment for why this is a distinct type rather than a
   /// nullable-self variant of this one.
   struct ScriptFieldRep
   {
      StringTableEntry name;
      const char* usage;
      ScriptValue(*get)(const ScriptObject* self);
      bool (*set)(ScriptObject* self, const ScriptValue& value);

      /// Absent (default-constructed, dirtyMask == 0) for fields that never
      /// replicate. See NetFieldAttribute::replicates().
      NetFieldAttribute network;
   };

   /// Read/write accessors for one exported static field - no self pointer,
   /// because a static-only reflected class (SCRIPT_CLASS_ROOT over
   /// something like an adapter registry: every member static, no
   /// instance ever created, no lifetime for ScriptObject to manage) has no
   /// instance to receive one.
   ///
   /// @note Deliberately a separate type from ScriptFieldRep rather than
   ///   the same struct with a self parameter that's simply unused for the
   ///   static case. A get/set pair that silently ignores an argument it
   ///   was handed is exactly the kind of implicit, easy-to-misuse-by-
   ///   accident shape the rest of this reflection layer avoids elsewhere
   ///   (see ScriptValue's explicit-conversion-only design) - a caller
   ///   holding a ScriptStaticFieldRep has no self to pass in the first
   ///   place, and the type of the accessor makes that fact visible at the
   ///   call site instead of hidden behind a parameter nobody reads.
   ///
   /// @note Network replication does not apply to static fields - there is
   ///   no per-instance dirty state for something with no instance, so this
   ///   carries no NetFieldAttribute.
   struct ScriptStaticFieldRep
   {
      StringTableEntry name;
      const char* usage;
      ScriptValue(*get)();
      bool (*set)(const ScriptValue& value);
   };

   /// One exported instance method. @a argCount is fixed arity for v1;
   /// variadic export is not yet supported. For a method on a static-only
   /// reflected class, use ScriptStaticMethodRep instead.
   struct ScriptMethodRep
   {
      StringTableEntry name;
      const char* usage;
      U32 argCount;
      ScriptValue(*invoke)(ScriptObject* self, ScriptValueSpan args);
   };

   /// One exported static method - no self pointer, same reasoning as
   /// ScriptStaticFieldRep above. Identical in shape to HostFunctionDecl
   /// (a free function also has no instance); kept as its own type so a
   /// static class method is discoverable as belonging to that class
   /// (via ScriptClassRep::getStaticMethods()) rather than only reachable
   /// through the flat global-function list.
   struct ScriptStaticMethodRep
   {
      StringTableEntry name;
      const char* usage;
      U32 argCount;
      ScriptValue(*invoke)(ScriptValueSpan args);
   };

   /// Reflection descriptor for one SCRIPT_CLASS or SCRIPT_CLASS_ROOT
   /// declared type.
   ///
   /// @note A class built from SCRIPT_CLASS_ROOT over a static-only C++
   ///   class (no instance, no ScriptObject base - see SCRIPT_CLASS_ROOT's
   ///   own comment) populates only the static field/method lists and
   ///   leaves the instance lists empty; the reverse is true for an
   ///   ordinary SCRIPT_CLASS. Nothing prevents a class from populating
   ///   both if it genuinely has both instance and static members, but the
   ///   common case is one or the other.
   ///
   /// @note Built once per class via ScriptClassRepBuilder and never mutated
   ///   after registration.
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

   /// Read-only view every language's binding generator consumes to produce
   /// its own native bindings.
   class HostBindingRegistry
   {
   public:
      static HostBindingRegistry& instance();

      /// @return false if a class with this name is already registered
      ///   (checked in every build, not just debug - class registration
      ///   happens once at startup and a silently-dropped duplicate here
      ///   is exactly the kind of thing that must not depend on assert
      ///   builds to catch).
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
