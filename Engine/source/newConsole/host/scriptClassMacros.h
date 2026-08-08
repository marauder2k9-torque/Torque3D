#ifndef _NEWCONSOLE_SCRIPTCLASSMACROS_H_
#define _NEWCONSOLE_SCRIPTCLASSMACROS_H_

#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTTYPETRAITS_H_
#include "newConsole/host/scriptTypeTraits.h"
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
///    // real implementation - the one and only place this signature's
///    // body lives. Script and C++ callers both end up here.
/// }
///
/// // A static-only class with no instance at all - e.g. an adapter
/// // registry in the shape of the engine's existing GFXInit - uses
/// // SCRIPT_CLASS_ROOT and the SCRIPT_STATIC_* macros instead:
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
/// Mechanism: SCRIPT_FIELDS/SCRIPT_FIELD_EX/SCRIPT_METHOD (and their
/// SCRIPT_STATIC_* counterparts) each declare a static registrar object
/// inside the class body. Every registrar's constructor appends to
/// ScriptSelf::__scriptClassBuildState() - a function-local static Vector
/// set, constructed on first call via ordinary Meyer's-singleton
/// semantics (see core/util/tSingleton.h for the same pattern used
/// elsewhere in the engine). This sidesteps cross-translation-unit static
/// initialization order entirely: nothing depends on these registrars
/// running before getScriptClassRep() is first called, because
/// __scriptClassBuildState() constructs itself (empty) the first time
/// anything touches it, whether that's a registrar appending or
/// getScriptClassRep() reading. What matters is only that every registrar
/// for a class runs before that class's own getScriptClassRep() is first
/// called - guaranteed because the registrars are static-storage-duration
/// members of the class itself and complete their initialization no
/// later than the point at which any code outside the class's own
/// translation unit could plausibly call a static member function on it.

namespace newConsole
{
   namespace detail
   {

      /// Per-class accumulation state, returned by reference so every
      /// registrar for a given ScriptSelf appends to the same instance
      /// regardless of which translation unit it runs in.
      struct ScriptClassBuildState
      {
         Vector<ScriptFieldRep> fields;
         Vector<ScriptMethodRep> methods;
         Vector<ScriptStaticFieldRep> staticFields;
         Vector<ScriptStaticMethodRep> staticMethods;
      };

