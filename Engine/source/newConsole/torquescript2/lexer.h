#ifndef _NEWCONSOLE_TS2_LEXER_H_
#define _NEWCONSOLE_TS2_LEXER_H_

#ifndef _NEWCONSOLE_TS2_TOKENKINDS_H_
#include "newConsole/torquescript2/tokenKinds.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _TORQUE_STRING_H_
#include "core/util/str.h"
#endif

#include <functional>

namespace newConsole
{
   namespace ts2
   {

      /// One lex-time diagnostic - malformed escape, unterminated string, etc.
      struct LexDiagnostic
      {
         U32 line = 0;
         U32 column = 0;
         String message;
      };

      /// Decides whether a matched identifier is a registered engine type
      /// name (-> TypeIdent) or a plain identifier (-> Ident).
      ///
      /// @note Mirrors CMDscan.l's Sc_ScanIdent: a live registry lookup at
      ///   lex time, not a fixed keyword list. Injected rather than wired to
      ///   HostBindingRegistry directly so the lexer has no reflection-layer
      ///   dependency - a caller with no registry (a syntax checker, a
      ///   formatter) can pass a callback that always returns false.
      using TypeNameLookup = std::function<bool(StringTableEntry name)>;

      /// Hand-written scanner over TorqueScript2 source text. No generated
      /// table, no Flex - reproduces CMDscan.l's behavior directly (see
      /// tokenKinds.inc for the two places this port diverges from the old grammar).
      class Lexer
      {
      public:
         /// @param source Must outlive the Lexer - tokens' text fields point
         ///   into StringTable-interned copies, but scanning itself reads
         ///   directly from this buffer without copying it.
         /// @param originName Diagnostics only, not used in any lexing decision.
         Lexer(const char* source, const char* originName, TypeNameLookup typeNameLookup);

         /// Scans and returns the next token, advancing position. Returns
         /// TokenKind::Eof at end of input and keeps returning Eof afterward
         /// rather than reading past the buffer.
         Token next();

         /// Diagnostics accumulated so far. Not cleared between next() calls -
         /// check size() before/after a call for per-token diagnostics.
         const Vector<LexDiagnostic>& diagnostics() const { return mDiagnostics; }
         bool hasErrors() const { return !mDiagnostics.empty(); }

      private:
         void reportError(U32 line, U32 column, const char* message);

         // Character-level helpers. Operate on mPos/mLine/mColumn, never read past mEnd.
         char peek(S32 offset = 0) const;
         char advance();
         bool match(char expected);
         bool isAtEnd() const;

         void skipWhitespaceAndComments();

         Token scanIdentifierOrKeyword();
         Token scanVar();
         Token scanNumber();
         Token scanString(char quoteChar);
         Token scanDocBlock();
         Token scanOperatorOrPunct();

         /// Applies collapseEscape's rules (CMDscan.l) to the raw text between
         /// the quote delimiters, writing resolved bytes into @a out.
         /// @return false on a malformed \x or \c escape.
         bool collapseEscapes(const char* rawStart, const char* rawEnd, String& out);

         /// @return the TokenKind for @a text if a reserved word, else
         ///   TokenKind::Ident. Checked before TypeNameLookup, matching
         ///   CMDscan.l's rule ordering (reserved words take priority over {ID}).
         TokenKind keywordKindOrIdent(StringTableEntry text) const;

         const char* mSource;
         const char* mPos;
         const char* mEnd;
         StringTableEntry mOriginName;
         TypeNameLookup mTypeNameLookup;

         U32 mLine = 1;
         U32 mColumn = 1;

         Vector<LexDiagnostic> mDiagnostics;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_LEXER_H_
