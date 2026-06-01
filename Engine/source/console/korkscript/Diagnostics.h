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
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif


namespace KorkScript
{
   struct SourceLocation
   {
      U32 line    = 0; //< 1-based line number.
      U32 col     = 0; //< 1-based col number.
      U32 offset  = 0; //< Byte offset from start of source buffer.

      bool isValid() const { return line > 0; }

      bool operator==(const SourceLocation& o) const { return offset == o.offset; }
      bool operator< (const SourceLocation& o) const { return offset < o.offset; }
      bool operator<=(const SourceLocation& o) const { return offset <= o.offset; }
   };

   struct SourceRange
   {
      SourceLocation start;
      SourceLocation end;

      bool isValid() const { return start.isValid(); }

      /// Is loc inside this range?
      bool contains(SourceLocation loc) const
      {
         return loc.offset >= start.offset && loc.offset < end.offset;
      }

      /// Does this range overlap with other?
      bool overlaps(const SourceRange& other) const
      {
         return start.offset < other.end.offset
                && other.start.offset < end.offset;
      }

      static SourceRange invalid() { return {}; }
   };

   struct DiagnosticMessage
   {
      enum class Severity { Note, Warning, Error };

      Severity    severity = Severity::Note;
      SourceRange range;
      String      message;  ///< Human-readable description.

      bool isError()   const { return severity == Severity::Error; }
      bool isWarning() const { return severity == Severity::Warning; }
      bool isNote()    const { return severity == Severity::Note; }
   };

}
