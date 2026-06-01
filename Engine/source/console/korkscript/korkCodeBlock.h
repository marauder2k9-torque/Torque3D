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

#ifndef _SCRIPT_MODULE_H_
#include "console/module.h"          // Con::Module, Con::EvalResult
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#include "codegen/compilerTables.h"  // VarRegisterTable
#include "korkdebug/debugInfo.h"

#include <atomic>

namespace KorkScript
{
   class KorkCodeBlock : Con::Module
   {
      friend struct CompilationUnit;

   public:
      KorkCodeBlock();
      ~KorkCodeBlock() override;

      //-----------------------------------------------------------------------
      // Identity
      //-----------------------------------------------------------------------
      StringTableEntry    name      = nullptr;
      StringTableEntry    fullPath  = nullptr;
      StringTableEntry    modPath   = nullptr;

      //-----------------------------------------------------------------------
      // ByteCode
      //-----------------------------------------------------------------------
      U32*  code                 = nullptr;
      U32   codeSize             = 0;
      U32*  lineBreakPairs       = nullptr;
      U32   lineBreakPairCount   = 0;

      //-----------------------------------------------------------------------
      // String / Float tables
      //-----------------------------------------------------------------------
      char* globalStrings = nullptr;
      char* functionStrings = nullptr;
      U32   globalStringsMaxLen = 0;
      U32   functionStringsMaxLen = 0;
      F64*  globalFloats = nullptr;
      F64*  functionFloats = nullptr;

      //-----------------------------------------------------------------------
      // VarRegisterTable  (used by debugger for variable inspection)
      //-----------------------------------------------------------------------
      VarRegisterTable variableRegisters;

      //-----------------------------------------------------------------------
      // Breakable line list (populated by calcBreakList)
      //-----------------------------------------------------------------------
      Vector<U32> breakList;
      void calcBreakList();

      //-----------------------------------------------------------------------
      // Debug info  (nullptr = stripped / release build)
      //-----------------------------------------------------------------------
      std::unique_ptr<DebugInfo> debugInfo;

      //-----------------------------------------------------------------------
      // Con::Module interface
      //-----------------------------------------------------------------------
      Con::EvalResult exec(U32               offset,
                           const char*       fnName,
                           Namespace*        ns,
                           U32               argc,
                           ConsoleValue*     argv,
                           bool              noCalls,
                           StringTableEntry  packageName,
                           S32               setFrame = -1) override;

      const char* getFunctionArgs(StringTableEntry fnName, U32 offset) override;
      const char* getPath() override { return fullPath; }
      const char* getName() override { return name; }

      void        findBreakLine(U32 ip, U32& line, U32& instruction) override;
      const char* getFileLine(U32 ip) override;
      U32         findFirstBreakLine(U32 lineNumber) override;

      // Breakpoints delegate to KorkScript::BreakpointTable — code array is never patched.
      bool setBreakpoint(U32 lineNumber) override;
      void clearBreakpoint(U32 lineNumber) override;
      void clearAllBreaks() override;
      void setAllBreaks() override;

      Vector<U32> getBreakableLines() override { return breakList; }

      //-----------------------------------------------------------------------
      // DSO Serialisation
      //-----------------------------------------------------------------------
      bool writeDSO(const char* dsoPath) const;
      bool readDSO(const char* dsoPath);

      //-----------------------------------------------------------------------
      // RefCounting
      //-----------------------------------------------------------------------
      void addRef() { mRefCount.fetch_add(1, std::memory_order_relaxed); }
      void release();  ///< Deletes this when mRefCount reaches zero.
      U32  getRefCount() const { return mRefCount.load(std::memory_order_relaxed); }


      //-----------------------------------------------------------------------
      // Global list  (mirrors CodeBlock::smCodeBlockList for debugger enumeration)
      //-----------------------------------------------------------------------
      static KorkCodeBlock* smList;
      static KorkCodeBlock* find(StringTableEntry name);
      KorkCodeBlock* mNext = nullptr;

   private:
      std::atomic<U32>  mRefCount{ 0 };

      void addToList();
      void removeFromList();
   };
}
