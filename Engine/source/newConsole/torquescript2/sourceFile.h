#ifndef _NEWCONSOLE_TS2_SOURCEFILE_H_
#define _NEWCONSOLE_TS2_SOURCEFILE_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      /// File extension this runtime claims, without the leading dot.
      constexpr const char* kFileExtension = "ts2";

      /// @return true if @a filename's extension is "ts2" (case-insensitive),
      ///   the check IScriptRuntime::canHandle uses to claim a file.
      bool hasScriptExtension(const char* filename);

      /// Reads an entire .ts2 file into memory via Torque::FS (FileStream, not
      /// raw fopen/libc I/O - this goes through the same virtual mount system
      /// every other engine file access does, so mounted zip/data/home paths
      /// resolve exactly like any other asset load).
      ///
      /// @return true and fills @a outSource on success. On failure, outSource
      ///   is left untouched and @a outError (if non-null) gets a short reason
      ///   - "file not found", "could not open stream", etc.
      bool loadSourceFile(const char* filename, String& outSource, String* outError = nullptr);

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_SOURCEFILE_H_
