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

#include "../Diagnostics.h"
#include "../korkArena.h"
#include "../lexer/Lexer.h"
#include "../ast/nodes.h"

namespace KorkScript
{
   //---------------------------------------------------------------------------
   // StrictMode usually set by a #pragma strict (file-level)
   //---------------------------------------------------------------------------
   enum class StrictMode { Off, Warn, Error };

   class ParserContext
   {
   public:
      // ParseContext owns the LexerContext so the arena is always constructed
      // before the lexer — no two-step initialisation needed by callers.
      explicit ParserContext(const char* source, StringTableEntry fileName);

      LexerContext   lexer;      ///< Owned — initialised with mArena reference
      const char*    source;
      StrictMode     strictMode = StrictMode::Off;

      KorkArena mArena;
      template<typename T, typename... Args>
      T* alloc(Args&&... args)
      {
         return mArena.construct<T>(std::forward<Args>(args)...);
      }

      void* allocRaw(U32 size) { return mArena.allocZeroed(size); }
      const char* allocString(const char* str, U32 len)
      {
         return mArena.allocString(str, len);
      }
      const char* allocString(const char* str)
      {
         return mArena.allocString(str);
      }

      //-----------------------------------------------------------------------
      // Diagnostics
      //-----------------------------------------------------------------------
      void error(SourceRange range, const char* fmt, ...);
      void warning(SourceRange range, const char* fmt, ...);
      void note(SourceRange range, const char* fmt, ...);

      bool hasErrors() const;
      const Vector<DiagnosticMessage>& diagnostics() const { return mDiagnostics; }

   private:
      Vector<DiagnosticMessage> mDiagnostics;
   };

   class Parser
   {
   public:
      explicit Parser(ParserContext& ctx);

      StmtNode* parseFile();
      ExprNode* parseExpression();

   private:
      ParserContext& mCtx;

      Token advance();
      Token peek(U32 ahead = 0) const;
      bool  match(TokenKind kind);
      Token expect(TokenKind kind, const char* msg);

      bool  check(TokenKind) const;
      bool  check2(TokenKind a, TokenKind b) const;

      bool  checkConCatOp() const;
      bool  atEnd() const;
      S32   line() const;
      bool  synchronize(std::initializer_list<TokenKind> stop);

      //-----------------------------------------------------------------------
      // #pragma handling
      //-----------------------------------------------------------------------
      void parsePragma();

      //-----------------------------------------------------------------------
      // Top-level declarations
      //-----------------------------------------------------------------------
      Node* parseDecl();
      Node* parsePackageDecl();
      FunctionDeclNode* parseFunctionDecl(StringTableEntry packageName = nullptr);

      //-----------------------------------------------------------------------
      // Statements
      //-----------------------------------------------------------------------
      Node* parseStmt();
      Node* parseStmtBlock();          ///< '{' statement_list '}'  or  single stmt
      Node* parseStatementList();      ///< zero or more stmts
      Node* parseIfStmt();
      Node* parseWhileStmt();
      Node* parseDoWhileStmt();
      Node* parseForStmt();
      Node* parseForeachStmt();
      Node* parseSwitchStmt();
      Node* parseSwitchBody();
      Node* parseDatablockDecl();
      Node* parseReturnStmt();
      Node* parseBreakStmt();
      Node* parseContinueStmt();
      Node* parseTTagSetStmt();        ///< TTAG '=' expr [',' expr] ';'
      Node* parseExpressionStmt();

      //-----------------------------------------------------------------------
      // switch helpers
      //-----------------------------------------------------------------------
      IfNode* parseCaseBlock(bool isString, Node* switchExpr);

      //-----------------------------------------------------------------------
      // Function parameter list
      //-----------------------------------------------------------------------
      VarNode* parseParamList();          ///< '(' var_list_decl ')' — returns linked list
      VarNode* parseParam();              ///< single parameter with optional default

      //-----------------------------------------------------------------------
      // Object / datablock helpers
      //-----------------------------------------------------------------------
      /// Parse the parent specifier ':' IDENT  (optional).
      StringTableEntry parseParentBlock();

      /// Parse optional constructor args ',' expr_list  (optional).
      Node* parseObjectArgs();

      /// Parse the name expression inside new/singleton/datablock.
      Node* parseObjectName();

      /// Parse the class name — IDENT or '(' expr ')'.
      Node* parseClassNameExpr();

      ObjectDeclNode* parseObjectDecl();
      /// Parse zero or more slot assignments.
      SlotAssignNode* parseSlotAssignList();
      SlotAssignNode* parseSlotAssign();     ///< single slot

      /// Parse a single object declaration (new / singleton).
      struct ObjectBlock { SlotAssignNode* slots; ObjectDeclNode* decls; };
      ObjectBlock       parseObjectDeclBlock();

      //-----------------------------------------------------------------------
      // Expressions
      //
      // Precedence levels match the %left/%right table in CMDgram.y (low→high):
      //
      //  1   '[' (array subscript — handled as postfix in parsePrimary)
      //  2   assignment operators  = += -= *= /= %= &= ^= |= <<= >>=
      //  3   ternary  ?:
      //  4   ||
      //  5   &&
      //  6   |
      //  7   ^
      //  8   &
      //  9   == !=
      // 10   < <= > >=
      // 11   @ SPC TAB NL  $=  !$=   (string ops)
      // 12   << >>
      // 13   + -
      // 14   * / %
      // 15   unary  ! ~ ++ --  (prefix)
      // 16   .  (member access — handled as postfix)
      // 17   -> -->  (internal slot)
      //-----------------------------------------------------------------------
      Node* parseExpr(U32 minPrec = 0);
      Node* parseUnary();
      Node* parsePrimary();
      Node* parsePostfix(Node* lhs);   ///< '.', '[', '->', '-->', '()'
      /// Parse a comma-separated argument list (may be empty). expr_list_decl.
      Node* parseArgList();
      /// Parse an array index expression: aidx_expr (may be comma-separated).
      Node* parseAidxExpr();

      // assign_op_struct equivalent — returns the operator token and rhs expr.
      struct AssignOp { S32 op; ExprNode* expr; S32 line; };

      //-----------------------------------------------------------------------
      // Precedence helpers
      //-----------------------------------------------------------------------
      struct InfixRule
      {
         TokenKind   kind;
         U32         prec;        ///< left-binding power
         bool        rightAssoc;  ///< true for assignment ops
      };
      static const InfixRule* getInfixRule(TokenKind kind);

      inline S32 ln() const { return peek().line(); }  ///< convenience: current line

   };

}
