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

#include "lexer.h"
#include "console/console.h"
#include "console/consoleObject.h" 
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"

namespace KorkScript
{
   struct KeywordEntry { const char* text; TokenKind kind; };

   static const KeywordEntry sKeywords[] =
   {
      // Booleans — emit as IntLiteral with intVal 0/1
      // Most frequent first
      { "function",   TokenKind::KwFunction   },
      { "if",         TokenKind::KwIf         },
      { "else",       TokenKind::KwElse       },
      { "return",     TokenKind::KwReturn     },
      { "for",        TokenKind::KwFor        },
      { "while",      TokenKind::KwWhile      },
      { "do",         TokenKind::KwDo         },
      { "break",      TokenKind::KwBreak      },
      { "continue",   TokenKind::KwContinue   },
      { "new",        TokenKind::KwNew        },
      { "datablock",  TokenKind::KwDatablock  },
      { "package",    TokenKind::KwPackage    },
      { "switch",     TokenKind::KwSwitch     },
      { "switch$",    TokenKind::KwSwitchStr  },
      { "case",       TokenKind::KwCase       },
      { "default",    TokenKind::KwDefault    },
      { "foreach",    TokenKind::KwForeach    },
      { "foreach$",   TokenKind::KwForeachStr },
      { "in",         TokenKind::KwIn         },
      { "or",         TokenKind::KwCaseOr     },
      { "singleton",  TokenKind::KwSingleton  },
      { "assert",     TokenKind::KwAssert     },
      { "const",      TokenKind::KwConst      },
      { "enum",       TokenKind::KwEnum       },
   };
   static const U32 sKeywordCount = sizeof(sKeywords) / sizeof(sKeywords[0]);

   LexerContext::LexerContext(const char* source, const char* fileName, KorkArena& arena)
      : mSource(source)
      , mPos(0)
      , mLine(1)
      , mCol(1)
      , mFileName(fileName)
      , mArena(arena)
   {
   }

   bool LexerContext::hasErrors() const
   {
      for (const DiagnosticMessage& d : mDiagnostics)
         if (d.isError()) return true;

      return false;
   }

   SourceLocation LexerContext::currentLocation() const
   {
      return { mLine, mCol, mPos };
   }

   char LexerContext::advance()
   {
      char c = mSource[mPos++];
      if (c == '\n') { mLine++; mCol = 1; }
      else { mCol++; }
      return c;
   }

   bool LexerContext::matchChar(char c)
   {
      if (mSource[mPos] == c) { advance(); return true; }
      return false;
   }

   Token LexerContext::makeToken(TokenKind kind, SourceLocation start) const
   {
      Token t;
      t.kind = kind;
      t.range.start = start;
      t.range.end = currentLocation();
      return t;
   }

   Token LexerContext::makeIntToken(S64 val, SourceLocation start) const
   {
      Token t = makeToken(TokenKind::IntLiteral, start);
      t.intVal = val;
      return t;
   }

   Token LexerContext::makeFloatToken(F64 val, SourceLocation start) const
   {
      Token t = makeToken(TokenKind::FloatLiteral, start);
      t.floatVal = val;
      return t;
   }

   Token LexerContext::makeStrToken(TokenKind kind, const char* val,
      SourceLocation start) const
   {
      Token t = makeToken(kind, start);
      t.strVal = val;
      return t;
   }

   Token LexerContext::makeError(const char* msg, SourceLocation start)
   {
      SourceRange range{ start, currentLocation() };
      emitError(range, "%s", msg);
      Token t;
      t.kind = TokenKind::Error;
      t.range = range;
      return t;
   }

   void LexerContext::emitError(SourceRange range, const char* fmt, ...)
   {
      char buf[512];
      va_list args;
      va_start(args, fmt);
      vsnprintf(buf, sizeof(buf), fmt, args);
      va_end(args);

      DiagnosticMessage msg;
      msg.severity = DiagnosticMessage::Severity::Error;
      msg.range = range;
      msg.message = buf;
      mDiagnostics.push_back(msg);
   }

