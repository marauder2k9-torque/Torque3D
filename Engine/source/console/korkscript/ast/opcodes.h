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

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

namespace KorkScript
{
   enum class Op : U32
   {
      FuncDecl = 0,
      DefaultEnd = 1,
      CreateObject = 2,
      AddObject = 3,
      EndObject = 4,
      FinishObject = 5,

      JmpIffNot = 6,
      JmpIfNot = 7,
      JmpNotString = 8,
      JmpIff = 9,
      JmpIf = 10,
      JmpIfNotNP = 11,
      JmpIfNP = 12,
      Jmp = 13,
      Return = 14,
      ReturnVoid = 15,
      ReturnFlt = 16,
      ReturnUInt = 17,

      CmpEq = 18,
      CmpGr = 19,
      CmpGe = 20,
      CmpLt = 21,
      CmpLe = 22,
      CmpNe = 23,
      Xor = 24,
      Mod = 25,
      BitAnd = 26,
      BitOr = 27,
      Not = 28,
      NotF = 29,
      OnesComplement = 30,

      Shr = 31,
      Shl = 32,
      And = 33,
      Or = 34,

      Add = 35,
      Sub = 36,
      Mul = 37,
      Div = 38,
      Neg = 39,
      Inc = 40,

      SetCurVar = 41,
      SetCurVarCreate = 42,
      SetCurVarArray = 43,
      SetCurVarArrayCreate = 44,

      LoadVarUInt = 45,
      LoadVarFlt = 46,
      LoadVarStr = 47,

      SaveVarUInt = 48,
      SaveVarFlt = 49,
      SaveVarStr = 50,

      LoadLocalVarUInt = 51,
      LoadLocalVarFlt = 52,
      LoadLocalVarStr = 53,

      SaveLocalVarUInt = 54,
      SaveLocalVarFlt = 55,
      SaveLocalVarStr = 56,

      SetCurObject = 57,
      SetCurObjectNew = 58,
      SetCurObjectInternal = 59,

      SetCurField = 60,
      SetCurFieldArray = 61,
      SetCurFieldType = 62,

      LoadFieldUInt = 63,
      LoadFieldFlt = 64,
      LoadFieldStr = 65,

      SaveFieldUInt = 66,
      SaveFieldFlt = 67,
      SaveFieldStr = 68,

      PopStk = 69,

      LoadImmedUInt = 70,
      LoadImmedFlt = 71,
      TagToStr = 72,
      LoadImmedStr = 73,
      DocBlockStr = 74,
      LoadImmedIdent = 75,

      CallFunc = 76,

      AdvanceStrAppendChar = 77,
      RewindStr = 78,
      TerminateRewindStr = 79,

      CompareStr = 80,

      Push = 81,
      PushFrame = 82,

      Assert = 83,
      Break = 84,

      IterBegin = 85,
      IterBeginStr = 86,
      Iter = 87,
      IterEnd = 88,

      Invalid = 89,
      MaxOpCodeLen = 90,
   };

   // Convenience cast for CodeStream::emit
   inline U32 op(Op o) { return static_cast<U32>(o); }
}
