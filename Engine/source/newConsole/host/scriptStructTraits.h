#ifndef _NEWCONSOLE_SCRIPTSTRUCTTRAITS_H_
#define _NEWCONSOLE_SCRIPTSTRUCTTRAITS_H_

#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTTYPETRAITS_H_
#include "newConsole/host/scriptTypeTraits.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTCLASSMACROS_H_
#include "newConsole/host/scriptClassMacros.h"
#endif
#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>

// Fixed-shape struct-valued script types (VectorF, AngAxisF, ...) -
// %v.x access, %v.method() calls, free functions. See SCRIPT_STRUCT below.

namespace newConsole
{

   // One named component - "x" at index 0, "y" at index 1, etc.
   struct ScriptStructFieldRep
   {
      StringTableEntry name;
      U32 componentIndex;
   };

   // One exported instance method. No ScriptObject self - self is the
   // ScriptValue itself, by reference so an in-place method can rewrite it.
   struct ScriptStructMethodRep
   {
      StringTableEntry name;
      U32 argCount;
      ScriptValue(*invoke)(ScriptValue& self, ScriptValueSpan args);
   };

   // One exported static/free function (VectorF::cross -> cross(%a, %b)).
   struct ScriptStructStaticMethodRep
   {
      StringTableEntry name;
      U32 argCount;
      ScriptValue(*invoke)(ScriptValueSpan args);
   };

   // Reflection descriptor for one SCRIPT_STRUCT type. Built once at
   // static init, never mutated - same lifetime as ScriptClassRep.
   class StructTypeRep
   {
   public:
      StructTypeRep(StringTableEntry name,
         Vector<ScriptStructFieldRep> fields,
         Vector<ScriptStructMethodRep> methods,
         Vector<ScriptStructStaticMethodRep> staticMethods)
         : mName(name)
         , mFields(std::move(fields))
         , mMethods(std::move(methods))
         , mStaticMethods(std::move(staticMethods))
      {
      }

      StringTableEntry getName() const { return mName; }
      U32 componentCount() const { return static_cast<U32>(mFields.size()); }
      const Vector<ScriptStructFieldRep>& getFields() const { return mFields; }
      const Vector<ScriptStructMethodRep>& getMethods() const { return mMethods; }
      const Vector<ScriptStructStaticMethodRep>& getStaticMethods() const { return mStaticMethods; }

      const ScriptStructFieldRep* findField(StringTableEntry name) const
      {
         for (const ScriptStructFieldRep& f : mFields)
            if (f.name == name)
               return &f;
         return nullptr;
      }

      const ScriptStructMethodRep* findMethod(StringTableEntry name) const
      {
         for (const ScriptStructMethodRep& m : mMethods)
            if (m.name == name)
               return &m;
         return nullptr;
      }

      const ScriptStructStaticMethodRep* findStaticMethod(StringTableEntry name) const
      {
         for (const ScriptStructStaticMethodRep& m : mStaticMethods)
            if (m.name == name)
               return &m;
         return nullptr;
      }

   private:
      StringTableEntry mName;
      Vector<ScriptStructFieldRep> mFields;
      Vector<ScriptStructMethodRep> mMethods;
      Vector<ScriptStructStaticMethodRep> mStaticMethods;
   };

   // Maps a C++ type to its StructTypeRep. Mirrors EnumRegistry.
   class StructTypeRegistry
   {
   public:
      static StructTypeRegistry& instance();

      // Returns the stored rep, or nullptr if T was already registered.
      template<typename T>
      const StructTypeRep* registerType(StructTypeRep rep)
      {
         return registerTypeImpl(std::type_index(typeid(T)), std::move(rep));
      }

      template<typename T>
      const StructTypeRep* find() const
      {
         return findImpl(std::type_index(typeid(T)));
      }

   private:
      const StructTypeRep* registerTypeImpl(std::type_index key, StructTypeRep rep);
      const StructTypeRep* findImpl(std::type_index key) const;

      std::unordered_map<std::type_index, StructTypeRep> mTypes;
   };

   namespace detail
   {