   void LexerContext::emitWarning(SourceRange range, const char* fmt, ...)
   {
      char buf[512];
      va_list args;
      va_start(args, fmt);
      vsnprintf(buf, sizeof(buf), fmt, args);
      va_end(args);

      DiagnosticMessage msg;
      msg.severity = DiagnosticMessage::Severity::Warning;
      msg.range = range;
      msg.message = buf;
      mDiagnostics.push_back(msg);
   }

   void LexerContext::fillLookahead(U32 needed)
   {
      while (mLookaheadCount < needed && mLookaheadCount < LOOKAHEAD_SIZE)
      {
         U32 idx = (mLookaheadHead + mLookaheadCount) % LOOKAHEAD_SIZE;
         mLookahead[idx] = scanOne();
         mLookaheadCount++;
      }
   }

   Token LexerContext::peek(U32 ahead)
   {
      if (ahead >= LOOKAHEAD_SIZE)
         ahead = LOOKAHEAD_SIZE - 1;
      fillLookahead(ahead + 1);
      U32 idx = (mLookaheadHead + ahead) % LOOKAHEAD_SIZE;
      return mLookahead[idx];
   }

   Token LexerContext::next()
   {
      fillLookahead(1);
      Token t = mLookahead[mLookaheadHead];
      mLookaheadHead = (mLookaheadHead + 1) % LOOKAHEAD_SIZE;
      mLookaheadCount--;
      return t;
   }

   void LexerContext::skipWhitespaceAndComments()
   {
      for (;;)
      {
         // Whitespace
         while (!atEnd() && (curChar() == ' ' || curChar() == '\t' ||
            curChar() == '\v' || curChar() == '\f' ||
            curChar() == '\r' || curChar() == '\n'))
         {
            advance();
         }

         if (atEnd()) return;

         // Line comment?
         if (curChar() == '/' && nextChar() == '/')
         {
            // Doc block?  Don't skip — return to scanOne() which handles it.
            if (mSource[mPos + 2] == '/')
               return;

            // Plain line comment — consume to end of line.
            while (!atEnd() && curChar() != '\n')
               advance();
            continue;
         }

         // Block comment?
         if (curChar() == '/' && nextChar() == '*')
         {
            advance(); advance(); // consume '/*'
            SourceLocation start = currentLocation();
            if (!scanBlockComment())
            {
               SourceRange r{ start, currentLocation() };
               emitError(r, "Unexpected end of file inside block comment.");
            }
            continue;
         }

         // Nothing left to skip.
         return;
      }
   }

   bool LexerContext::scanBlockComment()
   {
      while (!atEnd())
      {
         char c = advance();
         if (c == '*' && curChar() == '/')
         {
            advance(); // consume '/'
            return true;
         }
      }
      return false; // EOF — caller emits error
   }

   Token LexerContext::scanDocBlock(SourceLocation start)
   {
      // Accumulate raw text until we see a line that does NOT start with '///'
      // (after optional whitespace).
      static char buf[4096];
      U32 outLen = 0;

      while (!atEnd())
      {
         // Must be at the start of a '///' line.
         if (!(curChar() == '/' && nextChar() == '/' && mSource[mPos + 2] == '/'))
            break;

         // Skip '///'
         advance(); advance(); advance();

         // Copy line content (excluding \n\r) into buf.
         while (!atEnd() && curChar() != '\n' && curChar() != '\r')
         {
            if (outLen < sizeof(buf) - 1)
               buf[outLen++] = advance();
            else
               advance(); // overflow — discard
         }
         if (outLen < sizeof(buf) - 1)
            buf[outLen++] = '\n';

         // Skip the line ending.
         if (!atEnd() && curChar() == '\r') advance();
         if (!atEnd() && curChar() == '\n') advance();

         // Skip leading whitespace on the next line before checking for '///'.
         while (!atEnd() && (curChar() == ' ' || curChar() == '\t'))
            advance();
      }

      buf[outLen] = '\0';

      return makeStrToken(TokenKind::DocBlock, mArena.allocString(buf, outLen), start);
   }

