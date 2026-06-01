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

#pragma once

#include "../korkTypes.h"
#include "../Diagnostics.h"

#include "compilerTables.h"
#include "../korkdebug/DebugInfo.h" 

namespace KorkScript
{
   class KorkCodeBlock;

   struct CompilationUnit
   {
      //-----------------------------------------------------------------------
      // Job parameters — set by begin()
      //-----------------------------------------------------------------------
      StringTableEntry    fileName = nullptr;
      const char*         sourceText = nullptr;   ///< kept alive by caller
      bool                isEval = false;
      bool                emitDebugInfo = false;

      //-----------------------------------------------------------------------
      // String pools  (one at global/file scope, one inside a function)
      // currentStrings points at whichever is active.
      //-----------------------------------------------------------------------
      StringPool   globalStrings;
      StringPool   functionStrings;
      StringPool* currentStrings = nullptr;

      //-----------------------------------------------------------------------
      // Float pools
      //-----------------------------------------------------------------------
      FloatPool    globalFloats;
      FloatPool    functionFloats;
      FloatPool* currentFloats = nullptr;

      //-----------------------------------------------------------------------
      // Ident table  (STE → IP list for DSO fixup)
      //-----------------------------------------------------------------------
      IdentTable   identTable;

      //-----------------------------------------------------------------------
      // Variable → register mapping  (survives into CodeBlock2 for debugger)
      //-----------------------------------------------------------------------
      VarRegisterTable variableRegisters;

      //-----------------------------------------------------------------------
      // Per-function register allocator
      //-----------------------------------------------------------------------
      FuncVars     funcVars;
      bool         inFunction = false;

      //-----------------------------------------------------------------------
      // STE encoding mode
      //   korkEvalSTEtoCode    — write pointer directly  (eval / in-memory exec)
      //   korkCompileSTEtoCode — write zero, record in identTable  (DSO build)
      //-----------------------------------------------------------------------
      STEtoCodeFn  steToCode = korkEvalSTEtoCode;

      //-----------------------------------------------------------------------
      // Debug info  (nullptr when emitDebugInfo == false)
      //-----------------------------------------------------------------------
      std::unique_ptr<DebugInfo> debugInfo;

      //-----------------------------------------------------------------------
      // Lifecycle
      //-----------------------------------------------------------------------
      CompilationUnit() = default;
      ~CompilationUnit() { reset(); }

      CompilationUnit(const CompilationUnit&) = delete;
      CompilationUnit& operator=(const CompilationUnit&) = delete;

      void begin(StringTableEntry fileName,
         const char* sourceText,
         bool             isEval,
         bool             emitDebugInfo);

      void enterFunction(StringTableEntry fnName, StringTableEntry ns);
      void exitFunction();

      // Build KorkCodeBlock from the completed CodeStream.
      // OP_RETURN_VOID must have been emitted before calling this.
      // Resets the unit on return.
      KorkCodeBlock* finalise(CodeStream& codeStream);

      void reset();

      //-----------------------------------------------------------------------
      // Accessors used by CodeGen
      //-----------------------------------------------------------------------
      StringPool& currentStringPool() { return *currentStrings; }
      FloatPool& currentFloatPool() { return *currentFloats; }

      // Returns the FunctionDebugInfo for the function currently being compiled,
      // or nullptr if debug info is disabled or we are at file scope.
      FunctionDebugInfo* currentFunctionDebugInfo();

   private:
      bool             mBegun = false;
      StringTableEntry mCurrentFnName = nullptr;
      StringTableEntry mCurrentNs = nullptr;
   };
}
