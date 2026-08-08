// X-macro token list. #define NC_TOKEN(name) before including this file to
// generate the TokenKind enum, a name table, or anything else keyed on
// the token set - see tokenKinds.h for the enum itself.
//
// Grouped and named to mirror CMDgram.y's %token declarations directly,
// so a reader comparing this list against the grammar file can match
// entries one-to-one rather than needing to guess at a renamed token.
//
// Resolved: CMDgram.y separately declares TTAG and TAGATOM, but they
// describe the same lexical thing - a single-quoted 'tagged string'
// literal, whose purpose is the networked tagged-string system (a
// tagged string's text is transmitted once and referenced afterward by
// a small integer id - see NetStringTable/NetStringHandle). TTAG appears
// to be dead/superseded grammar rather than a second distinct literal
// form; this list keeps one token, TagAtom, for '...' literals. What the
// old grammar handled as two token kinds becomes, here, one token kind
// plus a semantic property the compiler acts on (recognizing a TagAtom
// and emitting whatever registers it with the tagged-string system) -
// not a second token kind to keep the lexer/parser in sync with.

// End of input / error
NC_TOKEN(Eof)
NC_TOKEN(Illegal)

// Reserved words
NC_TOKEN(KwDefine)
NC_TOKEN(KwEndDef)
NC_TOKEN(KwDeclare)
NC_TOKEN(KwDeclareSingleton)
NC_TOKEN(KwBreak)
NC_TOKEN(KwElse)
NC_TOKEN(KwContinue)
NC_TOKEN(KwGlobal)
NC_TOKEN(KwIf)
NC_TOKEN(KwNil)
NC_TOKEN(KwReturn)
NC_TOKEN(KwWhile)
NC_TOKEN(KwDo)
NC_TOKEN(KwEndIf)
NC_TOKEN(KwEndWhile)
NC_TOKEN(KwEndFor)
NC_TOKEN(KwDefault)
NC_TOKEN(KwFor)
NC_TOKEN(KwForeach)
NC_TOKEN(KwForeachStr)
NC_TOKEN(KwIn)
NC_TOKEN(KwDatablock)
NC_TOKEN(KwSwitch)
NC_TOKEN(KwCase)
NC_TOKEN(KwSwitchStr)
NC_TOKEN(KwCaseOr)
NC_TOKEN(KwPackage)
NC_TOKEN(KwAssert)

// Literals / identifiers
NC_TOKEN(CharConst)
NC_TOKEN(IntConst)
NC_TOKEN(FloatConst)
// One token for both sigils - CMDscan.l's Sc_ScanVar production matches
// [$%]{LETTER}{VARTAIL}* as a single rule and always returns VAR, with
// the sigil character ($ = global, % = local) kept as the first
// character of the interned text rather than split into a separate
// token kind at lex time.
NC_TOKEN(Var)          // VAR - both $Global::Foo and %localVar; sigil is text[0]
NC_TOKEN(Ident)        // IDENT - bareword / function / namespace name
NC_TOKEN(TypeIdent)    // TYPEIDENT - datablock/class type name
NC_TOKEN(DocBlock)
NC_TOKEN(StrAtom)      // STRATOM  - "quoted string"
NC_TOKEN(TagAtom)      // 'tagged string' - see header comment above re: TTAG/TAGATOM

// Punctuation / single-char operators (also used bare in the grammar)
NC_TOKEN(Plus) NC_TOKEN(Minus) NC_TOKEN(Star) NC_TOKEN(Slash)
NC_TOKEN(Less) NC_TOKEN(Greater) NC_TOKEN(Assign) NC_TOKEN(Dot)
NC_TOKEN(Pipe) NC_TOKEN(Amp) NC_TOKEN(Percent)
NC_TOKEN(LParen) NC_TOKEN(RParen) NC_TOKEN(Comma) NC_TOKEN(Colon)
NC_TOKEN(Semicolon) NC_TOKEN(LBrace) NC_TOKEN(RBrace)
NC_TOKEN(Caret) NC_TOKEN(Tilde) NC_TOKEN(Bang) NC_TOKEN(At)
NC_TOKEN(LBracket) NC_TOKEN(RBracket)
NC_TOKEN(Question)

// Multi-char operators
NC_TOKEN(IntName)        // opINTNAME  - -> (internal-name access)
NC_TOKEN(IntNameR)       // opINTNAMER - ~> (recursive internal-name access)
NC_TOKEN(MinusMinus)
NC_TOKEN(PlusPlus)
NC_TOKEN(Shl) NC_TOKEN(Shr)
NC_TOKEN(PlusAssign) NC_TOKEN(MinusAssign) NC_TOKEN(MulAssign)
NC_TOKEN(DivAssign) NC_TOKEN(ModAssign) NC_TOKEN(AndAssign)
NC_TOKEN(XorAssign) NC_TOKEN(OrAssign) NC_TOKEN(ShlAssign) NC_TOKEN(ShrAssign)
// Note: CMDgram.y declares opCAT as a token but, like TTAG, no lexer
// production for it exists in the scanner available for this port - the
// only string-concat lexeme the scanner actually emits is the unified
// @/NL/TAB/SPC rule (see the concat sigil note below). Treated the same
// way as TTAG: not carried forward as a distinct token kind here.
NC_TOKEN(Eq) NC_TOKEN(Ne) NC_TOKEN(Ge) NC_TOKEN(Le)
NC_TOKEN(AndAnd) NC_TOKEN(OrOr)
NC_TOKEN(StrEq) NC_TOKEN(StrNe)
NC_TOKEN(ColonColon)

// Concat sigils - NL/TAB/SPC/@ all lex as ONE token in the real scanner
// (CMDscan.l: "NL"/"TAB"/"SPC"/"@" rules all `return '@'`), distinguished
// only by a separator-char payload carried on the token (0 for plain @,
// '\n'/'\t'/' ' for NL/TAB/SPC respectively) - not four separate token
// kinds. See Token::intValue in tokenKinds.h, which carries that
// separator char for this token. At (below, in the punctuation block)
// covers this same lexeme; no separate Concat/ConcatNL/ConcatTAB/
// ConcatSPC kinds exist.

// Internal-slot access chain continuation (a->b->c) reuses IntName/
// IntNameR above per-segment; no separate token needed.
