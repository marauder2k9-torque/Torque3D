#ifndef _NEWCONSOLE_SCRIPTCLASSMACROS_H_
#define _NEWCONSOLE_SCRIPTCLASSMACROS_H_

#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTTYPETRAITS_H_
#include "newConsole/host/scriptTypeTraits.h"
#endif
#ifndef _NEWCONSOLE_CLASSFACTORY_H_
#include "newConsole/host/classFactory.h"
#endif

#include <type_traits>

/// @file
/// Declaration-side ergonomics for exposing a class to script.
///
/// @code
/// class Explosion : public ScriptSimObject
/// {
/// public:
///    SCRIPT_CLASS(Explosion, ScriptSimObject);
///    SCRIPT_FIELDS(mRadius, mDamage, mEffectName);
///
///    F32 mRadius;
///    S32 mDamage;
///    StringTableEntry mEffectName;
///
///    SCRIPT_METHOD(void, applyDamage, (ScriptObject* target, F32 falloff));
/// };
///
/// SCRIPT_CLASS_BEGIN(Explosion)
/// SCRIPT_CLASS_END(Explosion)
///
/// void Explosion::applyDamage(ScriptObject* target, F32 falloff)
/// {
///    // real implementation - script and C++ both call this
/// }
///
/// // Static-only class, no instance - use SCRIPT_CLASS_ROOT + SCRIPT_STATIC_*:
/// class GfxInit
/// {
/// public:
///    SCRIPT_CLASS_ROOT(GfxInit);
///    SCRIPT_STATIC_FIELDS(smAdapterCount);
///
///    static S32 smAdapterCount;
///
///    SCRIPT_STATIC_METHOD(void, enumerateAdapters, ());
/// };
///
/// SCRIPT_CLASS_ROOT_BEGIN(GfxInit)
/// SCRIPT_CLASS_END(GfxInit)
///
/// void GfxInit::enumerateAdapters() { /* ... */ }
/// @endcode
///
/// Mechanism: each SCRIPT_FIELDS/SCRIPT_FIELD_EX/SCRIPT_METHOD declares a
/// static registrar in the class body whose constructor appends to
/// ScriptSelf::__scriptClassBuildState() (Meyer's-singleton Vector set,
/// see tSingleton.h). Avoids static-init-order issues: every registrar
/// runs before its class's own getScriptClassRep() can be called, since
/// registrars are static class members.

namespace newConsole
{
   namespace detail
   {

      /// Per-class accumulation state; one instance per ScriptSelf,
      /// shared across translation units.
      struct ScriptClassBuildState
      {
         Vector<ScriptFieldRep> fields;
         Vector<ScriptMethodRep> methods;
         Vector<ScriptStaticFieldRep> staticFields;
         Vector<ScriptStaticMethodRep> staticMethods;
      };

      /// True if ParentT declares getScriptClassRep() (i.e. is itself
      /// SCRIPT_CLASS-annotated); false for a root class with no parent.
      template<typename T, typename = void>
      struct HasGetScriptClassRep : std::false_type {};

      template<typename T>
      struct HasGetScriptClassRep<T, std::void_t<decltype(T::getScriptClassRep())>> : std::true_type {};

      template<typename ParentT>
      const ScriptClassRep* parentRepOrNull()
      {
         if constexpr (HasGetScriptClassRep<ParentT>::value)
            return ParentT::getScriptClassRep();
         else
            return nullptr;
      }

   } // namespace detail
} // namespace newConsole

/// Declares the reflection entry point and per-class field/method
/// accumulator. Must appear once, first, in the class body. Use for a
/// class with a reflected C++ base; use SCRIPT_CLASS_ROOT for an API
/// root with no such base.
#define SCRIPT_CLASS(className, parentClassName)                              \
   public:                                                                    \
   using ScriptSelf = className;                                              \
   using ScriptParent = parentClassName;                                      \
   static ::newConsole::detail::ScriptClassBuildState& __scriptClassBuildState() \
   {                                                                          \
      static ::newConsole::detail::ScriptClassBuildState state;              \
      return state;                                                          \
   }                                                                          \
   const ::newConsole::ScriptClassRep* getRuntimeClassRep() const override    \
   {                                                                          \
      return getScriptClassRep();                                            \
   }                                                                          \
   static const ::newConsole::ScriptClassRep* getScriptClassRep()