      // One component's read/write pair. Same shape as FieldAccessor's
      // get/set, but free functions instead of a ScriptObject self.
      template<typename T>
      struct StructComponentOps
      {
         ScriptValue(*read)(const T&);
         bool (*write)(T&, const ScriptValue&);
      };

      // Per-T component table, populated once by SCRIPT_STRUCT's registrar.
      template<typename T>
      Vector<StructComponentOps<T>>& structComponentOps()
      {
         static Vector<StructComponentOps<T>> sOps;
         return sOps;
      }

      // Deduces a struct member's type, same as MemberPointerTraits.
      template<typename ClassT, auto Member>
      using StructMemberType = typename MemberPointerTraits<ClassT, Member>::Type;

      template<typename T, typename FieldT, FieldT T::* Member>
      ScriptValue readStructComponent(const T& v)
      {
         return ScriptTypeTraits<FieldT>::toScript(v.*Member);
      }

      template<typename T, typename FieldT, FieldT T::* Member>
      bool writeStructComponent(T& out, const ScriptValue& componentValue)
      {
         return ScriptTypeTraits<FieldT>::fromScript(out.*Member, componentValue);
      }

      // Per-T cached StructTypeRep pointer.
      template<typename T>
      const StructTypeRep*& structTypeRepFor()
      {
         static const StructTypeRep* sRep = nullptr;
         return sRep;
      }

      template<typename T>
      void registerStructTraits(const StructTypeRep* rep, Vector<StructComponentOps<T>> componentOps)
      {
         structComponentOps<T>() = std::move(componentOps);
         structTypeRepFor<T>() = rep;
      }

      // ScriptTypeTraits<T>::toScript for any SCRIPT_STRUCT type - builds
      // one Kind::Struct value from the registered component readers.
      template<typename T>
      ScriptValue structToScript(const T& v)
      {
         const Vector<StructComponentOps<T>>& ops = structComponentOps<T>();
         Vector<ScriptValue> components;
         components.setSize(ops.size());
         for (U32 i = 0; i < static_cast<U32>(ops.size()); ++i)
            components[i] = ops[i].read(v);
         return ScriptValue::makeStruct(structTypeRepFor<T>(), std::move(components));
      }

      // ScriptTypeTraits<T>::fromScript for any SCRIPT_STRUCT type.
      // Accepts a matching Kind::Struct value, or a plain Kind::Array
      // with enough elements (e.g. [1, 2, 3] for a VectorF).
      template<typename T>
      bool structFromScript(T& out, const ScriptValue& v)
      {
         if (v.kind() != ScriptValue::Kind::Struct && v.kind() != ScriptValue::Kind::Array)
            return false;
         if (v.kind() == ScriptValue::Kind::Struct && v.structType() != structTypeRepFor<T>())
            return false;

         const Vector<StructComponentOps<T>>& ops = structComponentOps<T>();
         ScriptValue vCopy = v; // arrayRef() is non-const
         Vector<ScriptValue>& components = vCopy.arrayRef();
         if (static_cast<U32>(components.size()) < static_cast<U32>(ops.size()))
            return false;

         for (U32 i = 0; i < static_cast<U32>(ops.size()); ++i)
         {
            if (!ops[i].write(out, components[i]))
               return false;
         }
         return true;
      }

      // Trampoline for a struct instance method. Re-serializes self
      // through ScriptTypeTraits<T> before and after the call so any
      // mutation is visible in self's own components afterward.
      template<typename T, auto Member> struct StructMethodTraits;

      template<typename T, typename ReturnT, typename... ArgTs, ReturnT(T::* Member)(ArgTs...)>
      struct StructMethodTraits<T, Member>
      {
         template<std::size_t... Is>
         static ScriptValue invokeImpl(ScriptValue& self, ScriptValueSpan args, std::index_sequence<Is...>)
         {
            T typed{};
            if (!ScriptTypeTraits<T>::fromScript(typed, self))
               return ScriptValue::makeError("struct method: self did not convert to its own C++ type");

            bool anyFailed = false;
            std::tuple<ArgTs...> converted{ convertArg<ArgTs>(args, static_cast<U32>(Is), anyFailed)... };
            if (anyFailed)
               return ScriptValue::makeError("struct method: argument type/arity mismatch");

            if constexpr (std::is_void_v<ReturnT>)
            {
               std::apply([&typed](ArgTs... a) { (typed.*Member)(a...); }, converted);
               self = ScriptTypeTraits<T>::toScript(typed);
               return ScriptValue::makeNull();
            }
            else
            {
               ReturnT result = std::apply([&typed](ArgTs... a) { return (typed.*Member)(a...); }, converted);
               self = ScriptTypeTraits<T>::toScript(typed);
               return ScriptTypeTraits<ReturnT>::toScript(result);
            }
         }

