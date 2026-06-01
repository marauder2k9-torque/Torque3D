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

#include "debugInfo.h"

namespace KorkScript
{
   U32 FunctionDebugInfo::resolveBreakIP(SourceLocation loc) const
   {
      if (breakableIPs.empty())
         return 0;

      U32 bestIP = breakableIPs[0];
      U32 bestDist = U32(-1);

      for (U32 ip : breakableIPs)
      {
         // Find the instruction record for this IP so we can compare ranges.
         const InstructionRecord* rec = findRecord(ip);
         if (!rec)
            continue;

         // Only consider IPs that start on or after the requested location.
         if (rec->range.start.line < loc.line)
            continue;
         if (rec->range.start.line == loc.line &&
            rec->range.start.col < loc.col)
            continue;

         // Among valid candidates, pick the one whose start is closest
         // (fewest lines + columns away) to avoid jumping too far forward.
         U32 lineDist = rec->range.start.line - loc.line;
         U32 colDist = (lineDist == 0)
            ? (rec->range.start.col >= loc.col ? rec->range.start.col - loc.col : 0)
            : 0;
         U32 dist = lineDist * 10000 + colDist;

         if (dist < bestDist)
         {
            bestDist = dist;
            bestIP = ip;
         }
      }

      return bestIP;
   }

   const InstructionRecord* FunctionDebugInfo::findRecord(U32 ip) const
   {
      if (instructions.empty())
         return nullptr;

      S32 lo = 0;
      S32 hi = (S32)instructions.size() - 1;

      while (lo <= hi)
      {
         S32 mid = (lo + hi) / 2;
         if (instructions[mid].ip == ip)
            return &instructions[mid];
         else if (instructions[mid].ip < ip)
            lo = mid + 1;
         else
            hi = mid - 1;
      }

      // Not found exactly — return the record just before ip (nearest preceding
      // instruction), which is more useful than nullptr for stack traces.
      if (lo > 0)
         return &instructions[lo - 1];

      return nullptr;
   }
}
