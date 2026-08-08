#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#define _NEWCONSOLE_SCRIPTVALUE_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include <memory>

namespace newConsole
{

   /// Stable, generation-checked reference to a ScriptObject.
   ///
   /// @note Never holds a raw pointer. Every embedded language wraps this
   ///   instead, so lifetime is governed by ScriptObject's own refcounting
   ///   regardless of which runtime is holding it.
   struct ObjectHandle
   {
      U32 id = 0;
      U32 generation = 0;

      bool isNull() const { return id == 0; }
      bool operator==(const ObjectHandle& other) const { return id == other.id && generation == other.generation; }
      bool operator!=(const ObjectHandle& other) const { return !(*this == other); }
   };

   /// Reference-counted, immutable string payload for ScriptValue.
   ///
   /// @note Cheap to copy (shares storage); never mutated in place.
   class RefCountedString
   {
   public:
      RefCountedString() = default;
      explicit RefCountedString(const char* str) : mData(str ? std::make_shared<String>(str) : nullptr) {}
      explicit RefCountedString(String str) : mData(std::make_shared<String>(std::move(str))) {}

      const char* c_str() const { return mData ? mData->c_str() : ""; }
      U32 length() const { return mData ? mData->length() : 0; }
      bool isEmpty() const { return !mData || mData->isEmpty(); }

   private:
      std::shared_ptr<String> mData;
   };

   /// Tagged value crossing an IScriptRuntime boundary.
   ///
   /// @note Anything richer than this (a CLR List<T>, a Lua table with
   ///   metatables) stays inside its own runtime and crosses only as an
   ///   ObjectHandle, never flattened into Array.
   class ScriptValue
   {
   public:
      enum class Kind : U8
      {
         Null,
         Bool,
         Int,
         Float,
         String,
         Object,
         Array,
         Error
      };

      ScriptValue() : mKind(Kind::Null) { mData.i = 0; }

      static ScriptValue makeNull() { return ScriptValue(); }
      static ScriptValue makeBool(bool v) { ScriptValue r; r.mKind = Kind::Bool; r.mData.b = v; return r; }
      static ScriptValue makeInt(S64 v) { ScriptValue r; r.mKind = Kind::Int; r.mData.i = v; return r; }
      static ScriptValue makeFloat(F64 v) { ScriptValue r; r.mKind = Kind::Float; r.mData.f = v; return r; }
      static ScriptValue makeString(const char* v) { ScriptValue r; r.mKind = Kind::String; r.mStr = RefCountedString(v); return r; }
      static ScriptValue makeObject(ObjectHandle v) { ScriptValue r; r.mKind = Kind::Object; r.mObj = v; return r; }
      static ScriptValue makeError(const char* msg) { ScriptValue r; r.mKind = Kind::Error; r.mStr = RefCountedString(msg); return r; }

      static ScriptValue makeArray()
      {
         ScriptValue r;
         r.mKind = Kind::Array;
         r.mArray = std::make_shared<Vector<ScriptValue>>();
         return r;
      }

      Kind kind() const { return mKind; }
      bool isNull() const { return mKind == Kind::Null; }
      bool isError() const { return mKind == Kind::Error; }

      /// @return true and writes @a out if this value converts to T without loss.
      /// @note Mirrors the conversion rules ScriptTypeTraits<T> uses on the
      ///   C++ reflection side, so both sides of a binding agree.
      /// @note Only ever widens between the *numeric* kinds (Bool/Int/Float),
      ///   and only where the widening is exact (Int -> Float truncation
      ///   above 2^53 is refused, not silently rounded). Never touches String
      ///   in either direction. A value that started as Int stays Int through
      ///   any number of copies/tryGet<Float> calls; nothing in this type
      ///   ever rewrites mKind as a side effect of being read.
      template<typename T> bool tryGet(T& out) const;

      /// Explicit, named, possibly-lossy text conversion. Never called
      /// implicitly by tryGet<T> or by any copy/assignment path — the only
      /// way a numeric value turns into text is a caller asking for it here.
      String toDisplayString() const;

      /// Explicit, named, possibly-lossy parse from text. Only way text turns
      /// into a numeric ScriptValue — never invoked implicitly.
      static ScriptValue parseFromString(Kind targetKind, const char* text);

      Vector<ScriptValue>& arrayRef()
      {
         AssertFatal(mKind == Kind::Array, "ScriptValue::arrayRef - not an array");
         return *mArray;
      }

      const char* errorMessage() const { return mKind == Kind::Error ? mStr.c_str() : ""; }

   private:
      Kind mKind;
      union
      {
         bool b;
         S64  i;
         F64  f;
      } mData;
      RefCountedString mStr;
      ObjectHandle mObj;
      std::shared_ptr<Vector<ScriptValue>> mArray;
   };

   template<> inline bool ScriptValue::tryGet<bool>(bool& out) const
   {
      switch (mKind)
      {
      case Kind::Bool:  out = mData.b; return true;
      case Kind::Int:   out = (mData.i != 0); return true;
      case Kind::Float: out = (mData.f != 0.0); return true;
      default: return false;
      }
   }

   template<> inline bool ScriptValue::tryGet<S64>(S64& out) const
   {
      switch (mKind)
      {
      case Kind::Int:  out = mData.i; return true;
      case Kind::Bool: out = mData.b ? 1 : 0; return true;
      case Kind::Float:
         // Only exact if the float has no fractional part and fits
         // losslessly in S64 - refuse rather than truncate silently.
         if (mData.f != static_cast<F64>(static_cast<S64>(mData.f)))
            return false;
         out = static_cast<S64>(mData.f);
         return true;
      default: return false;
      }
   }

   template<> inline bool ScriptValue::tryGet<F64>(F64& out) const
   {
      switch (mKind)
      {
      case Kind::Float: out = mData.f; return true;
      case Kind::Bool:  out = mData.b ? 1.0 : 0.0; return true;
      case Kind::Int:
         // S64 -> F64 is exact only up to 2^53; refuse beyond that rather
         // than hand back a silently rounded value.
         if (mData.i > (S64(1) << 53) || mData.i < -(S64(1) << 53))
            return false;
         out = static_cast<F64>(mData.i);
         return true;
      default: return false;
      }
   }

   template<> inline bool ScriptValue::tryGet<const char*>(const char*& out) const
   {
      if (mKind != Kind::String) return false;
      out = mStr.c_str();
      return true;
   }

   template<> inline bool ScriptValue::tryGet<ObjectHandle>(ObjectHandle& out) const
   {
      if (mKind != Kind::Object) return false;
      out = mObj;
      return true;
   }

   /// Non-owning view over a contiguous run of ScriptValue, passed by value.
   ///
   /// @note Stands in for std::span (C++20, not available on this project's
   ///   compiler target) with the one constructor shape actually used here.
   class ScriptValueSpan
   {
   public:
      ScriptValueSpan() = default;
      ScriptValueSpan(const ScriptValue* data, U32 count) : mData(data), mCount(count) {}
      ScriptValueSpan(const Vector<ScriptValue>& values) : mData(values.address()), mCount(values.size()) {}

      const ScriptValue* data() const { return mData; }
      U32 size() const { return mCount; }
      bool isEmpty() const { return mCount == 0; }

      const ScriptValue& operator[](U32 index) const
      {
         AssertFatal(index < mCount, "ScriptValueSpan::operator[] - index out of range");
         return mData[index];
      }

      const ScriptValue* begin() const { return mData; }
      const ScriptValue* end() const { return mData + mCount; }

   private:
      const ScriptValue* mData = nullptr;
      U32 mCount = 0;
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTVALUE_H_