         static ScriptValue invoke(ScriptValue& self, ScriptValueSpan args)
         {
            return invokeImpl(self, args, std::index_sequence_for<ArgTs...>{});
         }
      };

      // const overload - read-only methods skip the write-back.
      template<typename T, typename ReturnT, typename... ArgTs, ReturnT(T::* Member)(ArgTs...) const>
      struct StructMethodTraits<T, Member>
      {
         template<std::size_t... Is>
         static ScriptValue invokeImpl(const ScriptValue& self, ScriptValueSpan args, std::index_sequence<Is...>)
         {
            T typed{};
            if (!ScriptTypeTraits<T>::fromScript(typed, self))
               return ScriptValue::makeError("struct method: self did not convert to its own C++ type");

            bool anyFailed = false;
            std::tuple<ArgTs...> converted{ convertArg<ArgTs>(args, static_cast<U32>(Is), anyFailed)... };
            if (anyFailed)
               return ScriptValue::makeError("struct method: argument type/arity mismatch");

            if constexpr (std::is_void_v<ReturnT>)
            {
               std::apply([&typed](ArgTs... a) { (typed.*Member)(a...); }, converted);
               return ScriptValue::makeNull();
            }
            else
            {
               ReturnT result = std::apply([&typed](ArgTs... a) { return (typed.*Member)(a...); }, converted);
               return ScriptTypeTraits<ReturnT>::toScript(result);
            }
         }

         static ScriptValue invoke(ScriptValue& self, ScriptValueSpan args)
         {
            return invokeImpl(self, args, std::index_sequence_for<ArgTs...>{});
         }
      };

      template<typename T, auto Member>
      ScriptStructMethodRep makeStructMethod(const char* name)
      {
         ScriptStructMethodRep rep;
         rep.name = StringTable->insert(name);
         rep.argCount = 0; // arity enforced per-argument by convertArg
         rep.invoke = &StructMethodTraits<T, Member>::invoke;
         return rep;
      }

      // Trampoline for a struct type's static/free function - no self.
      template<auto Member> struct StructStaticMethodTraits;

      template<typename ReturnT, typename... ArgTs, ReturnT(*Member)(ArgTs...)>
      struct StructStaticMethodTraits<Member>
      {
         template<std::size_t... Is>
         static ScriptValue invokeImpl(ScriptValueSpan args, std::index_sequence<Is...>)
         {
            bool anyFailed = false;
            std::tuple<ArgTs...> converted{ convertArg<ArgTs>(args, static_cast<U32>(Is), anyFailed)... };
            if (anyFailed)
               return ScriptValue::makeError("struct static method: argument type/arity mismatch");

            if constexpr (std::is_void_v<ReturnT>)
            {
               Member(std::get<Is>(converted)...);
               return ScriptValue::makeNull();
            }
            else
            {
               ReturnT result = Member(std::get<Is>(converted)...);
               return ScriptTypeTraits<ReturnT>::toScript(result);
            }
         }

         static ScriptValue invoke(ScriptValueSpan args)
         {
            return invokeImpl(args, std::index_sequence_for<ArgTs...>{});
         }
      };

      template<auto Member>
      ScriptStructStaticMethodRep makeStructStaticMethod(const char* name)
      {
         ScriptStructStaticMethodRep rep;
         rep.name = StringTable->insert(name);
         rep.argCount = 0;
         rep.invoke = &StructStaticMethodTraits<Member>::invoke;
         return rep;
      }

   } // namespace detail

} // namespace newConsole

// Wraps component names as one group, e.g. SCRIPT_STRUCT_FIELDS(x, y, z).
// Only meaningful as a SCRIPT_STRUCT argument. Required, always first.
#define SCRIPT_STRUCT_FIELDS(...) (__VA_ARGS__)

