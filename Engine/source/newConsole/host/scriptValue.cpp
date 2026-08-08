#include "newConsole/host/scriptValue.h"

#include "core/strings/stringFunctions.h"

namespace newConsole
{

String ScriptValue::toDisplayString() const
{
   switch (mKind)
   {
      case Kind::Null:   return String();
      case Kind::Bool:   return String(mData.b ? "1" : "0");
      case Kind::Int:
      {
         char buf[32];
         dSprintf(buf, sizeof(buf), "%lld", static_cast<long long>(mData.i));
         return String(buf);
      }
      case Kind::Float:
      {
         // %g at full double precision - round-trips exactly through
         // parseFromString(Kind::Float, ...) for any value this type can
         // hold. Anything shorter (the legacy "%g" default precision) is
         // exactly the kind of silent precision loss this type exists to
         // avoid, so it is never used here.
         char buf[64];
         dSprintf(buf, sizeof(buf), "%.17g", mData.f);
         return String(buf);
      }
      case Kind::String: return String(mStr.c_str());
      case Kind::Object:
      {
         char buf[32];
         dSprintf(buf, sizeof(buf), "%u", mObj.id);
         return String(buf);
      }
      case Kind::Error:  return String(mStr.c_str());
      default:           return String();
   }
}

ScriptValue ScriptValue::parseFromString(Kind targetKind, const char* text)
{
   if (!text)
      text = "";

   switch (targetKind)
   {
      case Kind::Bool:   return makeBool(dAtob(text));
      case Kind::Int:
         // dAtoi is 32-bit; this project's string layer has no 64-bit
         // text parser today. Values outside S32 range parsed from text
         // will not round-trip exactly - this is a known limitation of
         // the text path specifically, not of ScriptValue's native
         // storage (which is S64 throughout and never truncates on its
         // own). Flagged rather than silently accepted.
         return makeInt(static_cast<S64>(dAtoi(text)));
      case Kind::Float:  return makeFloat(dAtof(text));
      case Kind::String: return makeString(text);
      default:
         AssertFatal(false, "ScriptValue::parseFromString - unsupported target kind");
         return makeNull();
   }
}

} // namespace newConsole
