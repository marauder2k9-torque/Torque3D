#ifndef _NEWCONSOLE_SCRIPTTYPETRAITS_H_
#define _NEWCONSOLE_SCRIPTTYPETRAITS_H_

#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#include "newConsole/host/scriptObject.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif

#include <type_traits>
#include <tuple>
#include <utility>

namespace newConsole
{
   namespace detail
   {

      /// Per-C++-type ScriptValue conversion. Specialized once per supported
      /// type; SCRIPT_FIELDS/SCRIPT_METHOD never hand-write a conversion, they
      /// go through this so a type's script-facing behavior is defined in
      /// exactly one place regardless of how many fields/methods use it.
      template<typename T> struct ScriptTypeTraits;

      template<> struct ScriptTypeTraits<bool>
      {
         static ScriptValue toScript(bool v) { return ScriptValue::makeBool(v); }
         static bool fromScript(bool& out, const ScriptValue& v) { return v.tryGet<bool>(out); }
      };

      template<> struct ScriptTypeTraits<S32>
      {
         static ScriptValue toScript(S32 v) { return ScriptValue::makeInt(v); }
         static bool fromScript(S32& out, const ScriptValue& v)
         {
            S64 wide = 0;
            if (!v.tryGet<S64>(wide)) return false;
            out = static_cast<S32>(wide);
            return true;
         }
      };

      template<> struct ScriptTypeTraits<U32>
      {
         static ScriptValue toScript(U32 v) { return ScriptValue::makeInt(static_cast<S64>(v)); }
         static bool fromScript(U32& out, const ScriptValue& v)
         {
            S64 wide = 0;
            if (!v.tryGet<S64>(wide)) return false;
            out = static_cast<U32>(wide);
            return true;
         }
      };

      template<> struct ScriptTypeTraits<S64>
      {
         static ScriptValue toScript(S64 v) { return ScriptValue::makeInt(v); }
         static bool fromScript(S64& out, const ScriptValue& v) { return v.tryGet<S64>(out); }
      };

      template<> struct ScriptTypeTraits<F32>
      {
         static ScriptValue toScript(F32 v) { return ScriptValue::makeFloat(static_cast<F64>(v)); }
         static bool fromScript(F32& out, const ScriptValue& v)
         {
            F64 wide = 0.0;
            if (!v.tryGet<F64>(wide)) return false;
            out = static_cast<F32>(wide);
            return true;
         }
      };

      template<> struct ScriptTypeTraits<F64>
      {
         static ScriptValue toScript(F64 v) { return ScriptValue::makeFloat(v); }
         static bool fromScript(F64& out, const ScriptValue& v) { return v.tryGet<F64>(out); }
      };

      template<> struct ScriptTypeTraits<StringTableEntry>
      {
         static ScriptValue toScript(StringTableEntry v) { return ScriptValue::makeString(v ? v : ""); }
         static bool fromScript(StringTableEntry& out, const ScriptValue& v)
         {
            const char* s = "";
            if (!v.tryGet<const char*>(s)) return false;
            out = StringTable->insert(s);
            return true;
         }
      };

      /// Object references marshal as ObjectHandle - a plain integer id/generation
      /// pair - never as a formatted-then-parsed id string. See scriptValue.h.
      /// Requires T to provide getObjectHandle() and a static T::resolveHandle().
      template<typename T>
      struct ScriptTypeTraits<T*>
      {
         static ScriptValue toScript(T* v) { return ScriptValue::makeObject(v ? v->getObjectHandle() : ObjectHandle{}); }
         static bool fromScript(T*& out, const ScriptValue& v)
         {
            ObjectHandle handle;
            if (!v.tryGet<ObjectHandle>(handle)) return false;
            out = T::resolveHandle(handle);
            return true;
         }
      };

      /// Field accessor pair generated as real, addressable functions - the
      /// member pointer is a non-type template parameter, not a captured
      /// runtime value, specifically so &FieldAccessor<...>::get and ::set are
      /// genuine plain function pointers with no closure state. This is what
      /// lets ScriptFieldRep::get/set stay plain function pointers (see
      /// hostBinding.h) instead of needing std::function or similar - a field
      /// accessor never allocates and never carries anything beyond the two
      /// function pointers already sitting in ScriptFieldRep.
      template<typename ClassT, typename FieldT, FieldT ClassT::* Member>
      struct FieldAccessor
      {
         static ScriptValue get(const ScriptObject* self)
         {
            const ClassT* typed = static_cast<const ClassT*>(self);
            return ScriptTypeTraits<FieldT>::toScript(typed->*Member);
         }