   Token LexerContext::scanStringLiteral(SourceLocation start)
   {
      advance(); // consume opening '"'

      static char buf[4096];
      U32 len = 0;

      while (!atEnd() && curChar() != '"')
      {
         if (curChar() == '\n' || curChar() == '\r')
         {
            SourceRange r{ start, currentLocation() };
            emitError(r, "Unterminated string literal.");
            break;
         }

         if (curChar() == '\\')
         {
            if (len < sizeof(buf) - 2)
            {
               buf[len++] = advance(); // consume '\'
               if (!atEnd() && curChar() != '\n' && curChar() != '\r')
                  buf[len++] = advance(); // consume escaped char
            }
            else
            {
               advance();
               if (!atEnd()) advance();
            }
            continue;
         }

         if (len < sizeof(buf) - 1)
            buf[len++] = advance();
         else
            advance(); // overflow — discard
      }

      if (!atEnd()) advance(); // consume closing '"'
      buf[len] = '\0';

      if (!collapseEscapes(buf, start))
         return makeError("Malformed escape sequence in string literal.", start);

      return makeStrToken(TokenKind::StringLiteral,
         mArena.allocString(buf), start);
   }

   Token LexerContext::scanTagLiteral(SourceLocation start)
   {
      advance(); // consume opening '\''

      static char buf[4096];
      U32 len = 0;

      while (!atEnd() && curChar() != '\'')
      {
         if (curChar() == '\n' || curChar() == '\r')
         {
            SourceRange r{ start, currentLocation() };
            emitError(r, "Unterminated tag literal.");
            break;
         }
         // Same backslash handling as scanStringLiteral — \' must not terminate.
         if (curChar() == '\\')
         {
            if (len < sizeof(buf) - 2)
            {
               buf[len++] = advance();
               if (!atEnd() && curChar() != '\n' && curChar() != '\r')
                  buf[len++] = advance();
            }
            else
            {
               advance();
               if (!atEnd()) advance();
            }
            continue;
         }
         if (len < sizeof(buf) - 1)
            buf[len++] = advance();
         else
            advance();
      }

      if (!atEnd()) advance(); // consume closing '\''
      buf[len] = '\0';

      if (!collapseEscapes(buf, start))
         return makeError("Malformed escape sequence in tag literal.", start);

      return makeStrToken(TokenKind::TagStringLiteral,
         mArena.allocString(buf), start);
   }

   Token LexerContext::scanNumber(SourceLocation start)
   {
      // Hex literal?
      if (curChar() == '0' && (nextChar() == 'x' || nextChar() == 'X'))
      {
         advance(); advance(); // skip '0x'
         S64 val = 0;
         bool anyDigit = false;
         while (!atEnd())
         {
            char c = curChar();
            int  d = -1;
            if (c >= '0' && c <= '9')       d = c - '0';
            else if (c >= 'a' && c <= 'f')  d = c - 'a' + 10;
            else if (c >= 'A' && c <= 'F')  d = c - 'A' + 10;
            if (d < 0) break;
            val = val * 16 + d;
            anyDigit = true;
            advance();
         }
         if (!anyDigit)
            return makeError("Expected hex digits after '0x'.", start);
         return makeIntToken(val, start);
      }

      // Decimal integer or float.
      // Collect the whole raw text and let strtod/strtol decide.
      static char buf[64];
      U32 len = 0;

      while (!atEnd() && curChar() >= '0' && curChar() <= '9')
      {
         if (len < sizeof(buf) - 1) buf[len++] = advance();
         else advance();
      }

      // Float?
      bool isFloat = false;
      if (!atEnd() && curChar() == '.')
      {
         isFloat = true;
         if (len < sizeof(buf) - 1) buf[len++] = advance();
         while (!atEnd() && curChar() >= '0' && curChar() <= '9')
         {
            if (len < sizeof(buf) - 1) buf[len++] = advance();
            else advance();
         }
      }

      // Scientific notation?
      if (!atEnd() && (curChar() == 'e' || curChar() == 'E'))
      {
         isFloat = true;
         if (len < sizeof(buf) - 1) buf[len++] = advance();
         if (!atEnd() && (curChar() == '+' || curChar() == '-'))
            if (len < sizeof(buf) - 1) buf[len++] = advance();
         while (!atEnd() && curChar() >= '0' && curChar() <= '9')
         {
            if (len < sizeof(buf) - 1) buf[len++] = advance();
            else advance();
         }
      }

      buf[len] = '\0';

      if (isFloat)
         return makeFloatToken(strtod(buf, nullptr), start);
      else
         return makeIntToken((S64)strtol(buf, nullptr, 10), start);
   }

