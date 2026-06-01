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

#include "../Diagnostics.h"
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

#include <unordered_map>

namespace KorkScript
{
   struct InstructionRecord
   {
      U32         ip;
      SourceRange range;
   };

   struct LocalVarRecord
   {
      StringTableEntry    name;
      S32                 registerIndex;
      U32                 firstIP;
      U32                 lastIP;
      SourceRange         declRange;
   };

   struct FunctionDebugInfo
   {
      StringTableEntry            functionName = nullptr;
      StringTableEntry            namespaceName = nullptr;
      SourceRange                 declRange;
      Vector<InstructionRecord>   instructions;
      Vector<LocalVarRecord>      locals;
      Vector<U32>                 breakableIPs;

      U32                      resolveBreakIP(SourceLocation loc) const;
      const InstructionRecord* findRecord(U32 ip) const;
   };

   struct DebugInfo
   {
      bool             present = false;
      StringTableEntry fileName = nullptr;
      String           sourceText;

      std::unordered_map<StringTableEntry, FunctionDebugInfo> functions;
   };
}