         static bool set(ScriptObject* self, const ScriptValue& value)
         {
            ClassT* typed = static_cast<ClassT*>(self);
            return ScriptTypeTraits<FieldT>::fromScript(typed->*Member, value);
         }
      };

      /// Builds one ScriptFieldRep from a real member pointer. Type is deduced
      /// from the pointer itself; FieldAccessor above supplies the actual
      /// get/set function pointers.
      template<typename ClassT, typename FieldT, FieldT ClassT::* Member>
      ScriptFieldRep makeFieldImpl(const char* name, const char* usage, NetFieldAttribute network)
      {
         ScriptFieldRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.network = network;
         rep.get = &FieldAccessor<ClassT, FieldT, Member>::get;
         rep.set = &FieldAccessor<ClassT, FieldT, Member>::set;
         return rep;
      }

      /// Deduces FieldT from a member-pointer expression so macro call sites
      /// can write makeField<ScriptSelf, &ScriptSelf::fieldName>(name, usage)
      /// without separately spelling out the field's type - Member is supplied
      /// as an explicit non-type template argument (required so FieldAccessor's
      /// functions have no captured state), FieldT is deduced from Member's own
      /// type via the partial specialization below.
      template<typename ClassT, auto Member>
      struct MemberPointerTraits;

      template<typename ClassT, typename FieldT, FieldT ClassT::* Member>
      struct MemberPointerTraits<ClassT, Member>
      {
         using Type = FieldT;
      };

      template<typename ClassT, auto Member>
      ScriptFieldRep makeField(const char* name, const char* usage,
         NetFieldAttribute network = NetFieldAttribute())
      {
         using FieldT = typename MemberPointerTraits<ClassT, Member>::Type;
         return makeFieldImpl<ClassT, FieldT, Member>(name, usage, network);
      }

      /// Static-field accessor pair - no self parameter, for a plain `static
      /// FieldT ClassT::member` rather than an instance member. Used by
      /// SCRIPT_CLASS_ROOT classes (see scriptClassMacros.h), where there may
      /// be no instance of ClassT anywhere, ever - a static-only export-scope
      /// class like the engine's existing GFXInit pattern.
      ///
      /// @note Member here is a plain `FieldT*` (address of a static data
      ///   member), not a pointer-to-member - static members don't have a
      ///   pointer-to-member type, they're addressed like any other global.
      template<typename FieldT, FieldT* Member>
      struct StaticFieldAccessor
      {
         static ScriptValue get()
         {
            return ScriptTypeTraits<FieldT>::toScript(*Member);
         }

         static bool set(const ScriptValue& value)
         {
            return ScriptTypeTraits<FieldT>::fromScript(*Member, value);
         }
      };

      template<typename FieldT, FieldT* Member>
      ScriptStaticFieldRep makeStaticFieldImpl(const char* name, const char* usage)
      {
         ScriptStaticFieldRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.get = &StaticFieldAccessor<FieldT, Member>::get;
         rep.set = &StaticFieldAccessor<FieldT, Member>::set;
         return rep;
      }

      /// Deduces FieldT from a static member's address so macro call sites can
      /// write makeStaticField<&ScriptSelf::staticFieldName>(name, usage)
      /// without spelling out the type by hand - mirrors makeField's role for
      /// instance members.
      template<auto Member> struct StaticMemberPointerTraits;

      template<typename FieldT, FieldT* Member>
      struct StaticMemberPointerTraits<Member>
      {
         using Type = FieldT;
      };

      template<auto Member>
      ScriptStaticFieldRep makeStaticField(const char* name, const char* usage = "")
      {
         using FieldT = typename StaticMemberPointerTraits<Member>::Type;
         return makeStaticFieldImpl<FieldT, Member>(name, usage);
      }