/// Same as SCRIPT_CLASS but with no reflected parent - getParent() is
/// always nullptr. Use for standalone API roots (GfxDevice, SoundSystem, ...).
#define SCRIPT_CLASS_ROOT(className)                                          \
   public:                                                                    \
   using ScriptSelf = className;                                              \
   static ::newConsole::detail::ScriptClassBuildState& __scriptClassBuildState() \
   {                                                                          \
      static ::newConsole::detail::ScriptClassBuildState state;              \
      return state;                                                          \
   }                                                                          \
   static const ::newConsole::ScriptClassRep* getScriptClassRep()

namespace newConsole
{
   namespace detail
   {

      /// Strips leading "a::b::" namespace qualification from a
      /// stringified class name. Needed because #className stringifies
      /// the fully-qualified name; script code expects the bare name.
      inline const char* bareClassName(const char* possiblyQualified)
      {
         const char* lastSep = nullptr;
         for (const char* p = possiblyQualified; *p; ++p)
         {
            if (p[0] == ':' && p[1] == ':')
            {
               lastSep = p + 2;
               ++p; // skip the second ':' too
            }
         }
         return lastSep ? lastSep : possiblyQualified;
      }

   } // namespace detail
} // namespace newConsole

/// Out-of-line definition matching SCRIPT_CLASS's declaration. Builds
/// one ScriptClassRep from __scriptClassBuildState() at first call -
/// all field/method registrars have already run by then.
#define SCRIPT_CLASS_BEGIN(className)                                         \
   const ::newConsole::ScriptClassRep* className::getScriptClassRep()         \
   {                                                                          \
      static const ::newConsole::ScriptClassRep __rep = [&]() {              \
         ::newConsole::detail::ScriptClassBuildState& __state =              \
            __scriptClassBuildState();                                       \
         return ::newConsole::ScriptClassRep(                                \
            ::StringTable->insert(::newConsole::detail::bareClassName(#className)), \
            ::newConsole::detail::parentRepOrNull<ScriptParent>(),          \
            std::move(__state.fields),                                      \
            std::move(__state.methods),                                     \
            std::move(__state.staticFields),                                \
            std::move(__state.staticMethods));

/// SCRIPT_CLASS_ROOT counterpart to SCRIPT_CLASS_BEGIN - never
/// references ScriptParent.
#define SCRIPT_CLASS_ROOT_BEGIN(className)                                    \
   const ::newConsole::ScriptClassRep* className::getScriptClassRep()         \
   {                                                                          \
      static const ::newConsole::ScriptClassRep __rep = [&]() {              \
         ::newConsole::detail::ScriptClassBuildState& __state =              \
            __scriptClassBuildState();                                       \
         return ::newConsole::ScriptClassRep(                                \
            ::StringTable->insert(::newConsole::detail::bareClassName(#className)), \
            nullptr,                                                        \
            std::move(__state.fields),                                      \
            std::move(__state.methods),                                     \
            std::move(__state.staticFields),                                \
            std::move(__state.staticMethods));

/// Auto-registration static for a class's ScriptClassRep. Safe
/// regardless of static-init order since getScriptClassRep() is a
/// Meyer's singleton.
#define __NC_AUTOREG(className)                                               \
   namespace                                                                  \
   {                                                                          \
      struct __NC_CONCAT(__NC_AutoReg_, __LINE__)                            \
      {                                                                      \
         __NC_CONCAT(__NC_AutoReg_, __LINE__)()                              \
         {                                                                   \
            ::newConsole::HostBindingRegistry::instance().registerClass(     \
               className::getScriptClassRep());                             \
            ::newConsole::detail::registerDefaultConstructorIfPossible<className>( \
               className::getScriptClassRep()->getName());                  \
         }                                                                   \
      };                                                                     \
      static __NC_CONCAT(__NC_AutoReg_, __LINE__) __NC_CONCAT(__nc_autoReg_, __LINE__); \
   }

/// @param className Must match the SCRIPT_CLASS_BEGIN/ROOT_BEGIN param.
///   Closes getScriptClassRep() and registers the class with
///   HostBindingRegistry.
#define SCRIPT_CLASS_END(className)                                           \
      }();                                                                   \
      return &__rep;                                                         \
   }                                                                          \
   __NC_AUTOREG(className)

/// FOR_EACH-style variadic expansion, up to 16 args. Pure token-pasting
/// preprocessor recursion (no C++17 compile-time iteration available).
#define __NC_CONCAT_(a,b) a##b
#define __NC_CONCAT(a,b) __NC_CONCAT_(a,b)
#define __NC_EXPAND(x) x
#define __NC_GET_MACRO(_1,_2,_3,_4,_5,_6,_7,_8,_9,_10,_11,_12,_13,_14,_15,_16,NAME,...) NAME
#define __NC_FE_1(m,x)  m(x)
#define __NC_FE_2(m,x,...)  m(x) __NC_EXPAND(__NC_FE_1(m,__VA_ARGS__))
#define __NC_FE_3(m,x,...)  m(x) __NC_EXPAND(__NC_FE_2(m,__VA_ARGS__))
#define __NC_FE_4(m,x,...)  m(x) __NC_EXPAND(__NC_FE_3(m,__VA_ARGS__))
#define __NC_FE_5(m,x,...)  m(x) __NC_EXPAND(__NC_FE_4(m,__VA_ARGS__))
#define __NC_FE_6(m,x,...)  m(x) __NC_EXPAND(__NC_FE_5(m,__VA_ARGS__))
#define __NC_FE_7(m,x,...)  m(x) __NC_EXPAND(__NC_FE_6(m,__VA_ARGS__))
#define __NC_FE_8(m,x,...)  m(x) __NC_EXPAND(__NC_FE_7(m,__VA_ARGS__))
#define __NC_FE_9(m,x,...)  m(x) __NC_EXPAND(__NC_FE_8(m,__VA_ARGS__))
#define __NC_FE_10(m,x,...) m(x) __NC_EXPAND(__NC_FE_9(m,__VA_ARGS__))
#define __NC_FE_11(m,x,...) m(x) __NC_EXPAND(__NC_FE_10(m,__VA_ARGS__))
#define __NC_FE_12(m,x,...) m(x) __NC_EXPAND(__NC_FE_11(m,__VA_ARGS__))
#define __NC_FE_13(m,x,...) m(x) __NC_EXPAND(__NC_FE_12(m,__VA_ARGS__))
#define __NC_FE_14(m,x,...) m(x) __NC_EXPAND(__NC_FE_13(m,__VA_ARGS__))
#define __NC_FE_15(m,x,...) m(x) __NC_EXPAND(__NC_FE_14(m,__VA_ARGS__))
#define __NC_FE_16(m,x,...) m(x) __NC_EXPAND(__NC_FE_15(m,__VA_ARGS__))
#define __NC_FOR_EACH(m, ...)                                                 \
   __NC_EXPAND(__NC_GET_MACRO(__VA_ARGS__,                                   \
      __NC_FE_16,__NC_FE_15,__NC_FE_14,__NC_FE_13,__NC_FE_12,__NC_FE_11,     \
      __NC_FE_10,__NC_FE_9,__NC_FE_8,__NC_FE_7,__NC_FE_6,__NC_FE_5,          \
      __NC_FE_4,__NC_FE_3,__NC_FE_2,__NC_FE_1)(m, __VA_ARGS__))

/// One field's push_back for SCRIPT_FIELDS. Same shape as
/// SCRIPT_FIELD_EX minus usage text/net metadata.
#define __NC_PUSH_FIELD(fieldName)                                            \
   __scriptClassBuildState().fields.push_back(                               \
      (::newConsole::detail::makeField<ScriptSelf, &ScriptSelf::fieldName>)  \
         (#fieldName, ""));

/// Registers a run of plain data fields for script/network export; type
/// deduced from &ScriptSelf::fieldName. Use SCRIPT_FIELD_EX for usage
/// text or NetFieldAttribute.
///
/// @note May be invoked more than once per class body - registrar name
///   is keyed on __LINE__, so calls never collide.
#define SCRIPT_FIELDS(...)                                                    \
   static inline struct __NC_CONCAT(__ScriptFieldsReg_, __LINE__)            \
   {                                                                          \
      __NC_CONCAT(__ScriptFieldsReg_, __LINE__)()                           \
      {                                                                      \
         __NC_FOR_EACH(__NC_PUSH_FIELD, __VA_ARGS__)                        \
      }                                                                      \
   } __NC_CONCAT(__scriptFieldsReg_, __LINE__) {}

/// Registers a field backed by getter/setter methods rather than a data
/// member - use when reading computes something or writing needs to
/// validate/reject.
///
/// getterMethod takes no args, returns ScriptValue. setterMethod takes
/// one arg of the field's type, returns bool (false rejects) or void.
///
/// @code
/// class Health : public ScriptObject {
///    SCRIPT_CLASS(Health, ScriptObject);
///    SCRIPT_FIELD_CUSTOM(mHealth, "current health", S32, getHealth, setHealth);
///    S32 mRawHealth = 100;
///    ScriptValue getHealth() const { return ScriptValue::makeInt(mRawHealth); }
///    bool setHealth(S32 v) { if (v < 0) return false; mRawHealth = v; return true; }
/// };
/// @endcode
#define SCRIPT_FIELD_CUSTOM(fieldName, usageText, fieldType, getterMethod, setterMethod, ...) \
   static inline struct __ScriptFieldReg_##fieldName                         \
   {                                                                          \
      __ScriptFieldReg_##fieldName()                                         \
      {                                                                      \
         __scriptClassBuildState().fields.push_back(                        \
            (::newConsole::detail::makeCustomField<ScriptSelf, fieldType,   \
               &ScriptSelf::getterMethod, &ScriptSelf::setterMethod>)       \
               (#fieldName, usageText, ##__VA_ARGS__));                     \
      }                                                                      \
   } __scriptFieldReg_##fieldName{}

/// Registers one field with explicit usage text and/or NetFieldAttribute,
/// in place of appearing in SCRIPT_FIELDS. Order doesn't matter.
#define SCRIPT_FIELD_EX(fieldName, usageText, ...)                            \
   static inline struct __ScriptFieldReg_##fieldName                         \
   {                                                                          \
      __ScriptFieldReg_##fieldName()                                         \
      {                                                                      \
         __scriptClassBuildState().fields.push_back(                        \
            (::newConsole::detail::makeField<ScriptSelf, &ScriptSelf::fieldName>) \
               (#fieldName, usageText, ##__VA_ARGS__));                     \
      }                                                                      \
   } __scriptFieldReg_##fieldName{}

/// Declares a NotifyField<type> member AND registers it for script
/// export in one call. Fires ScriptObject::onFieldChanged on every
/// write, C++ or script - see notifyField.h.
///
/// Defaults to NetFieldAttribute::alwaysDirty(). Use ADD_FIELD_NET for
/// an explicit dirty-mask bit, or NetFieldAttribute() to exclude from
/// networking.
///
/// @code
/// ADD_FIELD(S32, mHealth, "current health", 100);
/// @endcode
#define ADD_FIELD(type, fieldName, usageText, initialValue)                  \
   ::newConsole::NotifyField<type> fieldName{                               \
      this, ::StringTable->insert(#fieldName),                             \
      ::newConsole::NetFieldAttribute::alwaysDirty().dirtyMask, (initialValue) }; \
   SCRIPT_FIELD_EX(fieldName, usageText, ::newConsole::NetFieldAttribute::alwaysDirty())

/// Same as ADD_FIELD with an explicit NetFieldAttribute - use for a
/// dirty-mask bit, specific wire encoding, or opting out of networking.
///
/// @code
/// ADD_FIELD_NET(S32, mHealth, "current health", 100, NetFieldAttribute::fixed(0x01, 8));
/// @endcode
#define ADD_FIELD_NET(type, fieldName, usageText, initialValue, netAttr)     \
   ::newConsole::NotifyField<type> fieldName{                               \
      this, ::StringTable->insert(#fieldName), (netAttr).dirtyMask, (initialValue) }; \
   SCRIPT_FIELD_EX(fieldName, usageText, netAttr)

/// Declares AND registers a method for script export in one line. Write
/// the definition normally - no second signature to keep in sync.
///
/// Optional 4th argument gives every parameter a default value (all
/// params need one if used, matching torquescript2's own defaults):
///
/// @code
/// SCRIPT_METHOD(S32, addToValue, (S32 amount, const char* label), (0, "test"));
/// @endcode
#define SCRIPT_METHOD(...) \
   __NC_EXPAND(__NC_SCRIPT_METHOD_PICK(__VA_ARGS__, __NC_SCRIPT_METHOD_4, __NC_SCRIPT_METHOD_3)(__VA_ARGS__))
#define __NC_SCRIPT_METHOD_PICK(_1,_2,_3,_4,NAME,...) NAME

#define __NC_SCRIPT_METHOD_3(returnType, methodName, args)                    \
   returnType methodName args;                                                \
   static inline struct __ScriptMethodReg_##methodName                       \
   {                                                                          \
      __ScriptMethodReg_##methodName()                                       \
      {                                                                      \
         __scriptClassBuildState().methods.push_back(                       \
            ::newConsole::detail::makeMethod<&ScriptSelf::methodName>(      \
               #methodName));                                               \
      }                                                                      \
   } __scriptMethodReg_##methodName{}

#define __NC_SCRIPT_METHOD_UNWRAP(...) __VA_ARGS__

#define __NC_SCRIPT_METHOD_4(returnType, methodName, args, defaults)          \
   returnType methodName args;                                                \
   static inline struct __ScriptMethodReg_##methodName                       \
   {                                                                          \
      __ScriptMethodReg_##methodName()                                       \
      {                                                                      \
         __scriptClassBuildState().methods.push_back(                       \
            ::newConsole::detail::makeMethodWithDefaultsFromValues<          \
               &ScriptSelf::methodName>(#methodName, "",                    \
               __NC_EXPAND(__NC_SCRIPT_METHOD_UNWRAP defaults)));            \
      }                                                                      \
   } __scriptMethodReg_##methodName{}

/// One static field's push_back for SCRIPT_STATIC_FIELDS. Note
/// &ScriptSelf::fieldName here is a plain FieldT* (static data member),
/// not a pointer-to-member - hence makeStaticField, not makeField.
#define __NC_PUSH_STATIC_FIELD(fieldName)                                     \
   __scriptClassBuildState().staticFields.push_back(                         \
      (::newConsole::detail::makeStaticField<&ScriptSelf::fieldName>)        \
         (#fieldName));

/// SCRIPT_CLASS_ROOT counterpart to SCRIPT_FIELDS, for static-only
/// classes with no instance. Type deduced from &ScriptSelf::fieldName.
#define SCRIPT_STATIC_FIELDS(...)                                             \
   static inline struct __NC_CONCAT(__ScriptStaticFieldsReg_, __LINE__)      \
   {                                                                          \
      __NC_CONCAT(__ScriptStaticFieldsReg_, __LINE__)()                     \
      {                                                                      \
         __NC_FOR_EACH(__NC_PUSH_STATIC_FIELD, __VA_ARGS__)                 \
      }                                                                      \
   } __NC_CONCAT(__scriptStaticFieldsReg_, __LINE__) {}

/// Registers one static field with explicit usage text, in place of
/// SCRIPT_STATIC_FIELDS.
#define SCRIPT_STATIC_FIELD_EX(fieldName, usageText)                          \
   static inline struct __ScriptStaticFieldReg_##fieldName                   \
   {                                                                          \
      __ScriptStaticFieldReg_##fieldName()                                   \
      {                                                                      \
         __scriptClassBuildState().staticFields.push_back(                  \
            (::newConsole::detail::makeStaticField<&ScriptSelf::fieldName>) \
               (#fieldName, usageText));                                    \
      }                                                                      \
   } __scriptStaticFieldReg_##fieldName{}

/// SCRIPT_CLASS_ROOT counterpart to SCRIPT_METHOD. Definition written
/// normally, exactly once.
#define SCRIPT_STATIC_METHOD(returnType, methodName, args)                    \
   static returnType methodName args;                                        \
   static inline struct __ScriptStaticMethodReg_##methodName                 \
   {                                                                          \
      __ScriptStaticMethodReg_##methodName()                                 \
      {                                                                      \
         __scriptClassBuildState().staticMethods.push_back(                 \
            ::newConsole::detail::makeStaticMethod<&ScriptSelf::methodName>(\
               #methodName));                                               \
      }                                                                      \
   } __scriptStaticMethodReg_##methodName{}

/// Registers a true global function - no owning class. File/namespace
/// scope only, like SCRIPT_ENUM. methodName may be qualified; registers
/// under the bare name.
///
/// @code
/// bool compileScriptTree(const char* srcDir, const char* dstDir);
/// GLOBAL_SCRIPT_METHOD(bool, compileScriptTree, (const char* srcDir, const char* dstDir));
/// @endcode
#define GLOBAL_SCRIPT_METHOD(returnType, methodName, args)                    \
   namespace                                                                  \
   {                                                                          \
      struct __NC_CONCAT(__NC_GlobalMethodReg_, __LINE__)                    \
      {                                                                      \
         __NC_CONCAT(__NC_GlobalMethodReg_, __LINE__)()                      \
         {                                                                   \
            ::newConsole::HostBindingRegistry::instance().registerFunction(  \
               ::newConsole::detail::makeGlobalFunction<&methodName>(        \
                  ::newConsole::detail::bareClassName(#methodName)));        \
         }                                                                   \
      };                                                                     \
      static __NC_CONCAT(__NC_GlobalMethodReg_, __LINE__)                    \
         __NC_CONCAT(__nc_globalMethodReg_, __LINE__);                      \
   }

/// Registers an `enum class` for script marshalling in one line:
/// SCRIPT_ENUM(StateEnum, Idle, Running, Dead). Every SCRIPT_FIELDS/
/// SCRIPT_METHOD using StateEnum picks this up via ScriptTypeTraits<T>'s
/// std::is_enum_v specialization (scriptTypeTraits.h). O(1) lookup both
/// directions, replacing the legacy EngineEnumTable linear scan.
///
/// @note File/namespace scope only, not a class-body macro - write it
///   directly after the enum class's own definition.
/// @note Enumerator names must be bare (not enumType::name), matching
///   SCRIPT_FIELDS convention. Values read via static_cast<S64>.
/// @note __NC_ENUM_ENTRY takes enumType as an explicit macro argument on
///   every expansion since macro params aren't visible across nested
///   macro invocations; the __NC_ENUM_FE_N ladder threads it through.
#define __NC_ENUM_ENTRY(enumType, name) \
   ::newConsole::EnumTypeInfo::Entry{ static_cast<S64>(enumType::name), ::StringTable->insert(#name) },

#define __NC_ENUM_FE_1(enumType,x)  __NC_ENUM_ENTRY(enumType,x)
#define __NC_ENUM_FE_2(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_1(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_3(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_2(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_4(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_3(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_5(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_4(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_6(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_5(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_7(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_6(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_8(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_7(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_9(enumType,x,...)  __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_8(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_10(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_9(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_11(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_10(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_12(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_11(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_13(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_12(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_14(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_13(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_15(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_14(enumType,__VA_ARGS__))
#define __NC_ENUM_FE_16(enumType,x,...) __NC_ENUM_ENTRY(enumType,x) __NC_EXPAND(__NC_ENUM_FE_15(enumType,__VA_ARGS__))
#define __NC_ENUM_FOR_EACH(enumType, ...)                                     \
   __NC_EXPAND(__NC_GET_MACRO(__VA_ARGS__,                                   \
      __NC_ENUM_FE_16,__NC_ENUM_FE_15,__NC_ENUM_FE_14,__NC_ENUM_FE_13,       \
      __NC_ENUM_FE_12,__NC_ENUM_FE_11,__NC_ENUM_FE_10,__NC_ENUM_FE_9,        \
      __NC_ENUM_FE_8,__NC_ENUM_FE_7,__NC_ENUM_FE_6,__NC_ENUM_FE_5,           \
      __NC_ENUM_FE_4,__NC_ENUM_FE_3,__NC_ENUM_FE_2,__NC_ENUM_FE_1)           \
      (enumType, __VA_ARGS__))

#define SCRIPT_ENUM(enumType, ...)                                            \
   namespace                                                                  \
   {                                                                          \
      struct __NC_CONCAT(__NC_EnumReg_, __LINE__)                            \
      {                                                                      \
         __NC_CONCAT(__NC_EnumReg_, __LINE__)()                              \
         {                                                                   \
            ::newConsole::EnumRegistry::instance().registerEnum<enumType>(   \
               #enumType,                                                   \
               { __NC_ENUM_FOR_EACH(enumType, __VA_ARGS__) });              \
         }                                                                   \
      };                                                                     \
      static __NC_CONCAT(__NC_EnumReg_, __LINE__) __NC_CONCAT(__nc_enumReg_, __LINE__); \
   }

#endif // !_NEWCONSOLE_SCRIPTCLASSMACROS_H_
