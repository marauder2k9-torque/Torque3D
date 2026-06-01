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

#ifndef _PLATFORM_THREADS_MUTEX_H_
#include "platform/threads/mutex.h"
#endif

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace KorkScript
{
   class KorkCodeBlock;

   struct Breakpoint
   {
      enum class Kind { Line, Conditional, Tracepoint };

      Kind             kind = Kind::Line;
      StringTableEntry fileName = nullptr;
      U32              line = 0;
      U32              resolvedIP = 0;
      bool             enabled = true;
      const char* condition = nullptr;
   };

   class BreakpointTable
   {
   public:
      static BreakpointTable& get();

      BreakpointTable();
      ~BreakpointTable();

      void add(Breakpoint bp);
      void remove(StringTableEntry fileName, U32 line);
      void enable(StringTableEntry fileName, U32 line, bool on);
      bool shouldBreak(StringTableEntry fileName, U32 ip) const;

      void onCodeBlockLoaded(KorkCodeBlock* block);
      void onCodeBlockUnloaded(KorkCodeBlock* block);

   private:
      mutable Mutex  mMutex;
      Vector<Breakpoint>  mBreakpoints;
   };

}