      /// Peels one C++ parameter type off a ScriptValueSpan at compile-time
      /// index I, converting via ScriptTypeTraits<ArgT>. On conversion failure
      /// returns a default-constructed ArgT rather than failing the whole
      /// call - arity/type mismatches from script are a script-author error to
      /// surface via diagnostics at the call site in the owning IScriptRuntime,
      /// not something this trampoline silently swallows without a trace, so
      /// each ScriptTypeTraits<ArgT>::fromScript failure here ORs into an "any
      /// conversion failed" flag the trampoline checks before actually calling
      /// through to the method body.
      template<typename ArgT>
      ArgT convertArg(ScriptValueSpan args, U32 index, bool& anyFailed)
      {
         ArgT out{};
         if (index >= args.size() || !ScriptTypeTraits<ArgT>::fromScript(out, args[index]))
            anyFailed = true;
         return out;
      }

      /// Deduces ClassT/ReturnT/ArgTs... from a member function pointer type
      /// and generates the ScriptObject-facing trampoline in one partial
      /// specialization - Member, ReturnT and the ArgTs... pack all have to be
      /// deduced together in a single template-argument-deduction context, so
      /// this cannot be split into a separate "trampoline" template taking
      /// Member as its own parameter (a parameter pack must be the last
      /// template parameter; Member's type itself depends on ArgTs..., which
      /// rules out putting Member after the pack in a second template).
      /// makeMethod<&ScriptSelf::methodName>(name) below needs no type spelled
      /// out by hand as a result - the method's own declaration (written once,
      /// by SCRIPT_METHOD) is the only place its signature lives.
      template<auto Member> struct MethodPointerTraits;

      template<typename ClassT, typename ReturnT, typename... ArgTs, ReturnT(ClassT::* Member)(ArgTs...)>
      struct MethodPointerTraits<Member>
      {
         template<std::size_t... Is>
         static ScriptValue invokeImpl(ClassT* self, ScriptValueSpan args, std::index_sequence<Is...>)
         {
            bool anyFailed = false;
            // Args must be converted before the call, not interleaved with it -
            // argument evaluation order for a real call expression is
            // unspecified in C++, and a script-side type error must be
            // detected for every argument (to report all of them, and to
            // guarantee no partially-converted call reaches the real method)
            // rather than however many the compiler happened to evaluate
            // before hitting the first bad one.
            std::tuple<ArgTs...> converted{ convertArg<ArgTs>(args, static_cast<U32>(Is), anyFailed)... };
            if (anyFailed)
               return ScriptValue::makeError("argument type/arity mismatch");

            if constexpr (std::is_void_v<ReturnT>)
            {
               (self->*Member)(std::get<Is>(converted)...);
               return ScriptValue::makeNull();
            }
            else
            {
               ReturnT result = (self->*Member)(std::get<Is>(converted)...);
               return ScriptTypeTraits<ReturnT>::toScript(result);
            }
         }

         static ScriptValue invoke(ScriptObject* self, ScriptValueSpan args)
         {
            ClassT* typed = static_cast<ClassT*>(self);
            return invokeImpl(typed, args, std::index_sequence_for<ArgTs...>{});
         }
      };

      template<auto Member>
      ScriptMethodRep makeMethod(const char* name, const char* usage = "")
      {
         ScriptMethodRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.argCount = 0; // arity is enforced per-argument by convertArg, not pre-checked here
         rep.invoke = &MethodPointerTraits<Member>::invoke;
         return rep;
      }

      /// Static-method trampoline - same reasoning as MethodPointerTraits, but
      /// for a plain `static ReturnT ClassT::method(ArgTs...)` with no self to
      /// dispatch through. Deduced from a plain (non-member) function pointer
      /// type, since a static member function decays to that, not to a
      /// pointer-to-member-function.
      template<auto Member> struct StaticMethodPointerTraits;

      template<typename ReturnT, typename... ArgTs, ReturnT(*Member)(ArgTs...)>
      struct StaticMethodPointerTraits<Member>
      {
         template<std::size_t... Is>
         static ScriptValue invokeImpl(ScriptValueSpan args, std::index_sequence<Is...>)
         {
            bool anyFailed = false;
            std::tuple<ArgTs...> converted{ convertArg<ArgTs>(args, static_cast<U32>(Is), anyFailed)... };
            if (anyFailed)
               return ScriptValue::makeError("argument type/arity mismatch");

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
      ScriptStaticMethodRep makeStaticMethod(const char* name, const char* usage = "")
      {
         ScriptStaticMethodRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.argCount = 0;
         rep.invoke = &StaticMethodPointerTraits<Member>::invoke;
         return rep;
      }

   } // namespace detail
} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTTYPETRAITS_H_
