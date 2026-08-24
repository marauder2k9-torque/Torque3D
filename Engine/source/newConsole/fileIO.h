#ifndef _NEWCONSOLE_FILEIO_H_
#define _NEWCONSOLE_FILEIO_H_

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

/// Reads an entire file into memory via Torque::FS (FileStream, not raw
/// fopen/libc I/O) - goes through the same virtual mount system every
/// other engine file access does, so mounted zip/data/home paths resolve
/// exactly like any other asset load.
///
/// @note Deliberately not tied to any one script language/extension -
///   this is the one place file-reading logic for script source lives;
///   a per-runtime helper (e.g. torquescript2's own sourceFile.h) should
///   call this rather than reimplementing file I/O, and newConsole::exec
///   (newConsole.h) uses this directly for the same reason.
///
/// @return true and fills @a outSource on success. On failure, outSource
///   is left untouched and @a outError (if non-null) gets a short reason
///   - "file not found", "could not open stream", etc.
bool readScriptFile(const char* filename, String& outSource, String* outError = nullptr);

} // namespace newConsole

#endif // !_NEWCONSOLE_FILEIO_H_
