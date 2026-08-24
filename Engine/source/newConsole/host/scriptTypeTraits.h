#ifndef _NEWCONSOLE_SCRIPTTYPETRAITS_H_
#define _NEWCONSOLE_SCRIPTTYPETRAITS_H_

#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTOBJECT_H_
#include "newConsole/host/scriptObject.h"
#endif
#ifndef _NEWCONSOLE_OBJECTREGISTRY_H_
#include "newConsole/host/objectRegistry.h"
#endif
#ifndef _NEWCONSOLE_ENUMREGISTRY_H_
#include "newConsole/host/enumRegistry.h"
#endif
#ifndef _NEWCONSOLE_NOTIFYFIELD_H_
#include "newConsole/host/notifyField.h"
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

      /// Per-C++-type ScriptValue conversion, one specialization per
      /// supported type. SCRIPT_FIELDS/SCRIPT_METHOD always go through
      /// this rather than hand-writing conversions.
      ///
      /// @note Second template param only exists to support the
      ///   enum_class SFINAE specialization below.
      template<typename T, typename = void> struct ScriptTypeTraits;

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

      /// Object references marshal as ObjectHandle (id/generation pair),
      /// never a formatted id string. Resolved via ObjectRegistry since
      /// ScriptObject itself carries no identity - see scriptValue.h.
      template<typename T>
      struct ScriptTypeTraits<T*>
      {
         static ScriptValue toScript(T* v)
         {
            if (!v) return ScriptValue::makeNull();
            return ScriptValue::makeObject(ObjectRegistry::instance().registerObject(v));
         }
         static bool fromScript(T*& out, const ScriptValue& v)
         {
            ObjectHandle handle;
            if (!v.tryGet<ObjectHandle>(handle)) return false;
            out = static_cast<T*>(ObjectRegistry::instance().resolve(handle));
            return true;
         }
      };

      /// Generic conversion for any SCRIPT_ENUM-registered enum class - one
      /// template covers every enum, no per-enum specialization needed.
      /// toScript reads back the registered name, falling back to the raw
      /// int if unregistered/unmatched. fromScript accepts name or int.
      template<typename T>
      struct ScriptTypeTraits<T, std::enable_if_t<std::is_enum_v<T>>>
      {
         static ScriptValue toScript(T v)
         {
            S64 raw = static_cast<S64>(v);
            const EnumTypeInfo* info = EnumRegistry::instance().find<T>();
            if (info)
            {
               StringTableEntry name = info->findByValue(raw);
               if (name)
                  return ScriptValue::makeString(name);
            }
            // Unregistered, or no matching enumerator - fall back to raw int.
            return ScriptValue::makeInt(raw);
         }

         static bool fromScript(T& out, const ScriptValue& v)
         {
            const EnumTypeInfo* info = EnumRegistry::instance().find<T>();

            if (v.kind() == ScriptValue::Kind::String && info)
            {
               const char* s = "";
               v.tryGet<const char*>(s);
               S64 value = 0;
               if (info->findByName(StringTable->insert(s), value))
               {
                  out = static_cast<T>(value);
                  return true;
               }
               return false; // string given but matched no enumerator
            }

            S64 raw = 0;
            if (!v.tryGet<S64>(raw))
               return false;
            out = static_cast<T>(raw);
            return true;
         }
      };

      /// Field get/set as real addressable functions (Member is a non-type
      /// template param, so &FieldAccessor<...>::get/set are plain function
      /// pointers with no closure state, matching ScriptFieldRep's shape).
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

      /// Field accessor backed by getter/setter methods instead of a data
      /// member - getter can compute, setter can validate/reject.
      /// @note Setter may return bool (accept/reject) or void (always
      ///   succeeds) - two specializations below cover both.
      template<typename ClassT, typename FieldT, ScriptValue(ClassT::* Getter)() const, bool (ClassT::* Setter)(FieldT)>
      struct CustomFieldAccessorBoolSetter
      {
         static ScriptValue get(const ScriptObject* self)
         {
            const ClassT* typed = static_cast<const ClassT*>(self);
            return (typed->*Getter)();
         }
         static bool set(ScriptObject* self, const ScriptValue& value)
         {
            ClassT* typed = static_cast<ClassT*>(self);
            FieldT converted{};
            if (!ScriptTypeTraits<FieldT>::fromScript(converted, value))
               return false;
            return (typed->*Setter)(converted);
         }
      };

      template<typename ClassT, typename FieldT, ScriptValue(ClassT::* Getter)() const, void (ClassT::* Setter)(FieldT)>
      struct CustomFieldAccessorVoidSetter
      {
         static ScriptValue get(const ScriptObject* self)
         {
            const ClassT* typed = static_cast<const ClassT*>(self);
            return (typed->*Getter)();
         }
         static bool set(ScriptObject* self, const ScriptValue& value)
         {
            ClassT* typed = static_cast<ClassT*>(self);
            FieldT converted{};
            if (!ScriptTypeTraits<FieldT>::fromScript(converted, value))
               return false;
            (typed->*Setter)(converted);
            return true;
         }
      };

      /// Builds a ScriptFieldRep from getter/setter methods. Getter returns
      /// ScriptValue directly so it can return a derived/computed value.
      template<typename ClassT, typename FieldT, ScriptValue(ClassT::* Getter)() const, void (ClassT::* Setter)(FieldT)>
      ScriptFieldRep makeCustomField(const char* name, const char* usage, NetFieldAttribute network = NetFieldAttribute())
      {
         ScriptFieldRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.network = network;
         rep.get = &CustomFieldAccessorVoidSetter<ClassT, FieldT, Getter, Setter>::get;
         rep.set = &CustomFieldAccessorVoidSetter<ClassT, FieldT, Getter, Setter>::set;
         return rep;
      }

      template<typename ClassT, typename FieldT, ScriptValue(ClassT::* Getter)() const, bool (ClassT::* Setter)(FieldT)>
      ScriptFieldRep makeCustomField(const char* name, const char* usage, NetFieldAttribute network = NetFieldAttribute())
      {
         ScriptFieldRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.network = network;
         rep.get = &CustomFieldAccessorBoolSetter<ClassT, FieldT, Getter, Setter>::get;
         rep.set = &CustomFieldAccessorBoolSetter<ClassT, FieldT, Getter, Setter>::set;
         return rep;
      }

      /// Field accessor for a NotifyField<T> member - writes via operator=,
      /// which fires onFieldChanged (see notifyField.h).
      template<typename ClassT, typename T, NotifyField<T> ClassT::* Member>
      struct NotifyFieldAccessor
      {
         static ScriptValue get(const ScriptObject* self)
         {
            const ClassT* typed = static_cast<const ClassT*>(self);
            return ScriptTypeTraits<T>::toScript((typed->*Member).get());
         }

         static bool set(ScriptObject* self, const ScriptValue& value)
         {
            ClassT* typed = static_cast<ClassT*>(self);
            T converted{};
            if (!ScriptTypeTraits<T>::fromScript(converted, value))
               return false;
            (typed->*Member) = std::move(converted);
            return true;
         }
      };

      /// Builds a ScriptFieldRep from a NotifyField<T> member pointer.
      template<typename ClassT, typename T, NotifyField<T> ClassT::* Member>
      ScriptFieldRep makeNotifyFieldImpl(const char* name, const char* usage, NetFieldAttribute network)
      {
         ScriptFieldRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.network = network;
         rep.get = &NotifyFieldAccessor<ClassT, T, Member>::get;
         rep.set = &NotifyFieldAccessor<ClassT, T, Member>::set;
         return rep;
      }

      /// Builds one ScriptFieldRep from a real member pointer; type deduced from the pointer.
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

      /// Deduces FieldT from a member-pointer expression for makeField below.
      /// @note Only one specialization (any FieldT ClassT::*) - a second one
      ///   for NotifyField<T> is ambiguous (confirmed by testing).
      ///   IsNotifyFieldDetector distinguishes the two cases after deduction.
      template<typename ClassT, auto Member>
      struct MemberPointerTraits;

      template<typename ClassT, typename FieldT, FieldT ClassT::* Member>
      struct MemberPointerTraits<ClassT, Member>
      {
         using Type = FieldT;
      };

      template<typename FieldT>
      struct IsNotifyFieldDetector : std::false_type
      {
         using InnerType = FieldT;
      };

      template<typename T>
      struct IsNotifyFieldDetector<NotifyField<T>> : std::true_type
      {
         using InnerType = T;
      };

      /// Dispatches to makeNotifyFieldImpl or makeFieldImpl automatically -
      /// callers never need to know which kind of member Member is.
      template<typename ClassT, auto Member>
      ScriptFieldRep makeField(const char* name, const char* usage,
         NetFieldAttribute network = NetFieldAttribute())
      {
         using FieldT = typename MemberPointerTraits<ClassT, Member>::Type;
         using Detector = IsNotifyFieldDetector<FieldT>;
         if constexpr (Detector::value)
            return makeNotifyFieldImpl<ClassT, typename Detector::InnerType, Member>(name, usage, network);
         else
            return makeFieldImpl<ClassT, FieldT, Member>(name, usage, network);
      }

      /// Static-field accessor, no self param - for SCRIPT_CLASS_ROOT classes.
      /// @note Member is a plain FieldT* (static member address), not a pointer-to-member.
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

      /// Deduces FieldT from a static member's address - mirrors makeField for instance members.
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

      /// Peels one C++ parameter off a ScriptValueSpan at index I, via
      /// ScriptTypeTraits<ArgT>. On failure returns a default ArgT and sets
      /// anyFailed - caller reports all failures, not just the first.
      template<typename ArgT>
      ArgT convertArg(ScriptValueSpan args, U32 index, bool& anyFailed)
      {
         ArgT out{};
         if (index >= args.size() || !ScriptTypeTraits<ArgT>::fromScript(out, args[index]))
            anyFailed = true;
         return out;
      }

      /// Like convertArg, but for a trailing optional arg: falls back to
      /// defaultValue if the call didn't supply it. A real type mismatch on
      /// a supplied argument still fails.
      /// @note defaultValue comes from the SCRIPT_METHOD call site, not the
      ///   real C++ default - keeping them in sync is the author's job.
      template<typename ArgT>
      ArgT convertArgWithDefault(ScriptValueSpan args, U32 index, ArgT defaultValue, bool& anyFailed)
      {
         if (index >= args.size())
            return defaultValue;
         ArgT out{};
         if (!ScriptTypeTraits<ArgT>::fromScript(out, args[index]))
            anyFailed = true;
         return out;
      }

      /// Deduces ClassT/ReturnT/ArgTs... from a member function pointer and
      /// builds the trampoline in one partial specialization.
      template<auto Member> struct MethodPointerTraits;

      template<typename ClassT, typename ReturnT, typename... ArgTs, ReturnT(ClassT::* Member)(ArgTs...)>
      struct MethodPointerTraits<Member>
      {
         template<std::size_t... Is>
         static ScriptValue invokeImpl(ClassT* self, ScriptValueSpan args, std::index_sequence<Is...>)
         {
            bool anyFailed = false;
            // Convert all args before calling - call evaluation order is
            // unspecified, and every type error must be reported.
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

      /// Splits a member-function-pointer type into class/return/args + an
      /// index_sequence - MethodWithDefaultsTraits::invoke can't deduce
      /// these itself since Member is a class template arg.
      template<typename MemberPtr> struct MemberFunctionTraits;

      template<typename ClassT, typename ReturnT, typename... ArgTs>
      struct MemberFunctionTraits<ReturnT(ClassT::*)(ArgTs...)>
      {
         using ClassType = ClassT;
         using ReturnType = ReturnT;
         using IndexSeq = std::index_sequence_for<ArgTs...>;
         using ArgsTuple = std::tuple<ArgTs...>;
      };

      /// Trampoline for a method where every argument has a default -
      /// accepts 0 to sizeof...(ArgTs) args, each missing one falling back
      /// to its own DefaultsTuple entry. Matches torquescript2's own
      /// default-argument semantics.
      /// @note DefaultsTuple element types must match ArgTs... exactly -
      ///   values convert implicitly at registration time.
      template<auto Member>
      struct MethodWithDefaultsTraits
      {
         template<typename ClassT, typename ReturnT, typename... ArgTs, std::size_t... Is>
         static ScriptValue invokeImpl(ClassT* self, ScriptValueSpan args, const std::tuple<ArgTs...>& defaults,
            ReturnT(ClassT::* member)(ArgTs...), std::index_sequence<Is...>)
         {
            bool anyFailed = false;
            std::tuple<ArgTs...> converted{
               convertArgWithDefault<ArgTs>(args, static_cast<U32>(Is), std::get<Is>(defaults), anyFailed)...
            };
            if (anyFailed)
               return ScriptValue::makeError("argument type mismatch");

            if constexpr (std::is_void_v<ReturnT>)
            {
               std::apply([self, member](ArgTs... a) { (self->*member)(a...); }, converted);
               return ScriptValue::makeNull();
            }
            else
            {
               ReturnT result = std::apply([self, member](ArgTs... a) { return (self->*member)(a...); }, converted);
               return ScriptTypeTraits<ReturnT>::toScript(result);
            }
         }

         template<typename... ArgTs>
         static ScriptValue invoke(ScriptObject* self, ScriptValueSpan args, const std::tuple<ArgTs...>& defaults)
         {
            using Traits = MemberFunctionTraits<decltype(Member)>;
            typename Traits::ClassType* typed = static_cast<typename Traits::ClassType*>(self);
            return invokeImpl(typed, args, defaults, Member, typename Traits::IndexSeq{});
         }
      };

      template<auto Member, typename... ArgTs>
      ScriptMethodRep makeMethodWithDefaults(const char* name, const char* usage, std::tuple<ArgTs...> defaults)
      {
         using DefaultsTuple = std::tuple<ArgTs...>;
         // Leaked, permanent - ScriptMethodRep::invoke is a plain function
         // pointer with no captured state. Registered once, never torn down.
         static DefaultsTuple* sDefaults = new DefaultsTuple(std::move(defaults));

         ScriptMethodRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.argCount = 0; // every argument is optional
         rep.invoke = +[](ScriptObject* self, ScriptValueSpan args) -> ScriptValue
         {
            return MethodWithDefaultsTraits<Member>::invoke(self, args, *sDefaults);
         };
         return rep;
      }

      /// Overload taking raw default values (e.g. (0, "test")) - this is
      /// the one SCRIPT_METHOD actually calls. DefaultsTuple deduced from
      /// Member, so values convert implicitly the same as a normal call.
      template<auto Member, typename... RawValues>
      ScriptMethodRep makeMethodWithDefaultsFromValues(const char* name, const char* usage, RawValues&&... values)
      {
         using Traits = MemberFunctionTraits<decltype(Member)>;
         using DefaultsTuple = typename Traits::ArgsTuple;
         return makeMethodWithDefaults<Member>(name, usage, DefaultsTuple(std::forward<RawValues>(values)...));
      }

      template<auto Member>
      ScriptMethodRep makeMethod(const char* name, const char* usage = "")
      {
         ScriptMethodRep rep;
         rep.name = StringTable->insert(name);
         rep.usage = usage;
         rep.argCount = 0; // arity enforced per-argument by convertArg
         rep.invoke = &MethodPointerTraits<Member>::invoke;
         return rep;
      }

      /// Static-method trampoline - same idea as MethodPointerTraits, for a
      /// plain `static ReturnT ClassT::method(ArgTs...)` with no self.
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

      /// Same trampoline as makeStaticMethod, targeting HostFunctionDecl for GLOBAL_SCRIPT_METHOD.
      template<auto Member>
      HostFunctionDecl makeGlobalFunction(const char* name, const char* usage = "")
      {
         HostFunctionDecl decl;
         decl.name = StringTable->insert(name);
         decl.usage = usage;
         decl.argCount = 0;
         decl.invoke = &StaticMethodPointerTraits<Member>::invoke;
         return decl;
      }

   } // namespace detail
} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTTYPETRAITS_H_
