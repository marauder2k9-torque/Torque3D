// Copyright (c) 2026 WrenchSoft Ltd.
//
// This file is not licensed under the MIT License.
//
// Permission is granted to use, copy, modify, and distribute this file solely
// as part of the official TorqueGameEngines/Torque3D source repository and derivative
// works of that repository.
//
// No permission is granted to copy, use, distribute, sublicense, or incorporate
// this file independently or as part of any other software project without
// prior written permission from WrenchSoft Ltd.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND.

#include "console/console.h"     // Con::printf etc.
#include "platform/platform.h"
#include "core/stringTable.h"
#include "console/script.h"
#include "console/runtime.h"
#include "core/volume.h"
#include "core/stream/fileStream.h"
#include "core/fileObject.h"
#include "core/util/timeClass.h"

#include "korkRuntime.h"
#include "korkEvalState.h"
#include "korkCodeBlock.h"
#include "codegen/codegen.h"
#include "lexer/lexer.h"
#include "parser/parser.h"


namespace KorkScript
{
   KorkRuntime::KorkRuntime() 
   {
      Con::registerRuntime(0, this);
   }

   KorkRuntime::~KorkRuntime() = default;

   static KorkCodeBlock* runPipeline(const char* source,
      StringTableEntry fileEntry,
      bool             isEval,
      bool             emitDebugInfo,
      Vector<DiagnosticMessage>& diags)
   {
      // 1. Parse
      ParserContext ctx(source, fileEntry);
      Parser       parser(ctx);
      Node* root = parser.parseFile();

      if (!root)
         return nullptr;

      for (const auto& d : ctx.diagnostics())
         diags.push_back(d);

      if (ctx.hasErrors())
         return nullptr;

      // 2. Compile via KorkScript CodeGen
      CompilationUnit unit;
      unit.begin(fileEntry, source, isEval, emitDebugInfo);

      CodeStream cs;
      CodeGen    cg;

      if (!cg.compile(root, unit, cs, diags))
         return nullptr;

      // 3. Finalise into KorkCodeBlock
      return unit.finalise(cs);
   }

   //---------------------------------------------------------------------------
   // evaluateInternal — shared implementation
   //---------------------------------------------------------------------------

   Con::EvalResult KorkRuntime::evaluateInternal(const char* source,
      S32         frame,
      bool        echo,
      const char* fileName)
   {
      

      if (!source || !source[0])
         return Con::EvalResult(String("Empty source string."));

      if (echo)
      {
         if (source[0] == '%')
            Con::printf("%s", source);
         else
            Con::printf("%s%s", Con::getVariable("$Con::Prompt"), source);
      }

      StringTableEntry fileEntry = fileName
                                    ? StringTable->insert(fileName)
                                    : StringTable->insert("<eval>");

      Vector<DiagnosticMessage> diags;
      KorkCodeBlock* block = runPipeline(source, fileEntry, true, false, diags);

      for (const auto& d : diags)
      {
         if (d.isError())
            Con::errorf("%s - %s",fileEntry, d.message.c_str());
         else if (d.isWarning())
            Con::warnf("%s - %s",fileEntry, d.message.c_str());
      }

      if (!block)
         return Con::EvalResult(String("KorkScript compile error."));

      Con::EvalResult result = block->exec(
         0, nullptr, nullptr, 0, nullptr, false, nullptr, frame);

      return result;
   }

   Con::EvalResult KorkRuntime::evaluate(const char* source, bool echo, const char* fileName)
   {
      return evaluateInternal(source, -1, echo, fileName);
   }

   Con::EvalResult KorkRuntime::evaluate(const char* source, S32 frame, bool echo, const char* fileName)
   {
      // Make sure we're passing a valid frame to the eval.
      if (frame > KorkScript::gEvalState.getStackDepth())
         frame = KorkScript::gEvalState.getStackDepth() - 1;
      if (frame < 0)
         frame = 0;

      S32 evalBufferLen = dStrlen(source);
      bool isEvaluatingLocalVariable = evalBufferLen > 0 && source[0] == '%';
      if (isEvaluatingLocalVariable)
      {
         S32 stackIndex = KorkScript::gEvalState.getStackDepth() - frame - 1;
         KorkScript::gEvalState.pushDebugFrame(stackIndex);

         Dictionary& stackFrame = KorkScript::gEvalState.getCurrentFrame();
         StringTableEntry functionName = stackFrame.scopeName;
         StringTableEntry namespaceName = stackFrame.scopeNamespace->mName;
         StringTableEntry varToLookup = StringTable->insert(source);

         S32 regId = ((KorkCodeBlock*)stackFrame.module)->variableRegisters.lookup(namespaceName, functionName, varToLookup);
         if (regId == -1)
         {
            // ERROR, can't read the variable!
            return Con::EvalResult("variable not found");
         }

         const char* varRes = KorkScript::gEvalState.getLocalStringVariable(regId);

         KorkScript::gEvalState.popFrame();

         ConsoleValue val;
         val.setString(varRes);
         return Con::EvalResult("variable not found");
      }

      return evaluateInternal(source, frame, echo, fileName);
   }

