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

/// Callback used to decide whether a matched identifier is a registered
/// engine type name (-> TypeIdent) or a plain identifier (-> Ident).
///
/// @note Mirrors the original scanner's behavior exactly: Sc_ScanIdent
///   (CMDscan.l) checks ConsoleBaseType::getTypeByName() before deciding
///   between TYPEIDENT and IDENT - this is a live registry lookup at lex
///   time, not a fixed keyword list. Injected rather than hard-wired to
///   HostBindingRegistry so the lexer itself has no dependency on the
///   reflection layer - a caller with no registry at all (a syntax
///   checker, a formatter) can pass a callback that always returns false
///   and get plain-Ident behavior for every identifier.
using TypeNameLookup = std::function<bool(StringTableEntry name)>;

/// Hand-written scanner over TorqueScript2 source text. No generated
/// table, no Flex - a straightforward character-at-a-time reader that
/// reproduces CMDscan.l's confirmed behavior (see tokenKinds.inc's
/// header comments for the two places this port deliberately diverges
/// from the old grammar's token declarations).
class Lexer
{
public:
   /// @param source Must outlive the Lexer - tokens' text fields point
   ///   into StringTable-interned copies, but the lexer itself scans
   ///   directly over this buffer without copying it.
   /// @param originName Used only in diagnostics (file name for error
   ///   messages), not for any lexing decision.
   Lexer(const char* source, const char* originName, TypeNameLookup typeNameLookup);

   /// Scans and returns the next token, advancing internal position.
   /// Returns a Token with kind == TokenKind::Eof at end of input, and
   /// keeps returning Eof on every subsequent call rather than reading
   /// past the buffer.
   Token next();

   /// Diagnostics accumulated so far. Not cleared between next() calls -
   /// a caller wanting per-token diagnostics should check size() before
   /// and after a next() call.
   const Vector<LexDiagnostic>& diagnostics() const { return mDiagnostics; }
   bool hasErrors() const { return !mDiagnostics.empty(); }

private:
   void reportError(U32 line, U32 column, const char* message);

   // Character-level helpers. All operate on mPos/mLine/mColumn and
   // never read past mEnd.
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

   /// Applies collapseEscape's rules (see CMDscan.l) to the raw text
   /// between quote1+1 and quote2-1 (exclusive of the delimiters),
   /// writing the resolved bytes into @a out. @return false on a
   /// malformed \x or \c escape, matching collapseEscape's own failure
   /// contract.
   bool collapseEscapes(const char* rawStart, const char* rawEnd, String& out);

   /// @return the TokenKind for @a text if it is a reserved word,
   ///   otherwise TokenKind::Ident. Checked before the TypeNameLookup
   ///   callback, matching CMDscan.l's rule ordering (reserved words are
   ///   listed ahead of {ID} and take priority).
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
