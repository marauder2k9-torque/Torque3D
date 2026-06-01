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

#include "console/runtime.h"   // Con::Runtime, Con::EvalResult

namespace KorkScript
{
   class KorkRuntime : public Con::Runtime
   {
   public:
      KorkRuntime();
      ~KorkRuntime() override;

      //-----------------------------------------------------------------------
      // Con::Runtime interface
      //-----------------------------------------------------------------------

      Con::EvalResult evaluate(const char* source,
                               bool        echo = false,
                               const char* fileName = nullptr) override;

      Con::EvalResult evaluate(const char* source,
                               S32         frame,
                               bool        echo = false,
                               const char* fileName = nullptr) override;

      Con::EvalResult evaluatef(const char* fmt, ...) override;

      bool executeFile(const char* fileName,
                       bool        noCalls,
                       bool        journalScript) override;

      void expandEscapedCharacters(char* dest, const char* src) override;
      bool collapseEscapedCharacters(char* buf) override;

      //-----------------------------------------------------------------------
      // KorkScript specific
      //-----------------------------------------------------------------------
      bool compile(const char* filename, bool oeverrideNoDso);

   private:
      // Shared implementation used by both evaluate() overloads.
      Con::EvalResult evaluateInternal(const char* source,
         S32         frame,
         bool        echo,
         const char* fileName);
   };

   inline KorkRuntime* gRuntime = new KorkRuntime();
   inline KorkRuntime* getRuntime() { return gRuntime; }

} // namespace KorkScript
