#ifndef _NEWCONSOLE_TS2_BYTECODESERIALIZE_H_
#define _NEWCONSOLE_TS2_BYTECODESERIALIZE_H_

#ifndef _NEWCONSOLE_TS2_BYTECODE_H_
#include "newConsole/torquescript2/bytecode.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

class Stream;

namespace newConsole
{
   namespace ts2
   {

      /// Compiled-module file extension, no leading dot.
      constexpr const char* kCompiledFileExtension = "tsc";

      /// Every function one source file compiled to, plus the content
      /// hash it was compiled from (staleness check, not mtime-based).
      struct CompiledModule
      {
         U64 sourceHash = 0;
         Vector<StringTableEntry> functionNames;
         Vector<BytecodeUnit> functionUnits;
      };

      /// Content hash of @a source - used for staleness checks.
      U64 hashSource(const char* source, U32 length);

      /// Writes @a module to @a stream in the .tsc format.
      /// @return false on write failure; @a outError gets a short reason.
      bool writeCompiledModule(Stream& stream, const CompiledModule& module, String* outError = nullptr);

      /// Reads a .tsc previously written by writeCompiledModule. Does
      /// not check sourceHash - that's the caller's job.
      /// @return false if invalid/corrupt; @a outError gets a short reason.
      bool readCompiledModule(Stream& stream, CompiledModule& outModule, String* outError = nullptr);

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_BYTECODESERIALIZE_H_
