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

#include "korkEvalState.h"
#include "console/console.h"
#include "console/consoleInternal.h"

namespace KorkScript
{
   EvalState gEvalState;

   EvalState::EvalState()
   {
      VECTOR_SET_ASSOCIATION(stack);
      stack.reserve(64);
   }

   EvalState::~EvalState()
   {
      while (!stack.empty())
      {
         delete stack.last();
         stack.decrement();
      }
   }

   void EvalState::pushFrame(StringTableEntry frameName, Namespace* ns, S32 registerCount)
   {
      if (mStackDepth + 1 > stack.size())
         stack.push_back(new Dictionary);

      Dictionary& newFrame = *(stack[mStackDepth]);
      newFrame.setState();
      newFrame.scopeName = frameName;
      newFrame.scopeNamespace = ns;

      Con::pushStackFrame(stack[mStackDepth]);
      mStackDepth++;
      currentVariable = nullptr;

      AssertFatal(!newFrame.getCount(),
         "KorkScript::EvalState::pushFrame — Dictionary not empty!");

      ConsoleValue* regArray = new ConsoleValue[registerCount]();
      localStack.push_back(ConsoleValueFrame(regArray, false));
      currentRegisterArray = &localStack.last();

      AssertFatal(mStackDepth == (U32)localStack.size(),
         "KorkScript::EvalState::pushFrame — stack size mismatch");
   }

   void EvalState::popFrame()
   {
      AssertFatal(mStackDepth > 0, "KorkScript::EvalState::popFrame — stack underflow");

      Con::popStackFrame();
      mStackDepth--;
      stack[mStackDepth]->reset();
      currentVariable = nullptr;

      const ConsoleValueFrame& frame = localStack.last();
      localStack.pop_back();
      if (!frame.isReference)
         delete[] frame.values;

      currentRegisterArray = localStack.size() ? &localStack.last() : nullptr;

      AssertFatal(mStackDepth == (U32)localStack.size(),
         "KorkScript::EvalState::popFrame — stack size mismatch");
   }

   void EvalState::pushFrameRef(S32 stackIndex)
   {
      AssertFatal(stackIndex >= 0 && (U32)stackIndex < mStackDepth,
         "KorkScript::EvalState::pushFrameRef — invalid stack index");

      if (mStackDepth + 1 > stack.size())
         stack.push_back(new Dictionary);

      Dictionary& newFrame = *(stack[mStackDepth]);
      newFrame.setState(stack[stackIndex]);

      Con::pushStackFrame(stack[mStackDepth]);
      mStackDepth++;
      currentVariable = nullptr;

      ConsoleValue* values = localStack[stackIndex].values;
      localStack.push_back(ConsoleValueFrame(values, true));
      currentRegisterArray = &localStack.last();

      AssertFatal(mStackDepth == (U32)localStack.size(),
         "KorkScript::EvalState::pushFrameRef — stack size mismatch");
   }

   void EvalState::pushDebugFrame(S32 stackIndex)
   {
      pushFrameRef(stackIndex);
      Dictionary& newFrame = *(stack[mStackDepth - 1]);
      newFrame.scopeName = stack[stackIndex]->scopeName;
      newFrame.scopeNamespace = stack[stackIndex]->scopeNamespace;
      newFrame.module = stack[stackIndex]->module;
      newFrame.ip = stack[stackIndex]->ip;
   }

   void EvalState::setCurVarName(StringTableEntry name)
   {
      if (name[0] == '$')
         currentVariable = Con::gGlobalVars.lookup(name);
      else if (mStackDepth > 0)
         currentVariable = getCurrentFrame().lookup(name);
      else
         currentVariable = nullptr;
   }

   void EvalState::setCurVarNameCreate(StringTableEntry name)
   {
      if (name[0] == '$')
         currentVariable = Con::gGlobalVars.add(name);
      else if (mStackDepth > 0)
         currentVariable = getCurrentFrame().add(name);
      else
      {
         Con::warnf("KorkScript::EvalState — createVariable called at global scope");
         currentVariable = nullptr;
      }
   }

   S32 EvalState::getIntVariable() const
   {
      return currentVariable ? currentVariable->getIntValue() : 0;
   }

   F64 EvalState::getFloatVariable() const
   {
      return currentVariable ? currentVariable->getFloatValue() : 0.0;
   }

   const char* EvalState::getStringVariable() const
   {
      return currentVariable ? currentVariable->getStringValue() : "";
   }

   void EvalState::setIntVariable(S32 val)
   {
      if (currentVariable) currentVariable->setIntValue(val);
   }

   void EvalState::setFloatVariable(F64 val)
   {
      if (currentVariable) currentVariable->setFloatValue(val);
   }

   void EvalState::setStringVariable(const char* str)
   {
      if (currentVariable) currentVariable->setStringValue(str);
   }

   void EvalState::validate()
   {
      AssertFatal(mStackDepth <= (U32)stack.size(),
         "KorkScript::EvalState::validate — depth beyond last frame");
      for (U32 i = 0; i < (U32)stack.size(); ++i)
         stack[i]->validate();
   }

}
