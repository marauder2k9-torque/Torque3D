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

#include "tokens.h"
#include "../Diagnostics.h"
#include "../korkArena.h"

#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif

namespace KorkScript
{
   class LexerContext
   {
   public:
      /// <summary>
      /// Creates a lexer context owned by the Parser.
      /// </summary>
      /// <param name="source">Null-terminated source text. (owned by parser)</param>
      /// <param name="fileName">Used for diagnostics. May be null.</param>
      /// <param name="arena">The parserContexts arena.</param>
      LexerContext(const char* source, const char* fileName, KorkArena& arena);

      Token next();
      Token peek(U32 ahead = 0);
      SourceLocation currentLocation() const;
      const Vector<DiagnosticMessage>& diagnostics() const { return mDiagnostics; }
      bool hasErrors() const;

   private:
      static constexpr U32 LOOKAHEAD_SIZE = 4;

      Token mLookahead[LOOKAHEAD_SIZE];
      U32   mLookaheadHead = 0;     // index of oldest filled slot
      U32   mLookaheadCount = 0;    // number of filled slots

      // Fill the lookahead ring until it has at least (ahead+1) tokens.
      void fillLookahead(U32 needed);

      // Scan one token from the source stream (no lookahead buffering).
      Token scanOne();

      //---------------------------------------------------------------------------
      // Character-level source scanning
      //---------------------------------------------------------------------------
      const char*  mSource;
      U32          mPos;       ///< Byte offset of next char to consume.
      U32          mLine;      ///< 1-based line number of mPos.
      U32          mCol;       ///< 1-based column number of mPos.
      const char*  mFileName;
      KorkArena&   mArena;     ///< String storage — owned by ParseContext.

      /// Peek at current character without consuming.
      inline char curChar() const { return mSource[mPos]; }
      /// Peek one ahead without consuming.
      inline char nextChar() const { return mSource[mPos + 1]; }
      /// Consume and return current char, advancing position/line/col.
      char advance();
      /// Consume current char only if it matches c.  Returns true if matched.
      bool matchChar(char c);
      /// Are we at end of source?
      bool atEnd() const { return mSource[mPos] == '\0'; }

      //---------------------------------------------------------------------------
      // Token constructors  (each returns a fully formed Token)
      //---------------------------------------------------------------------------
      Token makeToken(TokenKind kind, SourceLocation start) const;
      Token makeIntToken(S64 val, SourceLocation start)     const;
      Token makeFloatToken(F64 val, SourceLocation start)   const;
      Token makeStrToken(TokenKind kind, const char* val, SourceLocation start) const;
      Token makeError(const char* msg, SourceLocation start);

      //---------------------------------------------------------------------------
      // Scanning helpers  (mirror legacy Sc_Scan* functions)
      //---------------------------------------------------------------------------

      /// Skip whitespace and comments.  Returns true if a doc-block was found
      /// and stored in outDocBlock (caller re-scans).
      void skipWhitespaceAndComments();

      /// Scan a block comment starting after the opening '/*'.
      /// Returns false and emits a diagnostic if EOF is reached before '*/'.
      bool scanBlockComment();

      /// Scan a doc-block comment (lines starting with '///').
      Token scanDocBlock(SourceLocation start);

      /// Scan a double-quoted string literal ("...").
      Token scanStringLiteral(SourceLocation start);

      /// Scan a single-quoted tag string literal ('...').
      Token scanTagLiteral(SourceLocation start);

      /// Scan a numeric literal (integer or float, including hex).
      Token scanNumber(SourceLocation start);

      /// Scan a variable token ($ident or %ident).
      Token scanVar(SourceLocation start);

      /// Scan an identifier or keyword.
      Token scanIdentOrKeyword(SourceLocation start);

      /// Attempt to collapse escape sequences in buf in-place.
      /// Returns false and emits a diagnostic if the escape is malformed.
      bool collapseEscapes(char* buf, SourceLocation literalStart);

      //---------------------------------------------------------------------------
      // Keyword table lookup
      //---------------------------------------------------------------------------
      static TokenKind lookupKeyword(const char* text, U32 len);

      //---------------------------------------------------------------------------
      // Diagnostics
      //---------------------------------------------------------------------------
      Vector<DiagnosticMessage> mDiagnostics;

      void emitError(SourceRange range, const char* fmt, ...);
      void emitWarning(SourceRange range, const char* fmt, ...);
   };
}