// Wraps instance method names (%v.method()). Optional - omit the
// argument entirely if there are none. Params/returns must be by value.
#define SCRIPT_STRUCT_METHODS(...) (__VA_ARGS__)

// Wraps static/free function names (cross(%a, %b)). Optional, by-value only.
#define SCRIPT_STRUCT_STATIC_METHODS(...) (__VA_ARGS__)

#define __NC_PUSH_STRUCT_FIELD(structType, fieldName)                        \
   __fields.push_back(::newConsole::ScriptStructFieldRep{                   \
      ::StringTable->insert(#fieldName),                                    \
      static_cast<U32>(__fields.size()) });                                 \
   __componentOps.push_back(::newConsole::detail::StructComponentOps<structType>{ \
      &::newConsole::detail::readStructComponent<structType,                \
         ::newConsole::detail::StructMemberType<structType, &structType::fieldName>, \
         &structType::fieldName>,                                           \
      &::newConsole::detail::writeStructComponent<structType,               \
         ::newConsole::detail::StructMemberType<structType, &structType::fieldName>, \
         &structType::fieldName> });

#define __NC_PUSH_STRUCT_METHOD(structType, methodName)                      \
   __methods.push_back(                                                      \
      ::newConsole::detail::makeStructMethod<structType, &structType::methodName>(#methodName));

#define __NC_PUSH_STRUCT_STATIC_METHOD(structType, methodName)               \
   __staticMethods.push_back(                                                \
      ::newConsole::detail::makeStructStaticMethod<&structType::methodName>(#methodName));

// Same threading trick as SCRIPT_ENUM's FE ladder, so each pusher call
// sees structType without capturing from an enclosing scope.
#define __NC_STRUCT_FE_1(m,t,x)  m(t,x)
#define __NC_STRUCT_FE_2(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_1(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_3(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_2(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_4(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_3(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_5(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_4(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_6(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_5(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_7(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_6(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_8(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_7(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_9(m,t,x,...)  m(t,x) __NC_EXPAND(__NC_STRUCT_FE_8(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_10(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_9(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_11(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_10(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_12(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_11(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_13(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_12(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_14(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_13(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_15(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_14(m,t,__VA_ARGS__))
#define __NC_STRUCT_FE_16(m,t,x,...) m(t,x) __NC_EXPAND(__NC_STRUCT_FE_15(m,t,__VA_ARGS__))
#define __NC_STRUCT_FOR_EACH(m, structType, ...)                             \
   __NC_EXPAND(__NC_GET_MACRO(__VA_ARGS__,                                  \
      __NC_STRUCT_FE_16,__NC_STRUCT_FE_15,__NC_STRUCT_FE_14,__NC_STRUCT_FE_13, \
      __NC_STRUCT_FE_12,__NC_STRUCT_FE_11,__NC_STRUCT_FE_10,__NC_STRUCT_FE_9, \
      __NC_STRUCT_FE_8,__NC_STRUCT_FE_7,__NC_STRUCT_FE_6,__NC_STRUCT_FE_5,   \
      __NC_STRUCT_FE_4,__NC_STRUCT_FE_3,__NC_STRUCT_FE_2,__NC_STRUCT_FE_1)   \
      (m, structType, __VA_ARGS__))

// Strips one paren layer ("(x,y,z)" -> "x,y,z") and threads it through FOR_EACH.
#define __NC_STRUCT_STRIP(...) __VA_ARGS__
#define __NC_STRUCT_UNWRAP_FE_(m, structType, ...) __NC_STRUCT_FOR_EACH(m, structType, __VA_ARGS__)
#define __NC_STRUCT_UNWRAP_FE(m, structType, group) \
   __NC_STRUCT_UNWRAP_FE_(m, structType, __NC_STRUCT_STRIP group)

// Registers structType as a script struct type. Dispatched by argument
// count (2/3/4); 3rd arg is always SCRIPT_STRUCT_METHODS.
#define __NC_STRUCT_PICK(_1,_2,_3,_4,NAME,...) NAME
#define SCRIPT_STRUCT(...) \
   __NC_EXPAND(__NC_STRUCT_PICK(__VA_ARGS__, __NC_STRUCT_IMPL_4, __NC_STRUCT_IMPL_3, __NC_STRUCT_IMPL_2)(__VA_ARGS__))

#define __NC_STRUCT_BODY_BEGIN(structType)                                   \
   namespace newConsole { namespace detail {                                \
      template<> struct ScriptTypeTraits<structType>                       \
      {                                                                      \
         static ScriptValue toScript(const structType& v) { return ::newConsole::detail::structToScript<structType>(v); } \
         static bool fromScript(structType& out, const ScriptValue& v) { return ::newConsole::detail::structFromScript<structType>(out, v); } \
      };                                                                     \
   } }                                                                       \
   namespace                                                                 \
   {                                                                         \
      struct __NC_CONCAT(__NC_StructReg_, __LINE__)                         \
      {                                                                      \
         __NC_CONCAT(__NC_StructReg_, __LINE__)()                           \
         {                                                                   \
            Vector<::newConsole::ScriptStructFieldRep> __fields;             \
            Vector<::newConsole::detail::StructComponentOps<structType>> __componentOps; \
            Vector<::newConsole::ScriptStructMethodRep> __methods;           \
            Vector<::newConsole::ScriptStructStaticMethodRep> __staticMethods;

#define __NC_STRUCT_BODY_END(structType)                                     \
            const ::newConsole::StructTypeRep* __rep =                      \
               ::newConsole::StructTypeRegistry::instance().registerType<structType>( \
                  ::newConsole::StructTypeRep(::StringTable->insert(#structType), \
                     __fields, std::move(__methods), std::move(__staticMethods))); \
            ::newConsole::detail::registerStructTraits<structType>(         \
               __rep, std::move(__componentOps));                          \
            for (U32 __i = 0; __i < static_cast<U32>(__rep->getStaticMethods().size()); ++__i) \
            {                                                                \
               const ::newConsole::ScriptStructStaticMethodRep& __sm = __rep->getStaticMethods()[__i]; \
               ::newConsole::HostFunctionDecl __decl;                       \
               __decl.name = __sm.name;                                     \
               __decl.usage = "";                                           \
               __decl.argCount = __sm.argCount;                             \
               __decl.invoke = __sm.invoke;                                 \
               ::newConsole::HostBindingRegistry::instance().registerFunction(__decl); \
            }                                                               \
         }                                                                   \
      };                                                                     \
      static __NC_CONCAT(__NC_StructReg_, __LINE__) __NC_CONCAT(__nc_structReg_, __LINE__); \
   }

#define __NC_STRUCT_IMPL_2(structType, fieldsGroup)                          \
   __NC_STRUCT_BODY_BEGIN(structType)                                       \
   __NC_STRUCT_UNWRAP_FE(__NC_PUSH_STRUCT_FIELD, structType, fieldsGroup)    \
   __NC_STRUCT_BODY_END(structType)

#define __NC_STRUCT_IMPL_3(structType, fieldsGroup, methodsGroup)            \
   __NC_STRUCT_BODY_BEGIN(structType)                                       \
   __NC_STRUCT_UNWRAP_FE(__NC_PUSH_STRUCT_FIELD, structType, fieldsGroup)    \
   __NC_STRUCT_UNWRAP_FE(__NC_PUSH_STRUCT_METHOD, structType, methodsGroup)  \
   __NC_STRUCT_BODY_END(structType)

#define __NC_STRUCT_IMPL_4(structType, fieldsGroup, methodsGroup, staticMethodsGroup) \
   __NC_STRUCT_BODY_BEGIN(structType)                                       \
   __NC_STRUCT_UNWRAP_FE(__NC_PUSH_STRUCT_FIELD, structType, fieldsGroup)    \
   __NC_STRUCT_UNWRAP_FE(__NC_PUSH_STRUCT_METHOD, structType, methodsGroup)  \
   __NC_STRUCT_UNWRAP_FE(__NC_PUSH_STRUCT_STATIC_METHOD, structType, staticMethodsGroup) \
   __NC_STRUCT_BODY_END(structType)

#endif // !_NEWCONSOLE_SCRIPTSTRUCTTRAITS_H_
