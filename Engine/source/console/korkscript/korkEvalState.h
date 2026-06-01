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

#ifndef _CONSOLEINTERNAL_H_
#include "console/consoleInternal.h"
#endif
#ifndef _CONSOLE_CONSOLE_VALUE_STACK_H_
#include "console/consoleValueStack.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace KorkScript
{
   class Con::Module;

   class EvalState
   {
   public:
      EvalState();
      ~EvalState();

      //-----------------------------------------------------------------------
      // Current variable — used by SETCURVAR / LOADVAR / SAVEVAR opcodes
      //-----------------------------------------------------------------------
      Dictionary::Entry* currentVariable = nullptr;
      Dictionary::Entry* copyVariable = nullptr;

      //-----------------------------------------------------------------------
      // Stack
      //-----------------------------------------------------------------------
      U32   mStackDepth = 0;
      bool  mShouldReset = false;
      bool  mResetLocked = false;

      Vector<Dictionary*>       stack;
      Vector<ConsoleValueFrame> localStack;
      ConsoleValueFrame* currentRegisterArray = nullptr;

      S32 getTopOfStack() const { return (S32)mStackDepth; }
      U32 getStackDepth() const { return mStackDepth; }

      Dictionary& getCurrentFrame() { return *(stack[mStackDepth - 1]); }
      Dictionary& getFrameAt(S32 depth) { return *(stack[depth]); }

      void pushFrame(StringTableEntry frameName, Namespace* ns, S32 registerCount);
      void popFrame();
      void pushFrameRef(S32 stackIndex);
      void pushDebugFrame(S32 stackIndex);

      //-----------------------------------------------------------------------
      // Named variable access (global / dictionary-backed)
      //-----------------------------------------------------------------------
      void        setCurVarName(StringTableEntry name);
      void        setCurVarNameCreate(StringTableEntry name);

      S32         getIntVariable()            const;
      F64         getFloatVariable()          const;
      const char* getStringVariable()         const;
      void        setIntVariable(S32 val);
      void        setFloatVariable(F64 val);
      void        setStringVariable(const char* str);

      //-----------------------------------------------------------------------
      // Register access (local / register-backed) — inlined for VM hot path
      //-----------------------------------------------------------------------
      TORQUE_FORCEINLINE S32 getLocalIntVariable(S32 reg) const
      {
         return currentRegisterArray->values[reg].getInt();
      }

      TORQUE_FORCEINLINE F64 getLocalFloatVariable(S32 reg) const
      {
         return currentRegisterArray->values[reg].getFloat();
      }

      TORQUE_FORCEINLINE const char* getLocalStringVariable(S32 reg) const
      {
         return currentRegisterArray->values[reg].getString();
      }

      TORQUE_FORCEINLINE void setLocalIntVariable(S32 reg, S64 val)
      {
         currentRegisterArray->values[reg].setInt(val);
      }

      TORQUE_FORCEINLINE void setLocalFloatVariable(S32 reg, F64 val)
      {
         currentRegisterArray->values[reg].setFloat(val);
      }

      TORQUE_FORCEINLINE void setLocalStringVariable(S32 reg, const char* val, S32 len)
      {
         currentRegisterArray->values[reg].setString(val, len);
      }

      TORQUE_FORCEINLINE void setLocalStringTableEntryVariable(S32 reg, StringTableEntry val)
      {
         currentRegisterArray->values[reg].setStringTableEntry(val);
      }

      TORQUE_FORCEINLINE void moveConsoleValue(S32 reg, ConsoleValue val)
      {
         currentRegisterArray->values[reg] = val;
      }

      //-----------------------------------------------------------------------
      // Integrity check (debug builds)
      //-----------------------------------------------------------------------
      void validate();
   };

   extern EvalState gEvalState;
}