   Token LexerContext::scanVar(SourceLocation start)
   {
      static char buf[256];
      U32 len = 0;

      // Consume sigil
      buf[len++] = advance(); // '$' or '%'

      // Must be followed by a letter or underscore to be a valid variable.
      // A lone '%' is the modulo operator — scanOne() handles that before
      // calling scanVar(), so we should always have a valid start here.
      while (!atEnd())
      {
         char c = curChar();
         // Variables can contain letters, digits, underscores, and colons
         // (for namespace-qualified globals like $Namespace::var).
         if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == ':')
         {
            if (len < sizeof(buf) - 1) buf[len++] = advance();
            else advance();
         }
         else break;
      }

      buf[len] = '\0';

      // Intern into StringTable, same as legacy Sc_ScanVar.
      StringTableEntry ste = StringTable->insert(buf);
      return makeStrToken(TokenKind::Var, ste, start);
   }

   Token LexerContext::scanIdentOrKeyword(SourceLocation start)
   {
      static char buf[256];
      U32 len = 0;

      while (!atEnd())
      {
         char c = curChar();
         if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '_' || c == '$')
         {
            if (len < sizeof(buf) - 1) buf[len++] = advance();
            else advance();
         }
         else break;
      }
      buf[len] = '\0';

      // Boolean literals — same as legacy (returns INTCONST)
      if (dStrcmp(buf, "true") == 0) return makeIntToken(1, start);
      if (dStrcmp(buf, "false") == 0) return makeIntToken(0, start);

      // Concatenation keyword-operators — return the appropriate OpCatXxx token.
      // These must be checked BEFORE the keyword table to ensure "NL" etc.
      // are not accidentally parsed as identifiers.
      if (dStrcmp(buf, "NL") == 0)
      {
         Token t = makeToken(TokenKind::OpCatNl, start);
         t.appendChar = '\n';
         return t;
      }
      if (dStrcmp(buf, "SPC") == 0)
      {
         Token t = makeToken(TokenKind::OpCatSpc, start);
         t.appendChar = ' ';
         return t;
      }
      if (dStrcmp(buf, "TAB") == 0)
      {
         Token t = makeToken(TokenKind::OpCatTab, start);
         t.appendChar = '\t';
         return t;
      }

      // Keyword table lookup
      TokenKind kw = lookupKeyword(buf, len);
      if (kw != TokenKind::Ident)
         return makeToken(kw, start);

      // Engine type name?  Mirrors Sc_ScanIdent's ConsoleBaseType check.
      ConsoleBaseType* type = ConsoleBaseType::getTypeByName(buf);
      if (type)
      {
         Token t = makeToken(TokenKind::TypeIdent, start);
         t.intVal = type->getTypeID();
         return t;
      }

      // Plain identifier — intern into StringTable.
      StringTableEntry ste = StringTable->insert(buf);
      return makeStrToken(TokenKind::Ident, ste, start);
   }

   TokenKind LexerContext::lookupKeyword(const char* text, U32 /*len*/)
   {
      for (U32 i = 0; i < sKeywordCount; ++i)
         if (dStrcmp(text, sKeywords[i].text) == 0)
            return sKeywords[i].kind;
      return TokenKind::Ident; // not a keyword
   }

   bool LexerContext::collapseEscapes(char* buf, SourceLocation literalStart)
   {
      // Delegate to the existing collapseEscape function from the legacy lexer
      // which is declared extern in parser.h and linked from CMDscan.cpp.
      // This guarantees byte-for-byte identical escape processing.
      if (!collapseEscape(buf))
      {
         SourceRange r{ literalStart, currentLocation() };
         emitError(r, "Invalid escape sequence in string literal.");
         return false;
      }
      return true;
   }

   Token LexerContext::scanOne()
   {
      skipWhitespaceAndComments();

      if (atEnd())
         return makeToken(TokenKind::Eof, currentLocation());

      SourceLocation start = currentLocation();
      char c = curChar();

      //-----------------------------------------------------------------------
      // Doc block  ///
      //-----------------------------------------------------------------------
      if (c == '/' && nextChar() == '/' && mSource[mPos + 2] == '/')
         return scanDocBlock(start);

      //-----------------------------------------------------------------------
      // String and tag literals
      //-----------------------------------------------------------------------
      if (c == '"')  return scanStringLiteral(start);
      if (c == '\'') return scanTagLiteral(start);

      //-----------------------------------------------------------------------
      // Numeric literals
      //-----------------------------------------------------------------------
      if (c >= '0' && c <= '9')
         return scanNumber(start);

      //-----------------------------------------------------------------------
      // Variables  $ and %
      // Check % here: if followed by a letter/underscore it's a variable,
      // otherwise it's the modulo/percent operator.
      //-----------------------------------------------------------------------
      if (c == '$' && ((nextChar() >= 'A' && nextChar() <= 'Z') ||
         (nextChar() >= 'a' && nextChar() <= 'z') ||
         nextChar() == '_'))
         return scanVar(start);
      if (c == '%' && ((nextChar() >= 'A' && nextChar() <= 'Z') ||
         (nextChar() >= 'a' && nextChar() <= 'z') ||
         nextChar() == '_'))
         return scanVar(start);
      if (c == '$')
      {
         // $= is the string-equality operator — must be checked BEFORE scanVar
         // or the '$' would be consumed as the start of a (lone) variable name.
         if (nextChar() == '=')
         {
            advance(); advance(); // consume '$' and '='
            return makeToken(TokenKind::OpStrEq, start);
         }
         return scanVar(start);
      }

      //-----------------------------------------------------------------------
      // #pragma  — KorkScript file-level directive
      // Lexed as a single KwPragma token; the parser reads the word that follows.
      //-----------------------------------------------------------------------
      if (c == '#')
      {
         advance(); // consume '#'
         // peek at following text to confirm "pragma"
         SourceLocation afterHash = currentLocation();
         Token identTok = scanIdentOrKeyword(afterHash);
         if (identTok.strVal && dStrcmp(identTok.strVal, "pragma") == 0)
            return makeToken(TokenKind::KwPragma, start);
         // Not "pragma" — emit error, return Error token.
         return makeError("Unknown preprocessor directive. Only '#pragma' is supported in KorkScript.", start);
      }

      //-----------------------------------------------------------------------
      // Identifiers and keywords  (including NL/SPC/TAB/true/false)
      //-----------------------------------------------------------------------
      if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_')
         return scanIdentOrKeyword(start);

      //-----------------------------------------------------------------------
      // Multi-character operators — check longest match first
      //-----------------------------------------------------------------------
      advance(); // consume first character

      switch (c)
      {
      case '=':
         if (matchChar('=')) return makeToken(TokenKind::OpEq, start);
         return makeToken(TokenKind::Equals, start);

      case '!':
         if (curChar() == '$' && mSource[mPos + 1] == '=')
         {
            advance(); advance(); // consume '$='
            return makeToken(TokenKind::OpStrNe, start);
         }
         if (matchChar('=')) return makeToken(TokenKind::OpNe, start);
         return makeToken(TokenKind::Bang, start);

      case '<':
         if (matchChar('<'))
         {
            if (matchChar('=')) return makeToken(TokenKind::OpShlAssign, start);
            return makeToken(TokenKind::OpShl, start);
         }
         if (matchChar('=')) return makeToken(TokenKind::OpLe, start);
         return makeToken(TokenKind::LessThan, start);

      case '>':
         if (matchChar('>'))
         {
            if (matchChar('=')) return makeToken(TokenKind::OpShrAssign, start);
            return makeToken(TokenKind::OpShr, start);
         }
         if (matchChar('=')) return makeToken(TokenKind::OpGe, start);
         return makeToken(TokenKind::GreaterThan, start);

      case '&':
         if (matchChar('&')) return makeToken(TokenKind::OpAnd, start);
         if (matchChar('=')) return makeToken(TokenKind::OpAndAssign, start);
         return makeToken(TokenKind::Ampersand, start);

      case '|':
         if (matchChar('|')) return makeToken(TokenKind::OpOr, start);
         if (matchChar('=')) return makeToken(TokenKind::OpOrAssign, start);
         return makeToken(TokenKind::Pipe, start);

      case '+':
         if (matchChar('+')) return makeToken(TokenKind::OpPlusPlus, start);
         if (matchChar('=')) return makeToken(TokenKind::OpPlusAssign, start);
         return makeToken(TokenKind::Plus, start);

      case '-':
         if (matchChar('-')) return makeToken(TokenKind::OpMinusMinus, start);
         if (matchChar('=')) return makeToken(TokenKind::OpMinusAssign, start);
         if (matchChar('>'))
         {
            if (matchChar('>')) return makeToken(TokenKind::OpIntNameR, start);
            return makeToken(TokenKind::OpIntName, start);
         }
         return makeToken(TokenKind::Minus, start);

      case '*':
         if (matchChar('=')) return makeToken(TokenKind::OpMulAssign, start);
         return makeToken(TokenKind::Star, start);

      case '/':
         if (matchChar('=')) return makeToken(TokenKind::OpDivAssign, start);
         return makeToken(TokenKind::ForwardSlash, start);

      case '%':
         if (matchChar('=')) return makeToken(TokenKind::OpModAssign, start);
         return makeToken(TokenKind::Percent, start);

      case '^':
         if (matchChar('=')) return makeToken(TokenKind::OpXorAssign, start);
         return makeToken(TokenKind::Caret, start);

      case ':':
         if (matchChar(':')) return makeToken(TokenKind::OpColonColon, start);
         return makeToken(TokenKind::Colon, start);

      case '@':
      {
         Token t = makeToken(TokenKind::OpCat, start);
         t.appendChar = 0;
         return t;
      }

      case '$':
         if (matchChar('=')) return makeToken(TokenKind::OpStrEq, start);
         // bare '$' with no following letter was already handled above in scanVar;
         // reaching here means it was a lone '$' — emit error.
         return makeError("Unexpected '$' — did you mean a variable like '$name'?", start);

         //-----------------------------------------------------------------------
         // Single-character punctuation
         //-----------------------------------------------------------------------
      case '(': return makeToken(TokenKind::LParen, start);
      case ')': return makeToken(TokenKind::RParen, start);
      case '{': return makeToken(TokenKind::LBrace, start);
      case '}': return makeToken(TokenKind::RBrace, start);
      case '[': return makeToken(TokenKind::LBracket, start);
      case ']': return makeToken(TokenKind::RBracket, start);
      case ';': return makeToken(TokenKind::Semicolon, start);
      case ',': return makeToken(TokenKind::Comma, start);
      case '.': return makeToken(TokenKind::Dot, start);
      case '?': return makeToken(TokenKind::Question, start);
      case '~': return makeToken(TokenKind::Tilde, start);

      default:
         return makeError("Unexpected character.", start);
      }
   }
} // namespace

