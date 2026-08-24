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
#ifndef _NEWCONSOLE_FILEIO_H_
#include "newConsole/fileIO.h"
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

      /// Reads an entire .ts2 file into memory. Thin wrapper over
      /// newConsole::readScriptFile (fileIO.h) - kept as its own name/header
      /// here for source compatibility with existing torquescript2 call sites
      /// and so this runtime's own file-reading entry point is discoverable
      /// from this file without needing to know it delegates elsewhere.
      inline bool loadSourceFile(const char* filename, String& outSource, String* outError = nullptr)
      {
         return readScriptFile(filename, outSource, outError);
      }

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_SOURCEFILE_H_
