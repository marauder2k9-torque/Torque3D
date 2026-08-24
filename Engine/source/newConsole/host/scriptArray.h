#ifndef _NEWCONSOLE_SCRIPTARRAY_H_
#define _NEWCONSOLE_SCRIPTARRAY_H_

#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif

namespace newConsole
{

   // Indexed read. Null if past the end, Error if base isn't an array or
   // index is invalid.
   ScriptValue scriptArrayGet(const ScriptValue& base, const ScriptValue& index);

   // Indexed write, growing as needed. Null base auto-vivifies a new array;
   // caller must write the result back or the vivified array is lost.
   ScriptValue scriptArraySet(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value);

} // namespace newConsole

#endif // !_NEWCONSOLE_SCRIPTARRAY_H_
