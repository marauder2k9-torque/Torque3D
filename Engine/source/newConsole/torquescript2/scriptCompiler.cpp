#include "newConsole/torquescript2/scriptCompiler.h"

#ifndef _NEWCONSOLE_TS2_LEXER_H_
#include "newConsole/torquescript2/lexer.h"
#endif
#ifndef _NEWCONSOLE_TS2_PARSER_H_
#include "newConsole/torquescript2/parser.h"
#endif
#ifndef _NEWCONSOLE_TS2_EMITTER_H_
#include "newConsole/torquescript2/emitter.h"
#endif
#ifndef _NEWCONSOLE_TS2_FUNCTIONTABLE_H_
#include "newConsole/torquescript2/functionTable.h"
#endif
#ifndef _NEWCONSOLE_TS2_BYTECODESERIALIZE_H_
#include "newConsole/torquescript2/bytecodeSerialize.h"
#endif
#ifndef _NEWCONSOLE_TS2_SOURCEFILE_H_
#include "newConsole/torquescript2/sourceFile.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTCLASSMACROS_H_
#include "newConsole/host/scriptClassMacros.h"
#endif
#ifndef _FILESTREAM_H_
#include "core/stream/fileStream.h"
#endif
#ifndef _VOLUME_H_
#include "core/volume.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      namespace
      {
         /// Reads only an existing .tsc's sourceHash, for the up-to-date check.
         bool tryReadExistingHash(const char* dstPath, U64& outHash)
         {
            if (!Torque::FS::IsFile(Torque::Path(dstPath)))
               return false;

            FileStream* stream = FileStream::createAndOpen(dstPath, Torque::FS::File::Read);
            if (!stream)
               return false;

            CompiledModule existing;
            bool ok = readCompiledModule(*stream, existing);
            delete stream;

            if (!ok)
               return false;

            outHash = existing.sourceHash;
            return true;
         }

         /// Name under which compileSourceToModule stores the file's
         /// top-level-statement chunk in CompiledModule::functionNames.
         /// Not a valid Ident (leading '$' - the lexer never produces an
         /// Ident starting with '$'), so it can never collide with a real
         /// user function name. A loader must special-case this name
         /// rather than treat it as a callable function.
         static StringTableEntry topLevelChunkName()
         {
            return StringTable->insert("$topLevel");
         }

         /// Compiles every top-level function (including those nested in
         /// package blocks) plus the file's top-level-statement chunk;
         /// never runs the code, only compiles it.
         bool compileSourceToModule(const char* originName, const char* source, U32 sourceLength,
            bool stripDebugInfo, CompiledModule& outModule, String& outError)
         {
            StringTableEntry internedOrigin = StringTable->insert(originName);

            // No live registry offline - TypeIdent degrades to Ident.
            Lexer lexer(source, originName, [](StringTableEntry) { return false; });
            Parser parser(lexer, internedOrigin);
            CompilationUnit unit = parser.parse();

            if (lexer.hasErrors())
            {
               outError = lexer.diagnostics()[0].message;
               return false;
            }
            if (parser.hasErrors())
            {
               outError = parser.diagnostics()[0].message;
               return false;
            }

            outModule.sourceHash = hashSource(source, sourceLength);

            // Compiles one FunctionDeclStmt and appends it under its
            // qualified name - shared by the plain top-level loop below
            // and the package-nested loop, so both compile/name a
            // function identically.
            auto compileAndAppend = [&](const ast::FunctionDeclStmt& fn) -> bool
            {
               Emitter emitter(unit);
               BytecodeUnit compiled = emitter.compileFunction(fn);

               if (emitter.hasErrors())
               {
                  outError = emitter.diagnostics()[0].message;
                  return false;
               }

               if (stripDebugInfo)
               {
                  compiled.lineTable.clear();
                  compiled.origin = nullptr;
                  compiled.localDebugInfo.clear();
               }

               StringTableEntry qualifiedName = FunctionTable::qualify(fn.namespaceName, fn.functionName);
               outModule.functionNames.push_back(qualifiedName);
               outModule.functionUnits.push_back(std::move(compiled));
               return true;
            };

            const ast::StmtHandle* top = CompilationUnit::listData(unit.stmtList, unit.topLevel);
            bool hasTopLevelExecStmt = false;
            for (U32 i = 0; i < unit.topLevel.count; ++i)
            {
               const ast::StmtNode& stmt = unit.get(top[i]);

               if (stmt.kind == ast::StmtKind::FunctionDecl)
               {
                  if (!compileAndAppend(stmt.functionDecl))
                     return false;
                  continue;
               }

               if (stmt.kind == ast::StmtKind::PackageDecl)
               {
                  // A package body may only contain FunctionDecl statements
                  // (enforced by the parser - see parsePackageDeclStatement),
                  // so every entry here is safe to compileAndAppend directly.
                  const ast::StmtHandle* decls = CompilationUnit::listData(unit.stmtList, stmt.packageDecl.functionDecls);
                  for (U32 d = 0; d < stmt.packageDecl.functionDecls.count; ++d)
                  {
                     const ast::StmtNode& declStmt = unit.get(decls[d]);
                     if (!compileAndAppend(declStmt.functionDecl))
                        return false;
                  }
                  continue;
               }

               hasTopLevelExecStmt = true;
            }

            if (hasTopLevelExecStmt)
            {
               Emitter topEmitter(unit);
               BytecodeUnit topUnit = topEmitter.compileTopLevel();
               if (topEmitter.hasErrors())
               {
                  outError = topEmitter.diagnostics()[0].message;
                  return false;
               }
               if (stripDebugInfo)
               {
                  topUnit.lineTable.clear();
                  topUnit.origin = nullptr;
                  topUnit.localDebugInfo.clear();
               }

               outModule.functionNames.push_back(topLevelChunkName());
               outModule.functionUnits.push_back(std::move(topUnit));
            }

            return true;
         }

         String deriveOutputPath(const char* srcPath)
         {
            Torque::Path path(srcPath);
            path.setExtension(kCompiledFileExtension);
            return path.getFullPath();
         }

         /// Ensures @a dir ends in '/' so Torque::Path parses it with an
         /// empty file/extension - Path::isDirectory() (and therefore
         /// FindByPattern/OpenDirectory) requires this; a path with no
         /// trailing slash parses its last segment as a filename instead.
         String asDirectoryString(const char* dir)
         {
            String s(dir ? dir : "");
            if (s.isEmpty() || s.c_str()[s.length() - 1] != '/')
               s += "/";
            return s;
         }

      } // namespace

      CompileFileResult compileScriptFile(const char* srcPath, const char* dstPath, bool stripDebugInfo, bool force)
      {
         CompileFileResult result;

         String resolvedDst = dstPath ? String(dstPath) : deriveOutputPath(srcPath);

         String source;
         String readError;
         if (!readScriptFile(srcPath, source, &readError))
         {
            result.errorMessage = readError;
            return result;
         }

         U64 sourceHash = hashSource(source.c_str(), source.length());

         if (!force)
         {
            U64 existingHash = 0;
            if (tryReadExistingHash(resolvedDst.c_str(), existingHash) && existingHash == sourceHash)
            {
               result.success = true;
               result.skippedUpToDate = true;
               return result;
            }
         }

         CompiledModule module;
         String compileError;
         if (!compileSourceToModule(srcPath, source.c_str(), source.length(), stripDebugInfo, module, compileError))
         {
            result.errorMessage = compileError;
            return result;
         }

         FileStream* outStream = FileStream::createAndOpen(resolvedDst.c_str(), Torque::FS::File::Write);
         if (!outStream)
         {
            result.errorMessage = "could not open output file for writing";
            return result;
         }

         String writeError;
         bool writeOk = writeCompiledModule(*outStream, module, &writeError);
         delete outStream;

         if (!writeOk)
         {
            result.errorMessage = writeError;
            return result;
         }

         result.success = true;
         return result;
      }

      CompileTreeResult compileScriptTree(const char* srcDir, const char* dstDir, bool stripDebugInfo, bool force)
      {
         CompileTreeResult result;

         String srcRootStr = asDirectoryString(srcDir);
         Torque::Path srcRoot(srcRootStr);

         Vector<String> matches;
         String pattern = String("*.") + kFileExtension;
         S32 matchCount = Torque::FS::FindByPattern(srcRoot, pattern, /*recursive*/ true, matches, /*multiMatch*/ false);
         result.filesFound = (matchCount > 0) ? static_cast<U32>(matches.size()) : 0;

         result.success = true;

         for (U32 i = 0; i < matches.size(); ++i)
         {
            String fullSrcPath = matches[i];

            // Relative portion, with the srcRoot prefix stripped.
            String relative = fullSrcPath.substr(srcRootStr.length());
            while (!relative.isEmpty() && relative.c_str()[0] == '/')
               relative = relative.substr(1);

            String dstDirStr = asDirectoryString(dstDir);
            Torque::Path dstPath(dstDirStr + relative);
            dstPath.setExtension(kCompiledFileExtension);

            Torque::Path dstParent(dstPath);
            dstParent.setFileName("");
            dstParent.setExtension("");
            Torque::FS::CreatePath(dstParent);

            CompileFileResult fileResult = compileScriptFile(fullSrcPath.c_str(), dstPath.getFullPath().c_str(), stripDebugInfo, force);

            if (!fileResult.success)
            {
               result.success = false;
               result.failures.push_back(fullSrcPath + ": " + fileResult.errorMessage);
            }
            else if (fileResult.skippedUpToDate)
            {
               ++result.filesUpToDate;
            }
            else
            {
               ++result.filesCompiled;
            }
         }

         return result;
      }

      bool scriptCompileFile(const char* srcPath, const char* dstPath, bool stripDebugInfo, bool force)
      {
         const char* resolvedDst = (dstPath && dstPath[0]) ? dstPath : nullptr;
         CompileFileResult result = compileScriptFile(srcPath, resolvedDst, stripDebugInfo, force);
         return result.success;
      }

      bool scriptCompileTree(const char* srcDir, const char* dstDir, bool stripDebugInfo, bool force)
      {
         CompileTreeResult result = compileScriptTree(srcDir, dstDir, stripDebugInfo, force);
         return result.success;
      }

   } // namespace ts2
} // namespace newConsole

GLOBAL_SCRIPT_METHOD(bool, newConsole::ts2::scriptCompileFile,
   (const char* srcPath, const char* dstPath, bool stripDebugInfo, bool force));
GLOBAL_SCRIPT_METHOD(bool, newConsole::ts2::scriptCompileTree,
   (const char* srcDir, const char* dstDir, bool stripDebugInfo, bool force));
