#include "newConsole/host/scriptArray.h"

namespace newConsole
{

   ScriptValue scriptArrayGet(const ScriptValue& base, const ScriptValue& index)
   {
      // A Null base (an unset variable, field, or array element that was
      // never touched) reads as empty, exactly like an out-of-range index
      // does below - not an error. This matters beyond plain reads: chained
      // index assignment (%grid[1][2] = 5) reads the *inner* base
      // (%grid[1]) before writing into it (see Emitter::emitStoreTo's
      // IndexAccess case), and that inner base is legitimately Null the
      // first time a grid is being built from nothing. If this returned
      // Error for a Null base, scriptArraySet's own vivification could
      // never even be reached - it would be handed an Error value, not the
      // Null one it specifically checks for. Any other non-Array,
      // non-Null kind is still a real type error, unchanged from before.
      if (base.kind() == ScriptValue::Kind::Null)
         return ScriptValue::makeNull();
      if (base.kind() != ScriptValue::Kind::Array)
         return ScriptValue::makeError("scriptArrayGet: base value is not an array");

      S64 i = 0;
      if (!index.tryGet<S64>(i) || i < 0)
         return ScriptValue::makeError("scriptArrayGet: index is not a non-negative integer");

      ScriptValue baseCopy = base;
      Vector<ScriptValue>& arr = baseCopy.arrayRef();
      if (static_cast<U32>(i) >= arr.size())
         return ScriptValue::makeNull();

      return arr[static_cast<U32>(i)];
   }

   ScriptValue scriptArraySet(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value)
   {
      S64 i = 0;
      if (!index.tryGet<S64>(i) || i < 0)
         return ScriptValue::makeError("scriptArraySet: index is not a non-negative integer");

      // Vivify: an unset variable/field/element reads as Kind::Null (see
      // ScriptValue's own default-constructed state) - the first indexed
      // write into it creates the array rather than failing. Any other
      // non-Array kind is a real type error, not something to vivify over.
      ScriptValue target = (base.kind() == ScriptValue::Kind::Null) ? ScriptValue::makeArray() : base;
      if (target.kind() != ScriptValue::Kind::Array)
         return ScriptValue::makeError("scriptArraySet: base value is not an array");

      Vector<ScriptValue>& arr = target.arrayRef();
      U32 idx = static_cast<U32>(i);
      if (idx >= arr.size())
         arr.setSize(idx + 1);

      arr[idx] = value;
      return target;
   }

} // namespace newConsole
