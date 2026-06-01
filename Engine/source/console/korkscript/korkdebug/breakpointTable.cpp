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

#include "breakpointTable.h"
#include "../korkCodeBlock.h"

#include "core/strings/stringFunctions.h"   // dStrcmp
#include "console/console.h"                 // Con::printf (debug logging)
#include "console/script.h"

namespace KorkScript
{
   BreakpointTable& BreakpointTable::get()
   {
      static BreakpointTable sInstance;
      return sInstance;
   }
   BreakpointTable::BreakpointTable()
   {
   }

   BreakpointTable::~BreakpointTable()
   {
   }

   void BreakpointTable::add(Breakpoint bp)
   {
      MutexHandle mutex;
      mutex.lock(&mMutex, true);

      // Replace existing entry at the same location.
      for (auto& existing : mBreakpoints)
      {
         if (existing.fileName == bp.fileName && existing.line == bp.line)
         {
            existing = bp;
            return;
         }
      }

      mBreakpoints.push_back(bp);
   }

   void BreakpointTable::remove(StringTableEntry fileName, U32 line)
   {
      MutexHandle mutex;
      mutex.lock(&mMutex, true);

      for (S32 i = (S32)mBreakpoints.size() - 1; i >= 0; --i)
      {
         if (mBreakpoints[i].fileName == fileName &&
            mBreakpoints[i].line == line)
         {
            // Swap with last and pop — O(1), order doesn't matter.
            mBreakpoints[i] = mBreakpoints[mBreakpoints.size() - 1];
            mBreakpoints.decrement();
            return;
         }
      }
   }

   void BreakpointTable::enable(StringTableEntry fileName, U32 line, bool on)
   {
      MutexHandle mutex;
      mutex.lock(&mMutex, true);

      for (auto& bp : mBreakpoints)
      {
         if (bp.fileName == fileName && bp.line == line)
         {
            bp.enabled = on;
            return;
         }
      }
   }

   bool BreakpointTable::shouldBreak(StringTableEntry fileName, U32 ip) const
   {
      MutexHandle mutex;
      mutex.lock(&mMutex, true);

      for (const auto& bp : mBreakpoints)
      {
         if (!bp.enabled)         continue;
         if (bp.fileName != fileName) continue;
         if (bp.resolvedIP != ip)     continue;

         // Conditional breakpoint — evaluate the condition.
         // A nullptr or empty condition string means unconditional.
         if (bp.condition && bp.condition[0] != '\0')
         {
            // Evaluate the condition expression in the current script context.
            // We use Con::evaluate() here so the condition can reference any
            // in-scope variable.  The lock is held during evaluation which is
            // safe because Con::evaluate() does not call back into BreakpointTable.
            Con::EvalResult result = Con::evaluate(bp.condition);
            if (!result.valid)
            {
               return false;  // condition error — don't break
            }
            return result.value.getBool();
         }
         return true;
      }
      return false;
   }

   void BreakpointTable::onCodeBlockLoaded(KorkCodeBlock* block)
   {
      if (!block) return;

      MutexHandle mutex;
      mutex.lock(&mMutex, true);

      for (Breakpoint& bp : mBreakpoints)
      {
         if (bp.fileName != block->fullPath &&
            bp.fileName != block->name)
            continue;

         // Prefer debug info resolution (exact source range matching).
         if (block->debugInfo && block->debugInfo->present)
         {
            // Search all functions for the one that contains this line.
            for (auto& fnPair : block->debugInfo->functions)
            {
               const FunctionDebugInfo& fn = fnPair.second;
               SourceLocation loc{ bp.line, 1, 0 };
               U32 ip = fn.resolveBreakIP(loc);
               if (ip != 0)
               {
                  bp.resolvedIP = ip;
                  break;
               }
            }
         }
         else
         {
            // Fall back to lineBreakPairs — find the nearest IP at or after
            // bp.line.  Mirrors findFirstBreakLine() in CodeBlock2.
            bp.resolvedIP = block->findFirstBreakLine(bp.line);
         }

#ifdef TORQUE_DEBUG
         if (bp.resolvedIP != 0)
            Con::printf("[KorkScript::BreakpointTable] Resolved breakpoint %s:%u → IP %u",
               bp.fileName, bp.line, bp.resolvedIP);
         else
            Con::warnf("[KorkScript::BreakpointTable] Could not resolve breakpoint %s:%u",
               bp.fileName, bp.line);
#endif
      }
   }

   void BreakpointTable::onCodeBlockUnloaded(KorkCodeBlock* block)
   {
      if (!block) return;

      MutexHandle mutex;
      mutex.lock(&mMutex, true);

      for (Breakpoint& bp : mBreakpoints)
      {
         if (bp.fileName == block->fullPath ||
            bp.fileName == block->name)
         {
            bp.resolvedIP = 0;  // will be re-resolved when the new block loads
         }
      }
   }

}