   Con::EvalResult KorkRuntime::evaluatef(const char* fmt, ...)
   {
      char buf[4096];
      va_list args;
      va_start(args, fmt);
      dVsprintf(buf, sizeof(buf), fmt, args);
      va_end(args);
      return evaluate(buf);
   }

   bool KorkRuntime::executeFile(const char* fileName, bool noCalls, bool journalScript)
   {
      char srcPath[1024];
      if (!Con::expandScriptFilename(srcPath, sizeof(srcPath), fileName))
      {
         Con::errorf("KorkScript::KorkRuntime::executeFile - could not expand path '%s'", fileName);
         return false;
      }

      if (dStrEndsWith(srcPath, ".dso2"))
         srcPath[dStrlen(srcPath) - dStrlen(".dso2")] = '\0';

      const char* ext = dStrrchr(srcPath, '.');
      if (!ext)
      {
         // Try .ts2 first, then fall back to the engine default.
         char withExt[1024];
         dSprintf(withExt, sizeof(withExt), "%s.ts2", srcPath);
         if (Torque::FS::GetFileNode(withExt))
            return executeFile(withExt, noCalls, false);

         dSprintf(withExt, sizeof(withExt), "%s.%s", srcPath, TORQUE_SCRIPT_EXTENSION);
         if (Torque::FS::GetFileNode(withExt))
            return executeFile(withExt, noCalls, false);

         Con::errorf("KorkScript::KorkRuntime::executeFile - no extension and file not found: '%s'", srcPath);
         return false;
      }

      StringTableEntry srcEntry = StringTable->insert(srcPath);
      StringTableEntry dsoDir = Con::getDSOPath(srcPath);

      char dsoPath[1024];
      if (dsoDir && *dsoDir)
      {
         // Put DSO in the designated DSO directory.
         const char* filenameOnly = dStrrchr(srcPath, '/');
         filenameOnly = filenameOnly ? filenameOnly + 1 : srcPath;
         char base[1024];
         Platform::makeFullPathName(filenameOnly, base, sizeof(base), dsoDir);
         dSprintf(dsoPath, sizeof(dsoPath), "%s.dso2", base);
      }
      else
      {
         // No DSO directory — put DSO next to the source.
         dSprintf(dsoPath, sizeof(dsoPath), "%s.dso2", srcPath);
      }

      Torque::FS::FileNodeRef srcNode = Torque::FS::GetFileNode(srcEntry);
      Torque::FS::FileNodeRef dsoNode = Torque::FS::GetFileNode(dsoPath);

      bool useDSO = false;
      if (dsoNode)
      {
         if (!srcNode)
         {
            // No source — must use DSO.
            useDSO = true;
         }
         else
         {
            // Use DSO if it is at least as new as the source.
            Torque::Time srcTime = srcNode->getModifiedTime();
            Torque::Time dsoTime = dsoNode->getModifiedTime();
            useDSO = (dsoTime >= srcTime);
         }
      }

      if (useDSO)
      {
         KorkCodeBlock* block = new KorkCodeBlock();
         block->name = srcEntry;
         block->fullPath = srcEntry;
         block->modPath = Con::getModNameFromPath(fileName);

         if (!block->readDSO(dsoPath))
         {
            Con::warnf("KorkScript::KorkRuntime::executeFile - DSO load failed for '%s', recompiling.", dsoPath);
            useDSO = false;   // fall through to source compile
         }
         else
         {
            Con::EvalResult result = block->exec(
               0, nullptr, nullptr, 0, nullptr, noCalls, nullptr);
            
            return result.valid;
         }
      }

      if (!srcNode)
      {
         Con::errorf("KorkScript::KorkRuntime::executeFile - file not found: '%s'", srcPath);
         return false;
      }

      void* rawData = nullptr;
      U32    dataSize = 0;
      Torque::FS::ReadFile(srcEntry, rawData, dataSize, true);
      if (!rawData || !dataSize)
      {
         Con::errorf("KorkScript::KorkRuntime::executeFile - could not read '%s'", srcPath);
         return false;
      }

      const char* source = (const char*)rawData;

      Vector<DiagnosticMessage> diags;
      // isEval=true: write live STE pointers directly (no fixup needed for
      // in-memory execution).
      KorkCodeBlock* block = runPipeline(source, srcEntry, false, false, diags);

      for (const auto& d : diags)
      {
         if (d.isError())        Con::errorf("%s - %s",srcEntry, d.message.c_str());
         else if (d.isWarning()) Con::warnf("%s - %s",srcEntry, d.message.c_str());
      }

      delete[](char*)rawData;

      if (!block) return false;

#ifndef TORQUE_NO_DSO_GENERATION
      if (dsoDir && *dsoDir && !Con::getBoolVariable("Scripts::ignoreDSOs"))
      {
         block->writeDSO(dsoPath);
      }
#endif
      Con::EvalResult result = block->exec(
         0, nullptr, nullptr, 0, nullptr, noCalls, nullptr);
      return result.valid;
   }

