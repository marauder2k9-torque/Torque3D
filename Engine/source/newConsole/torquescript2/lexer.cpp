#include "newConsole/torquescript2/lexer.h"

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STRINGFUNCTIONS_H_
#include "core/strings/stringFunctions.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      namespace
      {
         /// Mirrors charConv (CMDscan.l): r/n/t map to control chars,
         /// everything else passes through unchanged.
         char charConvEscape(char c)
         {
            switch (c)
            {
            case 'r': return '\r';
            case 'n': return '\n';
            case 't': return '\t';
            default:  return c;
            }
         }
      }

      Lexer::Lexer(const char* source, const char* originName, TypeNameLookup typeNameLookup)
         : mSource(source ? source : "")
         , mPos(mSource)
         , mEnd(mSource + dStrlen(mSource))
         , mOriginName(StringTable->insert(originName ? originName : "<unknown>"))
         , mTypeNameLookup(std::move(typeNameLookup))
      {
      }

      void Lexer::reportError(U32 line, U32 column, const char* message)
      {
         LexDiagnostic d;
         d.line = line;
         d.column = column;
         d.message = message;
         mDiagnostics.push_back(d);
      }

      bool Lexer::isAtEnd() const
      {
         return mPos >= mEnd;
      }

      char Lexer::peek(S32 offset) const
      {
         const char* p = mPos + offset;
         if (p < mSource || p >= mEnd)
            return '\0';
         return *p;
      }

      char Lexer::advance()
      {
         char c = *mPos;
         ++mPos;
         if (c == '\n')
         {
            ++mLine;
            mColumn = 1;
         }
         else
         {
            ++mColumn;
         }
         return c;
      }

      bool Lexer::match(char expected)
      {
         if (isAtEnd() || *mPos != expected)
            return false;
         advance();
         return true;
      }

      void Lexer::skipWhitespaceAndComments()
      {
         for (;;)
         {
            char c = peek();

            if (c == ' ' || c == '\t' || c == '\v' || c == '\f' || c == '\r')
            {
               advance();
               continue;
            }
            if (c == '\n')
            {
               advance();
               continue;
            }

            // "///" doc-blocks are handled by scanDocBlock before this
            // loop ever sees them - a leading "///" is ruled out by the
            // caller first. Plain "//" and "/* */" comments consumed here.
            if (c == '/' && peek(1) == '/' && peek(2) != '/')
            {
               while (!isAtEnd() && peek() != '\n')
                  advance();
               continue;
            }
            if (c == '/' && peek(1) == '*')
            {
               advance(); advance(); // consume "/*"
               U32 startLine = mLine, startColumn = mColumn;
               while (!isAtEnd() && !(peek() == '*' && peek(1) == '/'))
                  advance();
               if (isAtEnd())
               {
                  reportError(startLine, startColumn, "unexpected end of file inside block comment");
                  return;
               }
               advance(); advance(); // consume "*/"
               continue;
            }

            break;
         }
      }

      Token Lexer::scanIdentifierOrKeyword()
      {
         U32 line = mLine, column = mColumn;
         const char* start = mPos;

         while (!isAtEnd() && (isalnum((unsigned char)peek()) || peek() == '_'))
            advance();

         // "switch$"/"foreach$" are the only reserved words with a
         // trailing '$'; elsewhere '$' starts a Var token. Narrow check:
         // only fires for these two exact keywords followed by '$'.
         // Without it they'd lex as Ident + unrelated '$' - caught by
         // testing, had zero prior coverage.
         String scannedSoFar(start, static_cast<String::SizeType>(mPos - start));
         if (peek() == '$' && (scannedSoFar.equal("switch") || scannedSoFar.equal("foreach")))
            advance();

         String text(start, static_cast<String::SizeType>(mPos - start));
         StringTableEntry interned = StringTable->insert(text.c_str());

         Token tok;
         tok.line = line;
         tok.column = column;

         // "true"/"false" lex as IntConst - CMDscan.l has no boolean
         // literal token.
         if (text.equal("true"))
         {
            tok.kind = TokenKind::IntConst;
            tok.intValue = 1;
            return tok;
         }
         if (text.equal("false"))
         {
            tok.kind = TokenKind::IntConst;
            tok.intValue = 0;
            return tok;
         }

         // Reserved words take priority over {ID} - checked before
         // TypeNameLookup, matching CMDscan.l's rule ordering.
         TokenKind kw = keywordKindOrIdent(interned);
         if (kw != TokenKind::Ident)
         {
            tok.kind = kw;
            return tok;
         }

         // Sc_ScanIdent's live registry check: a registered engine type
         // name lexes as TypeIdent, not Ident.
         if (mTypeNameLookup && mTypeNameLookup(interned))
         {
            tok.kind = TokenKind::TypeIdent;
            tok.text = interned;
            return tok;
         }

         tok.kind = TokenKind::Ident;
         tok.text = interned;
         return tok;
      }

      Token Lexer::scanVar()
      {
         U32 line = mLine, column = mColumn;
         const char* start = mPos;

         advance(); // sigil ($ or %)
         advance(); // mandatory LETTER after the sigil - next() already verified this

         // VARTAIL = {VARMID}*{IDTAIL} - VARMID includes ':', IDTAIL
         // doesn't, so a trailing colon run isn't consumed unless
         // followed by more identifier chars. Scan greedily allowing
         // ':', then trim back if we ended on bare colons.
         while (!isAtEnd() && (isalnum((unsigned char)peek()) || peek() == '_' || peek() == ':'))
            advance();
         while (mPos > start + 2 && peek(-1) == ':')
         {
            --mPos;
            --mColumn;
         }

         String text(start, static_cast<String::SizeType>(mPos - start));
         Token tok;
         tok.kind = TokenKind::Var;
         tok.line = line;
         tok.column = column;
         tok.text = StringTable->insert(text.c_str());
         return tok;
      }

      Token Lexer::scanNumber()
      {
         U32 line = mLine, column = mColumn;
         const char* start = mPos;

         // Hex: 0[xX]{HEXDIGIT}+
         if (peek() == '0' && (peek(1) == 'x' || peek(1) == 'X'))
         {
            advance(); advance();
            const char* hexStart = mPos;
            while (!isAtEnd() && isxdigit((unsigned char)peek()))
               advance();
            if (mPos == hexStart)
            {
               reportError(line, column, "expected hex digits after 0x");
            }
            String text(start, static_cast<String::SizeType>(mPos - start));
            Token tok;
            tok.kind = TokenKind::IntConst;
            tok.line = line;
            tok.column = column;
            S32 val = 0;
            dSscanf(text.c_str(), "%x", &val);
            tok.intValue = val;
            return tok;
         }

         // Leading digit run (may be empty, for the ".5" float shape).
         while (!isAtEnd() && isdigit((unsigned char)peek()))
            advance();

         bool isFloat = false;

         // FLOAT alt 1: {INTEGER}?\.{INTEGER} - dot must be followed by
         // a digit, or "5." with nothing after doesn't match this form.
         if (peek() == '.' && isdigit((unsigned char)peek(1)))
         {
            isFloat = true;
            advance(); // '.'
            while (!isAtEnd() && isdigit((unsigned char)peek()))
               advance();
         }

         // FLOAT alt 2: {INTEGER}(\.{INTEGER})?[eE][+-]?{INTEGER} -
         // exponent mandatory. Still fires even after alt 1's dot was
         // consumed, since "1.5e5" is alt 2 with an optional dot inside.
         if ((peek() == 'e' || peek() == 'E') &&
            (isdigit((unsigned char)peek(1)) ||
               ((peek(1) == '+' || peek(1) == '-') && isdigit((unsigned char)peek(2)))))
         {
            isFloat = true;
            advance(); // e/E
            if (peek() == '+' || peek() == '-')
               advance();
            while (!isAtEnd() && isdigit((unsigned char)peek()))
               advance();
         }

         String text(start, static_cast<String::SizeType>(mPos - start));
         Token tok;
         tok.line = line;
         tok.column = column;

         if (isFloat)
         {
            tok.kind = TokenKind::FloatConst;
            tok.floatValue = dAtof(text.c_str());
         }
         else
         {
            tok.kind = TokenKind::IntConst;
            tok.intValue = dAtoi(text.c_str());
         }
         return tok;
      }

      bool Lexer::collapseEscapes(const char* rawStart, const char* rawEnd, String& out)
      {
         // Mirrors collapseEscape (CMDscan.l): \x + 2 hex digits, \c +
         // digit/r/p/o with a remap table, otherwise \X -> charConv(X).
         //
         // NOTE: previously wrote into a fixed 4096-byte stack buffer and
         // silently stopped (no error, no truncation flag) once full - a
         // string literal resolving to more than ~4095 bytes was cut off
         // with no diagnostic. Builds the result in a Vector<char> instead,
         // so there's no length ceiling at all.
         static const U8 kCollapseRemap[10] = { 0x1,0x2,0x3,0x4,0x5,0x6,0x7,0xb,0xc,0xe };

         Vector<char> buffer;
         buffer.reserve(static_cast<U32>(rawEnd - rawStart));
         const char* p = rawStart;

         auto hexDigit = [](char c) -> S32
         {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
         };

         while (p < rawEnd)
         {
            if (*p == '\\' && p + 1 < rawEnd)
            {
               char next = p[1];
               if (next == 'x' && p + 3 < rawEnd)
               {
                  S32 d1 = hexDigit(p[2]);
                  S32 d2 = hexDigit(p[3]);
                  if (d1 < 0 || d2 < 0)
                     return false;
                  buffer.push_back(static_cast<char>(d1 * 16 + d2));
                  p += 4;
               }
               else if (next == 'c' && p + 2 < rawEnd)
               {
                  char code = p[2];
                  U8 outByte;
                  if (code == 'r') outByte = 15;
                  else if (code == 'p') outByte = 16;
                  else if (code == 'o') outByte = 17;
                  else if (code >= '0' && code <= '9') outByte = kCollapseRemap[code - '0'];
                  else return false;

                  // collapseEscape's own guard: if this would be the
                  // first output byte and equals 0x1, emit 0x2 then 0x1
                  // instead of a bare 0x1.
                  if (outByte == 0x1 && buffer.empty())
                  {
                     buffer.push_back(static_cast<char>(0x2));
                     buffer.push_back(static_cast<char>(0x1));
                  }
                  else
                  {
                     buffer.push_back(static_cast<char>(outByte));
                  }
                  p += 3;
               }
               else
               {
                  buffer.push_back(static_cast<char>(charConvEscape(next)));
                  p += 2;
               }
            }
            else
            {
               buffer.push_back(*p);
               ++p;
            }
         }

         out = buffer.empty() ? String("") : String(buffer.address(), static_cast<String::SizeType>(buffer.size()));
         return true;
      }

      Token Lexer::scanString(char quoteChar)
      {
         U32 line = mLine, column = mColumn;
         advance(); // opening quote

         const char* contentStart = mPos;
         while (!isAtEnd() && peek() != quoteChar && peek() != '\n' && peek() != '\r')
         {
            if (peek() == '\\' && !isAtEnd())
               advance(); // skip escaped char so an escaped quote doesn't end the literal early
            advance();
         }

         if (isAtEnd() || peek() != quoteChar)
         {
            reportError(line, column, "unterminated string literal");
            Token tok;
            tok.kind = TokenKind::Illegal;
            tok.line = line;
            tok.column = column;
            return tok;
         }

         const char* contentEnd = mPos;
         advance(); // closing quote

         String resolved;
         if (!collapseEscapes(contentStart, contentEnd, resolved))
            reportError(line, column, "malformed \\x or \\c escape sequence in string literal");

         Token tok;
         tok.kind = (quoteChar == '"') ? TokenKind::StrAtom : TokenKind::TagAtom;
         tok.line = line;
         tok.column = column;
         tok.text = StringTable->insert(resolved.c_str());
         return tok;
      }

      Token Lexer::scanDocBlock()
      {
         U32 line = mLine, column = mColumn;
         String text;

         // One or more consecutive "///"-prefixed lines. Strip the
         // leading "///" and any \r, join remainder with newlines.
         for (;;)
         {
            if (!(peek() == '/' && peek(1) == '/' && peek(2) == '/'))
               break;

            advance(); advance(); advance(); // "///"
            while (!isAtEnd() && peek() != '\n' && peek() != '\r')
               text += advance();
            text += '\n';

            while (peek() == '\r' || peek() == '\n')
               advance();
         }

         Token tok;
         tok.kind = TokenKind::DocBlock;
         tok.line = line;
         tok.column = column;
         tok.text = StringTable->insert(text.c_str());
         return tok;
      }

      TokenKind Lexer::keywordKindOrIdent(StringTableEntry text) const
      {
         // Matched exactly against CMDscan.l's reserved-word rules.
         // "namespace"/"class" deliberately NOT here - removed from the
         // grammar, no productions ever consumed them.
         if (text == StringTable->insert("in"))         return TokenKind::KwIn;
         if (text == StringTable->insert("or"))         return TokenKind::KwCaseOr;
         if (text == StringTable->insert("break"))      return TokenKind::KwBreak;
         if (text == StringTable->insert("return"))     return TokenKind::KwReturn;
         if (text == StringTable->insert("else"))       return TokenKind::KwElse;
         if (text == StringTable->insert("assert"))     return TokenKind::KwAssert;
         if (text == StringTable->insert("while"))      return TokenKind::KwWhile;
         if (text == StringTable->insert("do"))         return TokenKind::KwDo;
         if (text == StringTable->insert("if"))         return TokenKind::KwIf;
         if (text == StringTable->insert("foreach$"))   return TokenKind::KwForeachStr;
         if (text == StringTable->insert("foreach"))    return TokenKind::KwForeach;
         if (text == StringTable->insert("for"))        return TokenKind::KwFor;
         if (text == StringTable->insert("continue"))   return TokenKind::KwContinue;
         if (text == StringTable->insert("function"))   return TokenKind::KwDefine;
         if (text == StringTable->insert("new"))        return TokenKind::KwDeclare;
         if (text == StringTable->insert("singleton"))  return TokenKind::KwDeclareSingleton;
         if (text == StringTable->insert("datablock"))  return TokenKind::KwDatablock;
         if (text == StringTable->insert("case"))       return TokenKind::KwCase;
         if (text == StringTable->insert("switch$"))    return TokenKind::KwSwitchStr;
         if (text == StringTable->insert("switch"))     return TokenKind::KwSwitch;
         if (text == StringTable->insert("default"))    return TokenKind::KwDefault;
         if (text == StringTable->insert("package"))    return TokenKind::KwPackage;
         return TokenKind::Ident;
      }

      Token Lexer::scanOperatorOrPunct()
      {
         U32 line = mLine, column = mColumn;
         char c = advance();

         auto make = [&](TokenKind kind) -> Token
         {
            Token tok;
            tok.kind = kind;
            tok.line = line;
            tok.column = column;
            return tok;
         };
         auto makeAt = [&](S64 separatorChar) -> Token
         {
            Token tok;
            tok.kind = TokenKind::At;
            tok.line = line;
            tok.column = column;
            tok.intValue = separatorChar;
            return tok;
         };

         switch (c)
         {
         case '=': return match('=') ? make(TokenKind::Eq) : make(TokenKind::Assign);
         case '!':
            if (match('='))     return make(TokenKind::Ne);
            if (peek() == '$' && peek(1) == '=') { advance(); advance(); return make(TokenKind::StrNe); }
            return make(TokenKind::Bang);
         case '>':
            if (match('='))     return make(TokenKind::Ge);
            if (match('>')) return match('=') ? make(TokenKind::ShrAssign) : make(TokenKind::Shr);
            return make(TokenKind::Greater);
         case '<':
            if (match('='))     return make(TokenKind::Le);
            if (match('<')) return match('=') ? make(TokenKind::ShlAssign) : make(TokenKind::Shl);
            return make(TokenKind::Less);
         case '&':
            if (match('&'))     return make(TokenKind::AndAnd);
            if (match('='))     return make(TokenKind::AndAssign);
            return make(TokenKind::Amp);
         case '|':
            if (match('|'))     return make(TokenKind::OrOr);
            if (match('='))     return make(TokenKind::OrAssign);
            return make(TokenKind::Pipe);
         case ':':
            if (match(':'))     return make(TokenKind::ColonColon);
            return make(TokenKind::Colon);
         case '-':
            // Order matters: "-->" checked before "--", or match('-')
            // would greedily consume the second '-' first.
            if (peek() == '-' && peek(1) == '>') { advance(); advance(); return make(TokenKind::IntNameR); }
            if (match('-'))     return make(TokenKind::MinusMinus);
            if (match('='))     return make(TokenKind::MinusAssign);
            if (match('>'))     return make(TokenKind::IntName);
            return make(TokenKind::Minus);
         case '+':
            if (match('+'))     return make(TokenKind::PlusPlus);
            if (match('='))     return make(TokenKind::PlusAssign);
            return make(TokenKind::Plus);
         case '*': return match('=') ? make(TokenKind::MulAssign) : make(TokenKind::Star);
         case '/': return match('=') ? make(TokenKind::DivAssign) : make(TokenKind::Slash);
         case '%': return match('=') ? make(TokenKind::ModAssign) : make(TokenKind::Percent);
         case '^': return match('=') ? make(TokenKind::XorAssign) : make(TokenKind::Caret);
         case '$':
            if (match('='))     return make(TokenKind::StrEq);
            // A bare '$' not followed by a LETTER has no production in
            // CMDscan.l - treated as Illegal, not silently accepted.
            return make(TokenKind::Illegal);
         case '@': return makeAt(0);
         case '.': return make(TokenKind::Dot);
         case '(': return make(TokenKind::LParen);
         case ')': return make(TokenKind::RParen);
         case '[': return make(TokenKind::LBracket);
         case ']': return make(TokenKind::RBracket);
         case '{': return make(TokenKind::LBrace);
         case '}': return make(TokenKind::RBrace);
         case ',': return make(TokenKind::Comma);
         case ';': return make(TokenKind::Semicolon);
         case '~': return make(TokenKind::Tilde);
         case '?': return make(TokenKind::Question);
         default:
            reportError(line, column, "unrecognized character");
            return make(TokenKind::Illegal);
         }
      }

      Token Lexer::next()
      {
         // "///" doc-blocks must be checked before the generic comment
         // skip, since skipWhitespaceAndComments treats "///" as
         // ordinary text - matches CMDscan.l's rule ordering.
         skipWhitespaceAndComments();
         if (peek() == '/' && peek(1) == '/' && peek(2) == '/')
            return scanDocBlock();

         if (isAtEnd())
         {
            Token tok;
            tok.kind = TokenKind::Eof;
            tok.line = mLine;
            tok.column = mColumn;
            return tok;
         }

         char c = peek();

         if (c == '"')
            return scanString('"');
         if (c == '\'')
            return scanString('\'');

         if ((c == '$' || c == '%') && (isalpha((unsigned char)peek(1)) || peek(1) == '_'))
            return scanVar();

         if (isalpha((unsigned char)c) || c == '_')
         {
            // "NL"/"TAB"/"SPC" would lex as plain identifiers by {ID}
            // shape, but CMDscan.l gives them dedicated rules ahead of
            // {ID} that all `return '@'` with a separator payload -
            // reproduce that priority before the general identifier path.
            if (dStrncmp(mPos, "NL", 2) == 0 && !(isalnum((unsigned char)peek(2)) || peek(2) == '_'))
            {
               U32 line = mLine, column = mColumn;
               advance(); advance();
               Token tok; tok.kind = TokenKind::At; tok.line = line; tok.column = column; tok.intValue = '\n';
               return tok;
            }
            if (dStrncmp(mPos, "TAB", 3) == 0 && !(isalnum((unsigned char)peek(3)) || peek(3) == '_'))
            {
               U32 line = mLine, column = mColumn;
               advance(); advance(); advance();
               Token tok; tok.kind = TokenKind::At; tok.line = line; tok.column = column; tok.intValue = '\t';
               return tok;
            }
            if (dStrncmp(mPos, "SPC", 3) == 0 && !(isalnum((unsigned char)peek(3)) || peek(3) == '_'))
            {
               U32 line = mLine, column = mColumn;
               advance(); advance(); advance();
               Token tok; tok.kind = TokenKind::At; tok.line = line; tok.column = column; tok.intValue = ' ';
               return tok;
            }
            return scanIdentifierOrKeyword();
         }

         if (isdigit((unsigned char)c) || (c == '.' && isdigit((unsigned char)peek(1))))
            return scanNumber();

         return scanOperatorOrPunct();
      }

   } // namespace ts2
} // namespace newConsole
