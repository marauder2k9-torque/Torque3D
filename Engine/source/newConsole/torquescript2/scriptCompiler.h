#ifndef _NEWCONSOLE_TS2_SCRIPTCOMPILER_H_
#define _NEWCONSOLE_TS2_SCRIPTCOMPILER_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      /// Result of compiling one .ts2 file.
      struct CompileFileResult
      {
         bool success = false;
         bool skippedUpToDate = false; ///< existing .tsc was already current
         String errorMessage;          ///< empty on success
      };

      /// Result of compiling a whole tree.
      struct CompileTreeResult
      {
         bool success = false;         ///< false if any file failed - see failures
         U32 filesFound = 0;           ///< .ts2 files matched (FindByPattern filters by extension directly)
         U32 filesCompiled = 0;
         U32 filesUpToDate = 0;
         Vector<String> failures;      ///< "path: message" per failed file
      };

      /// Compiles one .ts2 to a sibling (or @a dstPath) .tsc. Skips if
      /// already up to date unless @a force is set.
      /// @param dstPath nullptr writes alongside srcPath; parent dir must already exist.
      /// @param stripDebugInfo omits per-instruction line numbers.
      CompileFileResult compileScriptFile(const char* srcPath, const char* dstPath = nullptr,
         bool stripDebugInfo = false, bool force = false);

      /// Recursively compiles every .ts2 under @a srcDir into the
      /// mirrored path under @a dstDir. Continues past a failed file -
      /// see CompileTreeResult::failures.
      CompileTreeResult compileScriptTree(const char* srcDir, const char* dstDir,
         bool stripDebugInfo = false, bool force = false);

      /// Script-callable wrappers (see GLOBAL_SCRIPT_METHOD).
      bool scriptCompileFile(const char* srcPath, const char* dstPath, bool stripDebugInfo, bool force);
      bool scriptCompileTree(const char* srcDir, const char* dstDir, bool stripDebugInfo, bool force);

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_SCRIPTCOMPILER_H_