   bool KorkRuntime::compile(const char* fileName, bool overrideNoDso)
   {
      char expanded[1024];
      Con::expandScriptFilename(expanded, sizeof(expanded), fileName);

      // Figure out where to put DSOs
      StringTableEntry dsoPath = Con::getDSOPath(expanded);
      if (dsoPath && *dsoPath == 0)
         return false;

      // If the script file extention is '.ed.tscript' then compile it to a different compiled extention
      bool isEditorScript = false;
      const char* ext = dStrrchr(expanded, '.');
      if (ext && (dStricmp(ext, "." TORQUE_SCRIPT_EXTENSION) == 0))
      {
         const char* ext2 = ext - 3;
         if (dStricmp(ext2, ".ed." TORQUE_SCRIPT_EXTENSION) == 0)
            isEditorScript = true;
      }
      else if (ext && (dStricmp(ext, ".gui") == 0))
      {
         const char* ext2 = ext - 3;
         if (dStricmp(ext2, ".ed.gui") == 0)
            isEditorScript = true;
      }

      const char* filenameOnly = dStrrchr(expanded, '/');
      if (filenameOnly)
         ++filenameOnly;
      else
         filenameOnly = expanded;

      char nameBuffer[512];

      if (isEditorScript)
         dStrcpyl(nameBuffer, sizeof(nameBuffer), dsoPath, "/", filenameOnly, ".edso", NULL);
      else
         dStrcpyl(nameBuffer, sizeof(nameBuffer), dsoPath, "/", filenameOnly, ".dso", NULL);

      void* data = NULL;
      U32 dataSize = 0;
      Torque::FS::ReadFile(expanded, data, dataSize, true);
      if (data == NULL)
      {
         Con::errorf(ConsoleLogEntry::Script, "compile: invalid script file %s.", expanded);
         return false;
      }

      const char* script = static_cast<const char*>(data);

#ifdef TORQUE_DEBUG
      Con::printf("Compiling %s...", expanded);
#endif
      StringTableEntry fileEntry = StringTable->insert(expanded);
      Vector<DiagnosticMessage> diags;
      KorkCodeBlock* block = runPipeline(script, fileEntry, false, true, diags);

      for (const auto& d : diags)
      {
         if (d.isError())        Con::errorf("%s - %s", fileEntry, d.message.c_str());
         else if (d.isWarning()) Con::warnf("%s - %s", fileEntry, d.message.c_str());
      }

      if (!block) return false;

      // Derive DSO path: replace extension with .dso2
      char newdsoPath[1024];
      dStrcpy(newdsoPath, expanded, sizeof(dsoPath));
      char* ext2 = dStrrchr(newdsoPath, '.');
      if (ext2) dStrcpy(ext2, ".dso2", sizeof(newdsoPath) - (ext2 - newdsoPath));
      else     dStrcat(newdsoPath, ".dso2", sizeof(newdsoPath));

      bool ok = block->writeDSO(newdsoPath);
      return ok;
   }

   void KorkRuntime::expandEscapedCharacters(char* dest, const char* src)
   {
   }

   bool KorkRuntime::collapseEscapedCharacters(char* buf)
   {
      return false;
   }
}
