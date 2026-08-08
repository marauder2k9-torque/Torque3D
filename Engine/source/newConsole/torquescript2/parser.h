#ifndef _NEWCONSOLE_TS2_PARSER_H_
#define _NEWCONSOLE_TS2_PARSER_H_

#ifndef _NEWCONSOLE_TS2_LEXER_H_
#include "newConsole/torquescript2/lexer.h"
#endif
#ifndef _NEWCONSOLE_TS2_COMPILATIONUNIT_H_
#include "newConsole/torquescript2/compilationUnit.h"
#endif

#include <initializer_list>

namespace newConsole
{
namespace ts2
{

/// One parse-time diagnostic. Distinct from LexDiagnostic (lexer.h) -
/// kept separate so a caller can tell "the lexer choked on this text"
/// apart from "the tokens were fine but didn't form valid syntax", which
/// point at different kinds of bugs (a bad escape sequence vs. a missing
/// semicolon) and often want different presentation.
struct ParseDiagnostic
{
   U32 line = 0;
   U32 column = 0;
   String message;
};

/// Recursive-descent parser for statements, Pratt (precedence-climbing)
/// for expressions. Consumes a Lexer's token stream, builds a
/// CompilationUnit.
///
/// @note Error recovery is panic-mode at statement granularity: on a
///   parse error inside one statement, the parser reports it and skips
///   forward to the next statement boundary (';' or a block delimiter)
///   rather than aborting the whole file - one syntax error should not
///   prevent every other diagnostic in the file from being reported.
class Parser
{
public:
   Parser(Lexer& lexer, StringTableEntry originName);

   /// Parses the whole token stream into a CompilationUnit. Always
   /// returns a CompilationUnit, even on error - check hasErrors() to
   /// know whether the result should be trusted for compilation, since a
   /// unit produced after error recovery may have gaps where a
   /// mis-parsed statement was skipped rather than represented.
   CompilationUnit parse();

   const Vector<ParseDiagnostic>& diagnostics() const { return mDiagnostics; }
   bool hasErrors() const { return !mDiagnostics.empty(); }

private:
   // ---- token stream management ----
   void advanceToken();
   bool check(TokenKind kind) const;
   bool checkAny(std::initializer_list<TokenKind> kinds) const;
   bool matchToken(TokenKind kind);          // consumes and returns true if check() is true
   const Token& expect(TokenKind kind, const char* what); // reports an error if check() is false; always advances
   void reportError(const Token& at, const char* message);
   void synchronizeToStatementBoundary();

   ast::SourceSpan spanOf(const Token& t) const;

   // ---- statements ----
   ast::StmtHandle parseStatement();
   ast::StmtHandle parseBlockStatement();      // { statement* }
   ast::ListHandle parseStatementList(TokenKind terminator); // reads statements until terminator, does not consume it
   ast::StmtHandle parseIfStatement();
   ast::StmtHandle parseWhileStatement();
   ast::StmtHandle parseDoWhileStatement();
   ast::StmtHandle parseForStatement();
   ast::StmtHandle parseForeachStatement(bool isStringForm);
   ast::StmtHandle parseSwitchStatement(bool isStringForm);
   ast::StmtHandle parseBreakStatement();
   ast::StmtHandle parseContinueStatement();
   ast::StmtHandle parseReturnStatement();
   ast::StmtHandle parseFunctionDeclStatement();
   ast::StmtHandle parsePackageDeclStatement();
   ast::StmtHandle parseExprStatement();

   ast::Param parseParam();

   // ---- expressions (Pratt) ----
   ast::ExprHandle parseExpr(S32 minPrecedence = 0);
   ast::ExprHandle parsePrefix();               // literals, unary ops, ( expr ), prefix ++/--, new/datablock/singleton
   ast::ExprHandle parseInfix(ast::ExprHandle lhs, S32 minPrecedence);
   ast::ExprHandle parsePostfixChain(ast::ExprHandle base); // .field / ->field / [index] / (args) / postfix ++/--
   ast::ExprHandle parseObjectDecl(bool isDatablock, bool isSingleton);
   ast::ListHandle parseExprList(TokenKind terminator); // comma-separated, stops before terminator

   /// Binding power for the token currently at mCurrent, or -1 if it is
   /// not an infix/postfix operator at all. Encodes exactly the
   /// %left/%right table confirmed against CMDgram.y - see parser.cpp's
   /// definition for the actual precedence numbers and the comment
   /// explaining the table's provenance.
   S32 infixPrecedence(TokenKind kind) const;
   bool isRightAssociative(TokenKind kind) const;

   Lexer& mLexer;
   StringTableEntry mOriginName;
   Token mCurrent;
   Token mPrevious;

   CompilationUnit mUnit;
   Vector<ParseDiagnostic> mDiagnostics;
};

} // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_PARSER_H_
