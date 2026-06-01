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

#include "compilationUnit.h"
#include "../korkCodeBlock.h"

#include "console/torquescript/compiler.h" // for opcodes

namespace KorkScript
{
   //---------------------------------------------------------------------------
   // begin
   //---------------------------------------------------------------------------
   void CompilationUnit::begin(StringTableEntry fileName_,
                              const char* sourceText_,
                              bool             isEval_,
                              bool             emitDebugInfo_)
   {
      AssertFatal(!mBegun, "CompilationUnit::begin called twice without reset()");

      fileName = fileName_;
      sourceText = sourceText_;
      isEval = isEval_;
      emitDebugInfo = emitDebugInfo_;

      // Reset all tables to a clean state — mirrors Compiler::resetTables()
      // but operating on our own instances rather than the globals.
      globalStrings.reset();
      functionStrings.reset();
      globalFloats.reset();
      functionFloats.reset();
      identTable.reset();
      variableRegisters.reset();
      funcVars.clear();

      // Start at global scope — string/float current pointers aim at global tables.
      currentStrings = &globalStrings;
      currentFloats = &globalFloats;
      inFunction = false;

      steToCode = isEval ? korkEvalSTEtoCode
                          : korkCompileSTEtoCode;

      // Initialise debug info container if requested.
      if (emitDebugInfo)
      {
         debugInfo = std::make_unique<DebugInfo>();
         debugInfo->present = true;
         debugInfo->fileName = fileName_;
         if (sourceText_)
            debugInfo->sourceText = sourceText_;
      }

      mBegun = true;
   }

   //---------------------------------------------------------------------------
   // enterFunction
   //---------------------------------------------------------------------------
   void CompilationUnit::enterFunction(StringTableEntry fnName, StringTableEntry ns)
   {
      AssertFatal(mBegun, "CompilationUnit::enterFunction called before begin()");
      AssertFatal(!inFunction, "Nested enterFunction — KorkScript does not support nested function definitions");

      currentStrings = &functionStrings;
      currentFloats = &functionFloats;
      inFunction = true;

      mCurrentFnName = fnName;
      mCurrentNs = ns;

      // Reset register allocator for this function.
      funcVars.clear();

      // Initialise per-function debug info entry.
      if (emitDebugInfo && debugInfo)
      {
         FunctionDebugInfo info;
         info.functionName = fnName;
         info.namespaceName = ns;
         debugInfo->functions[fnName] = std::move(info);
      }
   }

   //---------------------------------------------------------------------------
   // exitFunction
   //---------------------------------------------------------------------------
   void CompilationUnit::exitFunction()
   {
      AssertFatal(inFunction, "CompilationUnit::exitFunction called outside function scope");

      // Swap back to global tables.
      currentStrings = &globalStrings;
      currentFloats = &globalFloats;
      inFunction = false;
      mCurrentFnName = nullptr;
      mCurrentNs = nullptr;
   }

   FunctionDebugInfo* CompilationUnit::currentFunctionDebugInfo()
   {
      if (!emitDebugInfo || !debugInfo || !inFunction)
         return nullptr;

      auto it = debugInfo->functions.find(mCurrentFnName);
      if (it == debugInfo->functions.end())
         return nullptr;

      return &it->second;
   }


   //---------------------------------------------------------------------------
   // reset
   //---------------------------------------------------------------------------
   KorkCodeBlock* CompilationUnit::finalise(CodeStream& codeStream)
   {
      AssertFatal(mBegun, "CompilationUnit::finalise called before begin()");
      AssertFatal(!inFunction, "CompilationUnit::finalise called inside function scope");

      codeStream.emit(Compiler::OP_RETURN_VOID);

      // Flatten the CodeStream chunks into a single heap array.
      U32  codeSize = 0;
      U32* code = nullptr;
      U32* lineBreakPairs = nullptr;
      codeStream.finalise(&codeSize, &code, &lineBreakPairs);

      U32 lineBreakPairCount = codeStream.getNumLineBreaks();

      if (!isEval)
      {
         identTable.applyFixups(globalStrings, code);
      }

      char* gStrings = globalStrings.totalLen() > 0 ? globalStrings.build() : nullptr;
      char* fStrings = functionStrings.totalLen() > 0 ? functionStrings.build() : nullptr;
      F64* gFloats = globalFloats.count() > 0 ? globalFloats.build() : nullptr;
      F64* fFloats = functionFloats.count() > 0 ? functionFloats.build() : nullptr;
      U32 gStrLen = globalStrings.totalLen();
      U32 fStrLen = functionStrings.totalLen();

      // Assemble the CodeBlock2.
      KorkCodeBlock* block = new KorkCodeBlock();

      block->name = fileName;
      block->fullPath = fileName;

      block->code = code;
      block->codeSize = codeSize;
      block->lineBreakPairs = lineBreakPairs;
      block->lineBreakPairCount = lineBreakPairCount;

      block->globalStrings = gStrings;
      block->functionStrings = fStrings;
      block->globalStringsMaxLen = gStrLen;
      block->functionStringsMaxLen = fStrLen;
      block->globalFloats = gFloats;
      block->functionFloats = fFloats;

      // Transfer the variable register mapping table.
      block->variableRegisters = variableRegisters.copy();

      // Transfer debug info (nullptr if not emitting).
      block->debugInfo = std::move(debugInfo);

      block->calcBreakList();

      reset();
      return block;

   }

   //---------------------------------------------------------------------------
   // reset
   //---------------------------------------------------------------------------
   void CompilationUnit::reset()
   {
      globalStrings.reset();
      functionStrings.reset();
      globalFloats.reset();
      functionFloats.reset();
      identTable.reset();
      variableRegisters.reset();
      funcVars.clear();

      currentStrings = nullptr;
      currentFloats = nullptr;
      inFunction = false;
      debugInfo.reset();
      mBegun = false;
   }
}