      /// Resolves ParentT::getScriptClassRep() when ParentT actually declares
      /// one (i.e. is itself SCRIPT_CLASS-annotated), or returns nullptr for a
      /// root class with no reflected parent. Detected via void_t + if
      /// constexpr rather than a tag-dispatch overload pair - an overload pair
      /// (one SFINAE'd on decltype(...), one variadic fallback) is ambiguous
      /// the moment both become viable through a wrapping call, which void_t
      /// detection into a single function template avoids by construction.
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

/// Declares the reflection entry point for a class, and the per-class
/// accumulation point every SCRIPT_FIELDS/SCRIPT_FIELD_EX/SCRIPT_METHOD
/// registrar in this class appends to. Must appear once, first, in the
/// class body. Use this for any class with a reflected C++ base (i.e.
/// parentClassName itself uses SCRIPT_CLASS or SCRIPT_CLASS_ROOT). For a
/// class with no such base at all - an API root like GfxDevice/
/// SoundSystem/PhysicsWorld that must work with zero dependency on this
/// hierarchy - use SCRIPT_CLASS_ROOT instead, which takes no parent.
#define SCRIPT_CLASS(className, parentClassName)                              \
   public:                                                                    \
   using ScriptSelf = className;                                              \
   using ScriptParent = parentClassName;                                      \
   static ::newConsole::detail::ScriptClassBuildState& __scriptClassBuildState() \
   {                                                                          \
      static ::newConsole::detail::ScriptClassBuildState state;              \
      return state;                                                          \
   }                                                                          \
   static const ::newConsole::ScriptClassRep* getScriptClassRep()

/// Same as SCRIPT_CLASS, for a class with no reflected parent at all -
/// getParent() on the resulting ScriptClassRep is unconditionally
/// nullptr, with no ParentT named or required. This is the entry point
/// for standalone API roots (GfxDevice, SoundSystem, PhysicsWorld, ...)
/// that must not be forced to invent a fake C++ base purely to satisfy a
/// macro parameter.
#define SCRIPT_CLASS_ROOT(className)                                          \
   public:                                                                    \
   using ScriptSelf = className;                                              \
   static ::newConsole::detail::ScriptClassBuildState& __scriptClassBuildState() \
   {                                                                          \
      static ::newConsole::detail::ScriptClassBuildState state;              \
      return state;                                                          \
   }                                                                          \
   static const ::newConsole::ScriptClassRep* getScriptClassRep()

/// Out-of-line definition matching SCRIPT_CLASS's declaration. Builds one
/// ScriptClassRep from whatever __scriptClassBuildState() holds at first
/// call - by construction, every SCRIPT_FIELDS/SCRIPT_METHOD registrar
/// for this class has already run, since they are static members of the
/// class itself.
///
/// @note The class name is interned via StringTable->insert(#className),
///   not passed as a raw string literal - every other name in this
///   reflection layer (field names, method names) is interned the same
///   way specifically so ScriptClassRep::findField/findMethod/
///   HostBindingRegistry::find can compare StringTableEntry values by
///   pointer equality rather than strcmp. A raw literal here would be a
///   different pointer than what StringTable->insert() returns for an
///   identical string elsewhere, silently breaking every by-name lookup
///   against this class.
#define SCRIPT_CLASS_BEGIN(className)                                         \
   const ::newConsole::ScriptClassRep* className::getScriptClassRep()         \
   {                                                                          \
      static const ::newConsole::ScriptClassRep __rep = [&]() {              \
         ::newConsole::detail::ScriptClassBuildState& __state =              \
            __scriptClassBuildState();                                       \
         return ::newConsole::ScriptClassRep(                                \
            ::StringTable->insert(#className),                              \
            ::newConsole::detail::parentRepOrNull<ScriptParent>(),          \
            std::move(__state.fields),                                      \
            std::move(__state.methods),                                     \
            std::move(__state.staticFields),                                \
            std::move(__state.staticMethods));

/// Matching begin/end pair for a class declared with SCRIPT_CLASS_ROOT -
/// identical to SCRIPT_CLASS_BEGIN except it never references
/// ScriptParent, which SCRIPT_CLASS_ROOT does not define.
#define SCRIPT_CLASS_ROOT_BEGIN(className)                                    \
   const ::newConsole::ScriptClassRep* className::getScriptClassRep()         \
   {                                                                          \
      static const ::newConsole::ScriptClassRep __rep = [&]() {              \
         ::newConsole::detail::ScriptClassBuildState& __state =              \
            __scriptClassBuildState();                                       \
         return ::newConsole::ScriptClassRep(                                \
            ::StringTable->insert(#className),                              \
            nullptr,                                                        \
            std::move(__state.fields),                                      \
            std::move(__state.methods),                                     \
            std::move(__state.staticFields),                                \
            std::move(__state.staticMethods));

/// Emits the auto-registration static for a class's ScriptClassRep -
/// shared tail for SCRIPT_CLASS_END, parameterized on className so the
/// registrar type name and the getScriptClassRep() call are both
/// unambiguous. Safe regardless of static-init order: getScriptClassRep()
/// is a Meyer's singleton (see SCRIPT_CLASS_BEGIN/ROOT_BEGIN above), so
/// this registrar's constructor can call it at any point in program
/// startup, from any translation unit, in any order relative to other
/// __NC_AUTOREG statics, and always observes a fully-built ScriptClassRep
/// - construction happens on first touch, not at a predetermined time.
#define __NC_AUTOREG(className)                                               \
   namespace                                                                  \
   {                                                                          \
      struct __NC_CONCAT(__NC_AutoReg_, className)                           \
      {                                                                      \
         __NC_CONCAT(__NC_AutoReg_, className)()                             \
         {                                                                   \
            ::newConsole::HostBindingRegistry::instance().registerClass(     \
               className::getScriptClassRep());                             \
         }                                                                   \
      };                                                                     \
      static __NC_CONCAT(__NC_AutoReg_, className) __NC_CONCAT(__nc_autoReg_, className); \
   }

/// @param className Must match the className passed to the matching
///   SCRIPT_CLASS_BEGIN/SCRIPT_CLASS_ROOT_BEGIN. Closes out
///   getScriptClassRep()'s definition and, immediately after, emits the
///   static that registers this class into HostBindingRegistry - every
///   SCRIPT_CLASS/SCRIPT_CLASS_ROOT class becomes visible to every
///   IScriptRuntime automatically, with no separate registration call
///   required anywhere else.
#define SCRIPT_CLASS_END(className)                                           \
      }();                                                                   \
      return &__rep;                                                         \
   }                                                                          \
   __NC_AUTOREG(className)

/// FOR_EACH-style variadic expansion: applies macro `m` to each argument
/// in `...`, supporting up to 16 fields per SCRIPT_FIELDS invocation.
/// Standard preprocessor recursion trick - no runtime component, this is
/// pure token-pasting so SCRIPT_FIELDS can expand "name1, name2, name3"
/// into one push_back(...) per name without a real reflection facility
/// (C++17 has none) to iterate a name list at compile time.
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

/// One field's push_back, used as the per-argument expansion in
/// SCRIPT_FIELDS below. Deliberately identical in shape to what
/// SCRIPT_FIELD_EX generates for a single field, minus usage text/network
/// metadata (use SCRIPT_FIELD_EX instead of SCRIPT_FIELDS for a field
/// that needs either).
#define __NC_PUSH_FIELD(fieldName)                                            \
   __scriptClassBuildState().fields.push_back(                               \
      (::newConsole::detail::makeField<ScriptSelf, &ScriptSelf::fieldName>)  \
         (#fieldName, ""));

/// Registers a run of plain data fields for script/network export, using
/// each name only once - type is deduced from &ScriptSelf::fieldName via
/// ScriptTypeTraits<T>, not authored separately. Expands to a single
/// static registrar whose constructor appends one ScriptFieldRep per
/// listed name into __scriptClassBuildState().fields. Fields needing
/// usage text or NetFieldAttribute use SCRIPT_FIELD_EX instead.
///
/// @note May be invoked more than once in the same class body - the
///   registrar struct's name is derived from __LINE__ (via
///   __NC_CONCAT), so two SCRIPT_FIELDS calls at different source lines
///   never collide. A single class growing its field list over several
///   SCRIPT_FIELDS(...) calls interspersed with other members is
///   supported by design, not just tolerated.
#define SCRIPT_FIELDS(...)                                                    \
   static inline struct __NC_CONCAT(__ScriptFieldsReg_, __LINE__)            \
   {                                                                          \
      __NC_CONCAT(__ScriptFieldsReg_, __LINE__)()                           \
      {                                                                      \
         __NC_FOR_EACH(__NC_PUSH_FIELD, __VA_ARGS__)                        \
      }                                                                      \
   } __NC_CONCAT(__scriptFieldsReg_, __LINE__) {}

/// Registers one field with explicit usage text and/or NetFieldAttribute,
/// in place of that field appearing in SCRIPT_FIELDS. Order relative to
/// SCRIPT_FIELDS does not matter - both append to the same
/// __scriptClassBuildState().fields.
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

/// Declares AND registers a method for script export in one line. Expands
/// to the real member function declaration plus a static registrar whose
/// constructor appends a trampoline built from &ScriptSelf::methodName -
/// write the definition exactly as you would for any ordinary method;
/// there is no second signature to keep in sync.
#define SCRIPT_METHOD(returnType, methodName, args)                           \
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

/// One static field's push_back, used as the per-argument expansion in
/// SCRIPT_STATIC_FIELDS below. Note &ScriptSelf::fieldName here takes
/// the address of a static data member - a plain FieldT*, not a
/// pointer-to-member - which is exactly why this goes through
/// makeStaticField rather than makeField (see StaticFieldAccessor's
/// comment in scriptTypeTraits.h).
#define __NC_PUSH_STATIC_FIELD(fieldName)                                     \
   __scriptClassBuildState().staticFields.push_back(                         \
      (::newConsole::detail::makeStaticField<&ScriptSelf::fieldName>)        \
         (#fieldName));

/// Registers a run of static data fields for script export - the
/// SCRIPT_CLASS_ROOT counterpart to SCRIPT_FIELDS, for a class with no
/// instance (a static-only export-scope class, e.g. an adapter registry
/// in the shape of the engine's existing GFXInit). Type is deduced from
/// &ScriptSelf::fieldName; usage text needing SCRIPT_STATIC_FIELD_EX
/// follows the same relationship SCRIPT_FIELD_EX has to SCRIPT_FIELDS.
#define SCRIPT_STATIC_FIELDS(...)                                             \
   static inline struct __NC_CONCAT(__ScriptStaticFieldsReg_, __LINE__)      \
   {                                                                          \
      __NC_CONCAT(__ScriptStaticFieldsReg_, __LINE__)()                     \
      {                                                                      \
         __NC_FOR_EACH(__NC_PUSH_STATIC_FIELD, __VA_ARGS__)                 \
      }                                                                      \
   } __NC_CONCAT(__scriptStaticFieldsReg_, __LINE__) {}

/// Registers one static field with explicit usage text, in place of that
/// field appearing in SCRIPT_STATIC_FIELDS.
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

/// Declares AND registers a static method for script export - the
/// SCRIPT_CLASS_ROOT counterpart to SCRIPT_METHOD. Expands to the real
/// static member function declaration plus a registrar; the definition
/// is written normally, exactly once, same as SCRIPT_METHOD.
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

#endif // !_NEWCONSOLE_SCRIPTCLASSMACROS_H_
