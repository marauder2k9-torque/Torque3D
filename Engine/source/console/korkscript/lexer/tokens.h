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

#include "../Diagnostics.h"

namespace KorkScript
{
   enum class TokenKind : U32
   {
      //-----------------------------------------------------------------------
      // Literals
      //-----------------------------------------------------------------------
      IntLiteral,          // [0-9]+ or 0x[0-9a-fA-F]+ or true/false
      FloatLiteral,        // [0-9]*\.[0-9]+ or scientific notation
      StringLiteral,       // "..." — STRATOM in legacy
      TagStringLiteral,    // '...' — TAGATOM in legacy (tag strings)
      DocBlock,            // /// ... block comment above a function

      //-----------------------------------------------------------------------
      // Identifiers and variables
      //-----------------------------------------------------------------------
      Ident,               // [A-Za-z_][A-Za-z0-9_]* — plain identifier
      TypeIdent,           // identifier that matches a registered engine type
      Var,                 // [$%][A-Za-z_][A-Za-z0-9_:]* — script variable
      TTag,                // %[0-9]+ style tag variable

      //-----------------------------------------------------------------------
      // Legacy Keywords
      //-----------------------------------------------------------------------
      KwAssert,            // assert
      KwBreak,             // break
      KwCase,              // case
      KwCaseOr,            // or   (used inside switch cases)
      KwContinue,          // continue
      KwDatablock,         // datablock
      KwDefault,           // default
      KwDo,                // do
      KwElse,              // else
      KwFor,               // for
      KwForeach,           // foreach
      KwForeachStr,        // foreach$
      KwFunction,          // function
      KwIf,                // if
      KwIn,                // in
      KwNew,               // new
      KwPackage,           // package
      KwReturn,            // return
      KwSingleton,         // singleton
      KwSwitch,            // switch
      KwSwitchStr,         // switch$
      KwWhile,             // while

      //-----------------------------------------------------------------------
      // KorkScript added Keywords
      //-----------------------------------------------------------------------
      KwConst,             // const
      KwEnum,              // enum
      KwPragma,            // #pragma  (lexed as a keyword, not a preprocessor token)

      //-----------------------------------------------------------------------
      // Operators
      //-----------------------------------------------------------------------
      // Multi-Char ops
      OpAnd,               // &&
      OpAndAssign,         // &=
      OpCat,               // @   — plain string concatenation (appendChar = 0)
      OpCatNl,             // NL  — concat with newline (appendChar = '\n')
      OpCatSpc,            // SPC — concat with space (appendChar = ' ')
      OpCatTab,            // TAB — concat with tab (appendChar = '\t')
      OpColonColon,        // ::
      OpDivAssign,         // /=
      OpEq,                // ==
      OpGe,                // >=
      OpIntName,           // ->
      OpIntNameR,          // -->
      OpLe,                // <=
      OpMinusMinus,        // --
      OpMinusAssign,       // -=
      OpModAssign,         // %=
      OpMulAssign,         // *=
      OpNe,                // !=
      OpOr,                // ||
      OpOrAssign,          // |=
      OpPlusAssign,        // +=
      OpPlusPlus,          // ++
      OpShl,               // <<
      OpShlAssign,         // <<=
      OpShr,               // >>
      OpShrAssign,         // >>=
      OpStrEq,             // $=
      OpStrNe,             // !$=
      OpXorAssign,         // ^=

      // Single char ops.
      Punct,               // catch-all; parser uses the named aliases below
      Ampersand,           // &
      At,                  // @  (when not followed by nothing — plain concat; kept for completeness)
      Bang,                // !
      Caret,               // ^
      Colon,               // :
      Comma,               // ,
      Dot,                 // .
      Equals,              // =
      ForwardSlash,        // /
      GreaterThan,         // >
      Hash,                // #  (used for #pragma)
      LBrace,              // {
      LBracket,            // [
      LParen,              // (
      LessThan,            // <
      Minus,               // -
      Percent,             // %
      Pipe,                // |
      Plus,                // +
      Question,            // ?
      RBrace,              // }
      RBracket,            // ]
      RParen,              // )
      Semicolon,           // ;
      Star,                // *
      Tilde,               // ~

      //-----------------------------------------------------------------------
      // Meta
      //-----------------------------------------------------------------------
      Eof,                 // end of input
      Error,               // illegal / unrecognised character
   };

   struct Token
   {
      TokenKind   kind = TokenKind::Eof;
      SourceRange range;

      // Value — only the field matching the kind is valid.
      S64         intVal = 0;       // IntLiteral, Punct (ASCII), KwXxx (line no.)
      F64         floatVal = 0.0;   // FloatLiteral
      const char* strVal = nullptr; // StringLiteral, TagLiteral, Ident,
                                    // TypeIdent, Var, TTag, DocBlock
                                    // (all interned into StringTable or arena)

      // For OpCat / OpCatNl / OpCatSpc / OpCatTab: the separator character.
      // 0 = no separator (plain @), '\n' = NL, ' ' = SPC, '\t' = TAB.
      char        appendChar = 0;

      // Convenience
      bool is(TokenKind k)        const { return kind == k; }
      bool isIdent()              const { return kind == TokenKind::Ident; }
      bool isVar()                const { return kind == TokenKind::Var; }
      bool isLiteral()            const
      {
         return kind == TokenKind::IntLiteral
                        || kind == TokenKind::FloatLiteral
                        || kind == TokenKind::StringLiteral
                        || kind == TokenKind::TagStringLiteral;
      }
      bool isEof()                const { return kind == TokenKind::Eof; }
      bool isError()              const { return kind == TokenKind::Error; }
      U32  line()                 const { return range.start.line; }
   };
}
