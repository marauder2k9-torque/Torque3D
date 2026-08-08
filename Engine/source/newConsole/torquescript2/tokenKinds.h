#ifndef _NEWCONSOLE_TS2_TOKENKINDS_H_
#define _NEWCONSOLE_TS2_TOKENKINDS_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      enum class TokenKind : U16
      {
#define NC_TOKEN(name) name,
#include "newConsole/torquescript2/tokenKinds.inc"
#undef NC_TOKEN
      };

      /// @return a human-readable name for @a kind, for diagnostics only -
      ///   never used for lexing/parsing decisions, so it doesn't need to be
      ///   fast.
      const char* tokenKindName(TokenKind kind);

      /// One lexed token. Identifiers/keywords intern into StringTable at lex
      /// time (see lexer.cpp), not at AST-build time - matches the original
      /// TorqueScript lexer's behavior, which the design doc called out as
      /// worth keeping.
      struct Token
      {
         TokenKind kind = TokenKind::Eof;
         U32 line = 1;
         U32 column = 1;

         // At most one of these is meaningful, selected by kind:
         StringTableEntry text = nullptr;  // Ident/Var/Tag/TypeIdent/StrAtom/TagAtom/DocBlock
         S64 intValue = 0;                 // IntConst/CharConst; also At's separator char (0/'\n'/'\t'/' ' for @/NL/TAB/SPC)
         F64 floatValue = 0.0;             // FloatConst

         bool is(TokenKind k) const { return kind == k; }
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_TOKENKINDS_H_
