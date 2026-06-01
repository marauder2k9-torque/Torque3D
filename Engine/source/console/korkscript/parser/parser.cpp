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

#include "parser.h"
#include "core/stringTable.h"
#include "core/strings/stringFunctions.h"

namespace KorkScript
{

   //-----------------------------------------------------------------------
   // ParserContext
   //-----------------------------------------------------------------------

   ParserContext::ParserContext(const char* source_, StringTableEntry fileName)
      : lexer(source_, fileName, mArena)
      , source(source_)
   {
   }

   void ParserContext::error(SourceRange range, const char* fmt, ...)
   {
      char buf[512];
      va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
      mDiagnostics.push_back({ DiagnosticMessage::Severity::Error, range, buf });
   }

   void ParserContext::warning(SourceRange range, const char* fmt, ...)
   {
      char buf[512];
      va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
      mDiagnostics.push_back({ DiagnosticMessage::Severity::Warning, range, buf });
   }

   void ParserContext::note(SourceRange range, const char* fmt, ...)
   {
      char buf[512];
      va_list a; va_start(a, fmt); vsnprintf(buf, sizeof(buf), fmt, a); va_end(a);
      mDiagnostics.push_back({ DiagnosticMessage::Severity::Note, range, buf });
   }

   bool ParserContext::hasErrors() const
   {
      for (const auto& d : mDiagnostics)
         if (d.isError()) return true;
      return lexer.hasErrors();
   }
   //-----------------------------------------------------------------------
   // ParserContext End
   //-----------------------------------------------------------------------

   //-----------------------------------------------------------------------
   // Parser
   //-----------------------------------------------------------------------

   Parser::Parser(ParserContext& ctx) : mCtx(ctx) {}

   //-----------------------------------------------------------------------
   // Token helpers
   //-----------------------------------------------------------------------
   Token Parser::advance() { return mCtx.lexer.next(); }
   Token Parser::peek(U32 ahead) const { return mCtx.lexer.peek(ahead); }
   bool  Parser::check(TokenKind k) const { return mCtx.lexer.peek(0).kind == k; }
   bool  Parser::check2(TokenKind a, TokenKind b) const
   {
      return peek(0).kind == a && peek(1).kind == b;
   }
   bool  Parser::atEnd() const { return check(TokenKind::Eof); }
   S32   Parser::line()  const { return (S32)peek().line(); }

   bool Parser::match(TokenKind k)
   {
      if (check(k)) { advance(); return true; } return false;
   }

   Token Parser::expect(TokenKind kind, const char* msg)
   {
      if (check(kind)) return advance();
      Token bad = peek();
      mCtx.error(bad.range, "%s", msg);
      Token syn; syn.kind = kind; syn.range = bad.range; return syn;
   }

   bool Parser::checkConCatOp() const
   {
      TokenKind k = peek().kind;
      return k == TokenKind::OpCat || k == TokenKind::OpCatSpc
         || k == TokenKind::OpCatNl || k == TokenKind::OpCatTab;
   }

   bool Parser::synchronize(std::initializer_list<TokenKind> stop)
   {
      while (!atEnd())
      {
         for (TokenKind k : stop) if (check(k)) return true;
         advance();
      }
      return false;
   }

   //-----------------------------------------------------------------------
   // #pragma
   //-----------------------------------------------------------------------
   void Parser::parsePragma()
   {
      Token name = peek();
      if (!name.isIdent())
      {
         mCtx.error(name.range, "Expected pragma name after '#pragma'.");
         synchronize({ TokenKind::Semicolon, TokenKind::RBrace });
         return;
      }
      advance();

      if (dStrcmp(name.strVal, "strict") == 0)
      {
         if (match(TokenKind::LParen))
         {
            Token mode = peek();
            if (mode.isIdent() && dStrcmp(mode.strVal, "warn") == 0)
            {
               advance(); mCtx.strictMode = StrictMode::Warn;
            }
            else if (mode.isIdent() && dStrcmp(mode.strVal, "error") == 0)
            {
               advance(); mCtx.strictMode = StrictMode::Error;
            }
            else
               mCtx.error(mode.range, "Expected 'warn' or 'error'.");
            expect(TokenKind::RParen, "Expected ')'");
         }
         else mCtx.strictMode = StrictMode::Warn;
      }
      else mCtx.warning(name.range, "Unknown pragma '%s' — ignored.", name.strVal);
   }

   //-----------------------------------------------------------------------
   // Top-Level entry
   //-----------------------------------------------------------------------
   Node* Parser::parseFile()
   {
      Node* list = nullptr;
      if (check(TokenKind::KwPragma)) { advance(); parsePragma(); }

      while (!atEnd())
      {
         Node* decl = parseDecl();
         if (!decl) continue;
         nodeAppend(list, decl);
      }
      return list;
   }

   Node* Parser::parseExpression() { return parseExpr(); }

   Node* Parser::parseDecl()
   {
      if (check(TokenKind::KwPackage))  return parsePackageDecl();
      if (check(TokenKind::KwFunction)) return parseFunctionDecl();

      Node* s = parseStmt();
      if (!s)
      {
         synchronize({ TokenKind::Semicolon, TokenKind::RBrace,
                       TokenKind::KwFunction, TokenKind::KwPackage });
         match(TokenKind::Semicolon);
      }
      return s;
   }

   //-----------------------------------------------------------------------
   // Package declaration  —  'package' IDENT '{' fn_list '}' ';'
   //-----------------------------------------------------------------------
   Node* Parser::parsePackageDecl()
   {
      S32 ln = line();
      expect(TokenKind::KwPackage, "Expected 'package'");
      Token nameToken = expect(TokenKind::Ident, "Expected package name");
      StringTableEntry packageName = StringTable->insert(nameToken.strVal);

      expect(TokenKind::LBrace, "Expected '{'");

      Node* list = nullptr;
      while (!check(TokenKind::RBrace) && !atEnd())
      {
         if (!check(TokenKind::KwFunction))
         {
            mCtx.error(peek().range, "Only function declarations are allowed inside a package.");
            synchronize({ TokenKind::KwFunction, TokenKind::RBrace });
            continue;
         }
         // Pass packageName so the FunctionDeclNode has it set directly.
         FunctionDeclNode* fn = parseFunctionDecl(packageName);
         if (fn) nodeAppend(list, fn);
      }

      expect(TokenKind::RBrace, "Expected '}' to close package");
      expect(TokenKind::Semicolon, "Expected ';' after package declaration");
      return list;
   }

   //-----------------------------------------------------------------------
   // Function declaration
   //   'function' [IDENT '::'] IDENT '(' params ')' '{' stmts '}'
   //-----------------------------------------------------------------------
   FunctionDeclNode* Parser::parseFunctionDecl(StringTableEntry package)
   {
      S32 startLine = line();
      expect(TokenKind::KwFunction, "Expected 'function'");

      Token firstName = expect(TokenKind::Ident, "Expected function name");
      StringTableEntry fnName = StringTable->insert(firstName.strVal);
      StringTableEntry nameSpace = nullptr;

      if (match(TokenKind::OpColonColon))
      {
         Token methodName = expect(TokenKind::Ident, "Expected method name after '::'");
         nameSpace = fnName;
         fnName = StringTable->insert(methodName.strVal);
      }

      expect(TokenKind::LParen, "Expected '(' after function name");
      VarNode* params = parseParamList();
      expect(TokenKind::RParen, "Expected ')' after parameters");

      expect(TokenKind::LBrace, "Expected '{' to open function body");
      Node* body = parseStatementList();
      expect(TokenKind::RBrace, "Expected '}' to close function body");

      FunctionDeclNode* node = mCtx.alloc<FunctionDeclNode>(startLine, fnName, nameSpace, params, body);
      node->package = package;   // set here — null for non-package functions
      node->range = { mCtx.lexer.currentLocation(), mCtx.lexer.currentLocation() };
      return node;
   }

   //-----------------------------------------------------------------------
   // Parameter list
   //-----------------------------------------------------------------------
   VarNode* Parser::parseParamList()
   {
      if (check(TokenKind::RParen)) return nullptr;
      VarNode* first = parseParam();
      VarNode* last = first;
      while (match(TokenKind::Comma))
      {
         VarNode* p = parseParam();
         if (p) { last->next = p; last = p; }
      }
      return first;
   }

   VarNode* Parser::parseParam()
   {
      Token varToken = expect(TokenKind::Var, "Expected parameter variable");
      StringTableEntry varName = StringTable->insert(varToken.strVal);
      match(TokenKind::Question); // optional marker — captured by defaultVal presence
      Node* defaultVal = nullptr;
      if (match(TokenKind::Equals))
         defaultVal = parseExpr();
      VarNode* n = mCtx.alloc<VarNode>((S32)varToken.line(), varName, nullptr,
         static_cast<ExprNode*>(defaultVal));
      n->range = varToken.range;
      return n;
   }

   //-----------------------------------------------------------------------
   // Statement list / block
   //-----------------------------------------------------------------------
   Node* Parser::parseStatementList()
   {
      Node* list = nullptr;
      while (!check(TokenKind::RBrace) && !atEnd())
      {
         Node* s = parseStmt();
         if (!s)
         {
            if (!synchronize({ TokenKind::Semicolon, TokenKind::RBrace })) break;
            match(TokenKind::Semicolon);
            continue;
         }
         nodeAppend(list, s);
      }
      return list;
   }

   Node* Parser::parseStmtBlock()
   {
      if (check(TokenKind::LBrace))
      {
         advance();
         Node* list = parseStatementList();
         expect(TokenKind::RBrace, "Expected '}'");
         return list;
      }
      return parseStmt();
   }

   //-----------------------------------------------------------------------
   // Statement dispatch
   //-----------------------------------------------------------------------
   Node* Parser::parseStmt()
   {
      Token t = peek();
      switch (t.kind)
      {
      case TokenKind::KwIf:          return parseIfStmt();
      case TokenKind::KwWhile:       return parseWhileStmt();
      case TokenKind::KwDo:          return parseDoWhileStmt();
      case TokenKind::KwFor:         return parseForStmt();
      case TokenKind::KwForeach:
      case TokenKind::KwForeachStr:  return parseForeachStmt();
      case TokenKind::KwSwitch:
      case TokenKind::KwSwitchStr:   return parseSwitchStmt();
      case TokenKind::KwDatablock:   return parseDatablockDecl();
      case TokenKind::KwReturn:      return parseReturnStmt();
      case TokenKind::KwBreak:       return parseBreakStmt();
      case TokenKind::KwContinue:    return parseContinueStmt();
      case TokenKind::DocBlock:
      {
         Token doc = advance();
         StrLitNode* n = mCtx.alloc<StrLitNode>((S32)doc.line(), doc.strVal, false, true);
         n->range = doc.range;
         return n;
      }
      case TokenKind::TTag:
         if (peek(1).kind == TokenKind::Equals) return parseTTagSetStmt();
         [[fallthrough]];
      default:
         return parseExpressionStmt();
      }
   }

   //-----------------------------------------------------------------------
   // Individual statements
   //-----------------------------------------------------------------------
   Node* Parser::parseIfStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwIf, "Expected 'if'");
      expect(TokenKind::LParen, "Expected '('");
      Node* test = parseExpr();
      expect(TokenKind::RParen, "Expected ')'");
      Node* thenBlock = parseStmtBlock();
      Node* elseBlock = nullptr;
      if (match(TokenKind::KwElse)) elseBlock = parseStmtBlock();
      return mCtx.alloc<IfNode>(ln, static_cast<ExprNode*>(test),
         thenBlock, elseBlock, false);
   }

   Node* Parser::parseWhileStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwWhile, "Expected 'while'");
      expect(TokenKind::LParen, "Expected '('");
      Node* test = parseExpr();
      expect(TokenKind::RParen, "Expected ')'");
      Node* body = parseStmtBlock();
      // Missing test → IntLit 1 (same semantics as legacy for(;;))
      if (!test) test = mCtx.alloc<IntLitNode>(ln, 1);
      return mCtx.alloc<LoopNode>(ln, nullptr, static_cast<ExprNode*>(test),
         nullptr, body, false);
   }

   Node* Parser::parseDoWhileStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwDo, "Expected 'do'");
      Node* body = parseStmtBlock();
      expect(TokenKind::KwWhile, "Expected 'while' after do-block");
      expect(TokenKind::LParen, "Expected '('");
      Node* test = parseExpr();
      expect(TokenKind::RParen, "Expected ')'");
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<LoopNode>(ln, nullptr, static_cast<ExprNode*>(test),
         nullptr, body, true);
   }

   Node* Parser::parseForStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwFor, "Expected 'for'");
      expect(TokenKind::LParen, "Expected '('");

      Node* init = nullptr;
      if (!check(TokenKind::Semicolon)) init = parseExpr();
      expect(TokenKind::Semicolon, "Expected ';' after for-init");

      Node* test = nullptr;
      if (!check(TokenKind::Semicolon)) test = parseExpr();
      if (!test) test = mCtx.alloc<IntLitNode>(ln, 1);
      expect(TokenKind::Semicolon, "Expected ';' after for-test");

      Node* incr = nullptr;
      if (!check(TokenKind::RParen)) incr = parseExpr();

      if (!expect(TokenKind::RParen, "Expected ')'").is(TokenKind::RParen))
         synchronize({ TokenKind::LBrace, TokenKind::Semicolon });

      Node* body = parseStmtBlock();
      return mCtx.alloc<LoopNode>(ln, static_cast<ExprNode*>(init),
         static_cast<ExprNode*>(test),
         static_cast<ExprNode*>(incr), body, false);
   }

   Node* Parser::parseForeachStmt()
   {
      S32  ln = line();
      bool isStr = check(TokenKind::KwForeachStr);
      advance();
      expect(TokenKind::LParen, "Expected '('");
      Token varToken = expect(TokenKind::Var, "Expected loop variable");
      StringTableEntry varName = StringTable->insert(varToken.strVal);
      expect(TokenKind::KwIn, "Expected 'in'");
      Node* container = parseExpr();
      expect(TokenKind::RParen, "Expected ')'");
      Node* body = parseStmtBlock();
      return mCtx.alloc<ForeachNode>(ln, varName, static_cast<ExprNode*>(container),
         body, isStr);
   }

   Node* Parser::parseSwitchStmt()
   {
      S32  ln = line();
      bool isStr = check(TokenKind::KwSwitchStr);
      advance();
      expect(TokenKind::LParen, "Expected '('");
      Node* expr = parseExpr();
      expect(TokenKind::RParen, "Expected ')'");
      expect(TokenKind::LBrace, "Expected '{'");

      IfNode* caseRoot = parseCaseBlock(isStr, expr);

      expect(TokenKind::RBrace, "Expected '}'");
      return caseRoot;
   }

   //-----------------------------------------------------------------------
   // parseSwitchBody — statement list that stops at 'case', 'or', or 'default'
   // so that case blocks don't consume each other.
   //-----------------------------------------------------------------------
   Node* Parser::parseSwitchBody()
   {
      Node* list = nullptr;
      while (!check(TokenKind::RBrace) &&
         !check(TokenKind::KwCase) &&
         !check(TokenKind::KwCaseOr) &&
         !check(TokenKind::KwDefault) &&
         !atEnd())
      {
         Node* s = parseStmt();
         if (!s)
         {
            if (!synchronize({ TokenKind::Semicolon, TokenKind::RBrace,
                               TokenKind::KwCase,    TokenKind::KwDefault })) break;
            match(TokenKind::Semicolon);
            continue;
         }
         nodeAppend(list, s);
      }
      return list;
   }

   IfNode* Parser::parseCaseBlock(bool isString, Node* switchExpr)
   {
      expect(TokenKind::KwCase, "Expected 'case'");
      S32 ln = line();

      Node* caseExpr = parseExpr();
      while (match(TokenKind::KwCaseOr))
      {
         Node* extra = parseExpr();
         if (extra) nodeAppend(caseExpr, extra);
      }
      expect(TokenKind::Colon, "Expected ':' after case");

      // Use parseSwitchBody so we stop at the next case/default boundary
      Node* body = parseSwitchBody();

      Node* elseBlock = nullptr;
      if (match(TokenKind::KwDefault))
      {
         expect(TokenKind::Colon, "Expected ':' after 'default'");
         elseBlock = parseSwitchBody();
      }
      else if (check(TokenKind::KwCase))
         elseBlock = parseCaseBlock(isString, switchExpr);

      // Wrap the case expression with an equality comparison against the switch
      // expression so the CodeGen can emit a simple if-else chain.
      // switch  (int)  → integer equality via numeric compare
      // switch$ (str)  → string equality via $= (StrEqNode, eq=true)
      Node* testNode = nullptr;
      if (isString)
      {
         // Walk the caseExpr chain: each alt becomes a separate StrEq test
         // chained with OR.  For simplicity, wrap the first/only value.
         // Multiple 'or' alternatives handled via elseBlock chaining below.
         testNode = mCtx.alloc<StrEqNode>(ln,
            static_cast<ExprNode*>(switchExpr),
            static_cast<ExprNode*>(caseExpr),
            true /* eq = $= */);
      }
      else
      {
         // Integer switch — wrap as == comparison
         testNode = mCtx.alloc<BinaryIntNode>(ln,
            (S32)TokenKind::OpEq,
            static_cast<ExprNode*>(switchExpr),
            static_cast<ExprNode*>(caseExpr));
      }

      return mCtx.alloc<IfNode>(ln, static_cast<ExprNode*>(testNode),
         body, elseBlock, elseBlock != nullptr);
   }

   Node* Parser::parseDatablockDecl()
   {
      S32 ln = line();
      expect(TokenKind::KwDatablock, "Expected 'datablock'");
      Node* className = parseClassNameExpr();
      expect(TokenKind::LParen, "Expected '('");
      Node* objName = parseObjectName();
      StringTableEntry parent = parseParentBlock();
      expect(TokenKind::RParen, "Expected ')'");
      expect(TokenKind::LBrace, "Expected '{'");
      SlotAssignNode* slots = parseSlotAssignList();
      expect(TokenKind::RBrace, "Expected '}'");
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<ObjectDeclNode>(ln,
         static_cast<ExprNode*>(className),
         static_cast<ExprNode*>(objName),
         nullptr, parent, slots, nullptr,
         true, false, false);
   }

   Node* Parser::parseReturnStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwReturn, "Expected 'return'");
      Node* expr = nullptr;
      if (!check(TokenKind::Semicolon)) expr = parseExpr();
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<ReturnNode>(ln, static_cast<ExprNode*>(expr));
   }

   Node* Parser::parseBreakStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwBreak, "Expected 'break'");
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<BreakNode>(ln);
   }

   Node* Parser::parseContinueStmt()
   {
      S32 ln = line();
      expect(TokenKind::KwContinue, "Expected 'continue'");
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<ContinueNode>(ln);
   }

   Node* Parser::parseTTagSetStmt()
   {
      Token tag = advance();
      StringTableEntry tagName = StringTable->insert(tag.strVal);
      expect(TokenKind::Equals, "Expected '='");
      Node* valExpr = parseExpr();
      Node* strExpr = nullptr;
      if (match(TokenKind::Comma)) strExpr = parseExpr();
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<TTagSetNode>((S32)tag.line(), tagName,
         static_cast<ExprNode*>(valExpr),
         static_cast<ExprNode*>(strExpr));
   }

   Node* Parser::parseExpressionStmt()
   {
      Node* expr = parseExpr();
      if (!expr) return nullptr;
      expect(TokenKind::Semicolon, "Expected ';'");
      return mCtx.alloc<ExprStmtNode>(expr->line, static_cast<ExprNode*>(expr));
   }

   //-----------------------------------------------------------------------
   // Object helpers
   //-----------------------------------------------------------------------
   StringTableEntry Parser::parseParentBlock()
   {
      if (match(TokenKind::Colon))
      {
         Token parent = expect(TokenKind::Ident, "Expected parent class name");
         return StringTable->insert(parent.strVal);
      }
      return nullptr;
   }

   Node* Parser::parseObjectArgs()
   {
      if (match(TokenKind::Comma)) return parseArgList();
      return nullptr;
   }

   Node* Parser::parseObjectName()
   {
      if (check(TokenKind::RParen) || check(TokenKind::Comma))
      {
         S32 ln = line();
         const char* empty = mCtx.allocString("");
         return mCtx.alloc<StrLitNode>(ln, empty, false);
      }
      return parseExpr();
   }

   Node* Parser::parseClassNameExpr()
   {
      if (match(TokenKind::LParen))
      {
         Node* e = parseExpr();
         expect(TokenKind::RParen, "Expected ')'");
         return e;
      }
      Token ident = expect(TokenKind::Ident, "Expected class name");
      return mCtx.alloc<ConstNode>((S32)ident.line(), StringTable->insert(ident.strVal));
   }

   Parser::ObjectBlock Parser::parseObjectDeclBlock()
   {
      ObjectBlock decl{ nullptr, nullptr };
      decl.slots = parseSlotAssignList();

      ObjectDeclNode* lastObj = nullptr;
      while (check(TokenKind::KwNew) || check(TokenKind::KwSingleton) ||
         check2(TokenKind::Ident, TokenKind::LParen))
      {
         ObjectDeclNode* obj = parseObjectDecl();
         if (!obj) break;
         expect(TokenKind::Semicolon, "Expected ';'");
         if (!decl.decls) decl.decls = obj;
         else             lastObj->next = obj;
         lastObj = obj;
      }
      return decl;
   }

   ObjectDeclNode* Parser::parseObjectDecl()
   {
      S32 ln = line();
      bool isSingleton = false;
      bool isInternal = false;

      if (check(TokenKind::KwNew))       advance();
      else if (check(TokenKind::KwSingleton)) { advance(); isSingleton = true; }

      Node* className = parseClassNameExpr();
      expect(TokenKind::LParen, "Expected '('");

      if (check(TokenKind::LBracket))
      {
         advance();
         className = parseExpr();
         expect(TokenKind::RBracket, "Expected ']'");
         isInternal = true;
      }

      Node* objName = parseObjectName();
      StringTableEntry parent = parseParentBlock();
      Node* args = parseObjectArgs();
      expect(TokenKind::RParen, "Expected ')'");

      SlotAssignNode* slots = nullptr;
      ObjectDeclNode* subObjs = nullptr;

      if (check(TokenKind::LBrace))
      {
         advance();
         ObjectBlock block = parseObjectDeclBlock();
         slots = block.slots;
         subObjs = block.decls;
         expect(TokenKind::RBrace, "Expected '}'");
      }

      return mCtx.alloc<ObjectDeclNode>(ln,
         static_cast<ExprNode*>(className),
         static_cast<ExprNode*>(objName),
         static_cast<ExprNode*>(args),
         parent, slots, subObjs,
         false, isInternal, isSingleton);
   }

   SlotAssignNode* Parser::parseSlotAssignList()
   {
      SlotAssignNode* list = nullptr;
      SlotAssignNode* last = nullptr;

      while (!check(TokenKind::RBrace) && !check(TokenKind::KwNew) &&
         !check(TokenKind::KwSingleton) && !atEnd())
      {
         bool isSlot = (check(TokenKind::Ident) &&
            (peek(1).kind == TokenKind::Equals ||
               peek(1).kind == TokenKind::LBracket))
            || (check(TokenKind::TypeIdent) &&
               peek(1).kind == TokenKind::Ident)
            || check2(TokenKind::KwDatablock, TokenKind::Equals);
         if (!isSlot) break;

         SlotAssignNode* s = parseSlotAssign();
         if (!s) break;
         if (!list) { list = last = s; }
         else { last->next = s; last = s; }
      }
      return list;
   }

   SlotAssignNode* Parser::parseSlotAssign()
   {
      S32 ln = line();
      U32 typeID = (U32)-1;

      if (check(TokenKind::TypeIdent))
      {
         Token t = advance();
         typeID = (U32)t.intVal;
      }

      if (check(TokenKind::KwDatablock))
      {
         advance();
         expect(TokenKind::Equals, "Expected '='");
         Node* val = parseExpr();
         expect(TokenKind::Semicolon, "Expected ';'");
         return mCtx.alloc<SlotAssignNode>(ln, nullptr, nullptr,
            StringTable->insert("datablock"),
            static_cast<ExprNode*>(val), typeID);
      }

      Token nameToken = expect(TokenKind::Ident, "Expected slot name");
      StringTableEntry slotName = StringTable->insert(nameToken.strVal);

      Node* arrayIdx = nullptr;
      if (match(TokenKind::LBracket))
      {
         arrayIdx = parseAidxExpr();
         expect(TokenKind::RBracket, "Expected ']'");
      }

      if (match(TokenKind::Equals))
      {
         Node* val;
         if (check(TokenKind::LBrace))
         {
            advance();
            val = parseArgList();
            expect(TokenKind::RBrace, "Expected '}'");
         }
         else val = parseExpr();
         expect(TokenKind::Semicolon, "Expected ';'");
         return mCtx.alloc<SlotAssignNode>(ln, nullptr,
            static_cast<ExprNode*>(arrayIdx), slotName,
            static_cast<ExprNode*>(val), typeID);
      }

      mCtx.error(peek().range, "Expected '=' in slot assignment.");
      synchronize({ TokenKind::Semicolon, TokenKind::RBrace });
      match(TokenKind::Semicolon);
      return nullptr;
   }

   //-----------------------------------------------------------------------
   // Pratt expression parser
   //-----------------------------------------------------------------------

   struct PrecRule { TokenKind kind; U32 prec; bool rightAssoc; };

   static const PrecRule sPrecTable[] =
   {
      // Assignments — right associative prec 2
      { TokenKind::Equals,        2, true  },
      { TokenKind::OpPlusAssign,  2, true  },
      { TokenKind::OpMinusAssign, 2, true  },
      { TokenKind::OpMulAssign,   2, true  },
      { TokenKind::OpDivAssign,   2, true  },
      { TokenKind::OpModAssign,   2, true  },
      { TokenKind::OpAndAssign,   2, true  },
      { TokenKind::OpXorAssign,   2, true  },
      { TokenKind::OpOrAssign,    2, true  },
      { TokenKind::OpShlAssign,   2, true  },
      { TokenKind::OpShrAssign,   2, true  },
      // Logical
      { TokenKind::OpOr,          4, false },
      { TokenKind::OpAnd,         5, false },
      { TokenKind::Pipe,          6, false },
      { TokenKind::Caret,         7, false },
      { TokenKind::Ampersand,     8, false },
      // Equality
      { TokenKind::OpEq,          9, false },
      { TokenKind::OpNe,          9, false },
      // Relational
      { TokenKind::LessThan,     10, false },
      { TokenKind::OpLe,         10, false },
      { TokenKind::GreaterThan,  10, false },
      { TokenKind::OpGe,         10, false },
      // String ops
      { TokenKind::OpCat,        11, false },
      { TokenKind::OpCatSpc,     11, false },
      { TokenKind::OpCatNl,      11, false },
      { TokenKind::OpCatTab,     11, false },
      { TokenKind::OpStrEq,      11, false },
      { TokenKind::OpStrNe,      11, false },
      // Shift
      { TokenKind::OpShl,        12, false },
      { TokenKind::OpShr,        12, false },
      // Additive
      { TokenKind::Plus,         13, false },
      { TokenKind::Minus,        13, false },
      // Multiplicative
      { TokenKind::Star,         14, false },
      { TokenKind::ForwardSlash, 14, false },
      { TokenKind::Percent,      14, false },
   };
   static const U32 kPrecTableCount = sizeof(sPrecTable) / sizeof(sPrecTable[0]);

   const Parser::InfixRule* Parser::getInfixRule(TokenKind kind)
   {
      for (U32 i = 0; i < kPrecTableCount; ++i)
         if (sPrecTable[i].kind == kind)
            return (const InfixRule*)&sPrecTable[i];
      return nullptr;
   }

   Node* Parser::parseExpr(U32 minPrec)
   {
      Node* lhs = parseUnary();
      if (!lhs) return nullptr;
      lhs = parsePostfix(lhs);

      for (;;)
      {
         Token op = peek();

         // Ternary
         if (op.kind == TokenKind::Question && minPrec <= 3)
         {
            S32 ln = op.line();
            advance();
            Node* trueExpr = parseExpr(0);
            expect(TokenKind::Colon, "Expected ':'");
            Node* falseExpr = parseExpr(3);
            lhs = mCtx.alloc<TernaryNode>(ln,
               static_cast<ExprNode*>(lhs),
               static_cast<ExprNode*>(trueExpr),
               static_cast<ExprNode*>(falseExpr));
            lhs = parsePostfix(lhs);
            continue;
         }

         const InfixRule* rule = getInfixRule(op.kind);
         if (!rule || rule->prec < minPrec) break;

         S32 ln = op.line();
         advance();
         U32   nextPrec = rule->rightAssoc ? rule->prec : rule->prec + 1;
         Node* rhs = parseExpr(nextPrec);

         switch (op.kind)
         {
            // ----- Assignment -----
         case TokenKind::Equals:
         {
            if (auto* v = (lhs->kind == NodeKind::Var ? static_cast<VarNode*>(lhs) : nullptr))
               lhs = mCtx.alloc<AssignNode>(ln, v->name,
                  v->index, static_cast<ExprNode*>(rhs));
            else if (auto* s = (lhs->kind == NodeKind::SlotAccess ? static_cast<SlotAccessNode*>(lhs) : nullptr))
               lhs = mCtx.alloc<SlotAssignNode>(ln, s->objectExpr, s->arrayExpr,
                  s->slotName, static_cast<ExprNode*>(rhs), (U32)-1);
            else
               mCtx.error(op.range, "Left side of '=' is not assignable.");
            break;
         }

         // ----- Compound assignment -----
         case TokenKind::OpPlusAssign:
         case TokenKind::OpMinusAssign:
         case TokenKind::OpMulAssign:
         case TokenKind::OpDivAssign:
         case TokenKind::OpModAssign:
         case TokenKind::OpAndAssign:
         case TokenKind::OpXorAssign:
         case TokenKind::OpOrAssign:
         case TokenKind::OpShlAssign:
         case TokenKind::OpShrAssign:
         {
            static const struct { TokenKind assign; S32 base; } map[] = {
                { TokenKind::OpPlusAssign,  '+' },
                { TokenKind::OpMinusAssign, '-' },
                { TokenKind::OpMulAssign,   '*' },
                { TokenKind::OpDivAssign,   '/' },
                { TokenKind::OpModAssign,   '%' },
                { TokenKind::OpAndAssign,   '&' },
                { TokenKind::OpXorAssign,   '^' },
                { TokenKind::OpOrAssign,    '|' },
                { TokenKind::OpShlAssign,   (S32)TokenKind::OpShl },
                { TokenKind::OpShrAssign,   (S32)TokenKind::OpShr },
            };
            S32 baseOp = '+';
            for (auto& m : map) if (m.assign == op.kind) { baseOp = m.base; break; }

            if (auto* v = (lhs->kind == NodeKind::Var ? static_cast<VarNode*>(lhs) : nullptr))
               lhs = mCtx.alloc<AssignOpNode>(ln, v->name, v->index,
                  static_cast<ExprNode*>(rhs), baseOp);
            else if (auto* s = (lhs->kind == NodeKind::SlotAccess ? static_cast<SlotAccessNode*>(lhs) : nullptr))
               lhs = mCtx.alloc<SlotAssignOpNode>(ln, s->objectExpr, s->arrayExpr,
                  s->slotName, static_cast<ExprNode*>(rhs), baseOp);
            else
               mCtx.error(op.range, "Left side of compound assignment is not assignable.");
            break;
         }

         // ----- Integer binary -----
         case TokenKind::OpOr:
         case TokenKind::OpAnd:
         case TokenKind::Pipe:
         case TokenKind::Caret:
         case TokenKind::Ampersand:
         case TokenKind::OpEq:
         case TokenKind::OpNe:
         case TokenKind::LessThan:
         case TokenKind::OpLe:
         case TokenKind::GreaterThan:
         case TokenKind::OpGe:
         case TokenKind::OpShl:
         case TokenKind::OpShr:
         case TokenKind::Percent:
            lhs = mCtx.alloc<BinaryIntNode>(ln, (S32)op.kind,
               static_cast<ExprNode*>(lhs), static_cast<ExprNode*>(rhs));
            break;

            // ----- Float binary -----
         case TokenKind::Plus:
         case TokenKind::Minus:
         case TokenKind::Star:
         case TokenKind::ForwardSlash:
            lhs = mCtx.alloc<BinaryFloatNode>(ln, (S32)op.kind,
               static_cast<ExprNode*>(lhs), static_cast<ExprNode*>(rhs));
            break;

            // ----- String concat -----
         case TokenKind::OpCat:
         case TokenKind::OpCatSpc:
         case TokenKind::OpCatNl:
         case TokenKind::OpCatTab:
            lhs = mCtx.alloc<StrCatNode>(ln,
               static_cast<ExprNode*>(lhs),
               static_cast<ExprNode*>(rhs),
               op.appendChar);
            break;

         case TokenKind::OpStrEq:
            lhs = mCtx.alloc<StrEqNode>(ln,
               static_cast<ExprNode*>(lhs),
               static_cast<ExprNode*>(rhs), true);
            break;
         case TokenKind::OpStrNe:
            lhs = mCtx.alloc<StrEqNode>(ln,
               static_cast<ExprNode*>(lhs),
               static_cast<ExprNode*>(rhs), false);
            break;

         default:
            mCtx.error(op.range, "Unexpected infix operator.");
            break;
         }

         lhs = parsePostfix(lhs);
      }
      return lhs;
   }

   //-----------------------------------------------------------------------
   // Unary
   //-----------------------------------------------------------------------
   Node* Parser::parseUnary()
   {
      Token t = peek();
      switch (t.kind)
      {
      case TokenKind::Bang:
         advance();
         // Use parsePostfix(parseUnary()) so member access / method calls
         // bind BEFORE the ! operator.  Without this, !obj.method() incorrectly
         // parses as (!obj).method() making the UnaryIntNode the first argument
         // of the call instead of the call's return value being negated.
         return mCtx.alloc<UnaryIntNode>((S32)t.line(), '!',
            static_cast<ExprNode*>(parsePostfix(parseUnary())));
      case TokenKind::Tilde:
         advance();
         return mCtx.alloc<UnaryIntNode>((S32)t.line(), '~',
            static_cast<ExprNode*>(parsePostfix(parseUnary())));
      case TokenKind::Minus:
         advance();
         return mCtx.alloc<UnaryFloatNode>((S32)t.line(),
            static_cast<ExprNode*>(parsePostfix(parseUnary())));
      case TokenKind::Star:
         advance();
         return mCtx.alloc<TTagDerefNode>((S32)t.line(),
            static_cast<ExprNode*>(parsePostfix(parseUnary())));
      case TokenKind::OpPlusPlus:
      {
         advance();
         Node* operand = parsePostfix(parsePrimary());
         if (auto* v = (operand->kind == NodeKind::Var ? static_cast<VarNode*>(operand) : nullptr))
            return mCtx.alloc<AssignOpNode>((S32)t.line(), v->name, v->index,
               mCtx.alloc<FloatLitNode>((S32)t.line(), 1.0),
               (S32)TokenKind::OpPlusPlus);
         mCtx.error(t.range, "Operand of '++' must be a variable.");
         return operand;
      }
      case TokenKind::OpMinusMinus:
      {
         advance();
         Node* operand = parsePostfix(parsePrimary());
         if (auto* v = (operand->kind == NodeKind::Var ? static_cast<VarNode*>(operand) : nullptr))
            return mCtx.alloc<AssignOpNode>((S32)t.line(), v->name, v->index,
               mCtx.alloc<FloatLitNode>((S32)t.line(), 1.0),
               (S32)TokenKind::OpMinusMinus);
         mCtx.error(t.range, "Operand of '--' must be a variable.");
         return operand;
      }
      default: return parsePrimary();
      }
   }

   //-----------------------------------------------------------------------
   // Primary
   //-----------------------------------------------------------------------
   Node* Parser::parsePrimary()
   {
      Token t = peek();
      switch (t.kind)
      {
      case TokenKind::IntLiteral:
         advance();
         return mCtx.alloc<IntLitNode>((S32)t.line(), (S32)t.intVal);

      case TokenKind::FloatLiteral:
         advance();
         return mCtx.alloc<FloatLitNode>((S32)t.line(), t.floatVal);

      case TokenKind::StringLiteral:
         advance();
         return mCtx.alloc<StrLitNode>((S32)t.line(), t.strVal, false);

      case TokenKind::TagStringLiteral:
         advance();
         return mCtx.alloc<StrLitNode>((S32)t.line(), t.strVal, true);

      case TokenKind::TTag:
         advance();
         return mCtx.alloc<TTagExprNode>((S32)t.line(), StringTable->insert(t.strVal));

      case TokenKind::Var:
      {
         advance();
         StringTableEntry name = StringTable->insert(t.strVal);
         Node* idx = nullptr;
         if (match(TokenKind::LBracket))
         {
            idx = parseAidxExpr();
            expect(TokenKind::RBracket, "Expected ']'");
         }
         return mCtx.alloc<VarNode>((S32)t.line(), name,
            static_cast<ExprNode*>(idx));
      }

      case TokenKind::LParen:
      {
         advance();
         Node* e = parseExpr();
         expect(TokenKind::RParen, "Expected ')'");
         return e;
      }

      case TokenKind::KwNew:
      case TokenKind::KwSingleton:
         return parseObjectDecl();

      case TokenKind::Ident:
      {
         advance();
         StringTableEntry firstName = StringTable->insert(t.strVal);

         if (check(TokenKind::LParen))
         {
            advance();
            Node* args = parseArgList();
            expect(TokenKind::RParen, "Expected ')'");
            return mCtx.alloc<FuncCallNode>((S32)t.line(), firstName, nullptr,
               static_cast<ExprNode*>(args), CallType::Function);
         }

         if (check(TokenKind::OpColonColon))
         {
            advance();
            Token methodName = expect(TokenKind::Ident, "Expected name after '::'");
            StringTableEntry ns = firstName;
            StringTableEntry fn = StringTable->insert(methodName.strVal);
            expect(TokenKind::LParen, "Expected '('");
            Node* args = parseArgList();
            expect(TokenKind::RParen, "Expected ')'");
            // Determine call type: Parent:: = ParentCall, else StaticCall
            CallType ct = (dStricmp(ns, "Parent") == 0)
               ? CallType::Parent : CallType::Static;
            return mCtx.alloc<FuncCallNode>((S32)t.line(), fn, ns,
               static_cast<ExprNode*>(args), ct);
         }

         return mCtx.alloc<ConstNode>((S32)t.line(), firstName);
      }

      case TokenKind::KwAssert:
      {
         advance();
         expect(TokenKind::LParen, "Expected '('");
         Node* test = parseExpr();
         const char* msg = nullptr;
         if (match(TokenKind::Comma))
         {
            Token msgToken = expect(TokenKind::StringLiteral, "Expected string message");
            msg = msgToken.strVal;
         }
         expect(TokenKind::RParen, "Expected ')'");
         return mCtx.alloc<AssertNode>((S32)t.line(),
            static_cast<ExprNode*>(test), msg);
      }

      default:
         mCtx.error(t.range, "Unexpected token in expression.");
         advance();
         return nullptr;
      }
   }

   //-----------------------------------------------------------------------
   // Postfix
   //-----------------------------------------------------------------------
   Node* Parser::parsePostfix(Node* lhs)
   {
      for (;;)
      {
         Token t = peek();

         if (t.kind == TokenKind::Dot)
         {
            advance();
            Token member = expect(TokenKind::Ident, "Expected member name");
            StringTableEntry slotName = StringTable->insert(member.strVal);

            if (check(TokenKind::LParen))
            {
               advance();
               Node* args = parseArgList();
               expect(TokenKind::RParen, "Expected ')'");
               // For method calls, prepend lhs as the implicit first argument
               // so exec() can find it as callArgv[1].
               FuncCallNode* call = mCtx.alloc<FuncCallNode>((S32)t.line(),
                  slotName, nullptr,
                  static_cast<ExprNode*>(lhs), CallType::Method);
               // append further args
               if (args)
                  lhs->next = args;
               lhs = call;
            }
            else
            {
               Node* arrayExpr = nullptr;
               if (match(TokenKind::LBracket))
               {
                  arrayExpr = parseAidxExpr();
                  expect(TokenKind::RBracket, "Expected ']'");
               }
               lhs = mCtx.alloc<SlotAccessNode>((S32)t.line(),
                  static_cast<ExprNode*>(lhs),
                  static_cast<ExprNode*>(arrayExpr),
                  slotName);
            }
            continue;
         }

         if (t.kind == TokenKind::OpIntName || t.kind == TokenKind::OpIntNameR)
         {
            advance();
            Node* slotExpr = parseClassNameExpr();
            lhs = mCtx.alloc<InternalSlotNode>((S32)t.line(),
               static_cast<ExprNode*>(lhs),
               static_cast<ExprNode*>(slotExpr),
               t.kind == TokenKind::OpIntNameR);
            continue;
         }

         if (t.kind == TokenKind::OpPlusPlus)
         {
            advance();
            if (auto* v = (lhs->kind == NodeKind::Var ? static_cast<VarNode*>(lhs) : nullptr))
               lhs = mCtx.alloc<AssignOpNode>((S32)t.line(), v->name, v->index,
                  mCtx.alloc<FloatLitNode>((S32)t.line(), 1.0),
                  (S32)TokenKind::OpPlusPlus);
            else mCtx.error(t.range, "Operand of '++' must be a variable. (line: %d, col: %d)", t.range.start.line, t.range.start.col);
            continue;
         }

         if (t.kind == TokenKind::OpMinusMinus)
         {
            advance();
            if (auto* v = (lhs->kind == NodeKind::Var ? static_cast<VarNode*>(lhs) : nullptr))
               lhs = mCtx.alloc<AssignOpNode>((S32)t.line(), v->name, v->index,
                  mCtx.alloc<FloatLitNode>((S32)t.line(), 1.0),
                  (S32)TokenKind::OpMinusMinus);
            else mCtx.error(t.range, "Operand of '--' must be a variable. (line: %d, col: %d)", t.range.start.line, t.range.start.col);
            continue;
         }

         break;
      }
      return lhs;
   }

   //-----------------------------------------------------------------------
   // Arg list / aidx
   //-----------------------------------------------------------------------
   Node* Parser::parseArgList()
   {
      if (check(TokenKind::RParen) || check(TokenKind::RBrace)) return nullptr;
      Node* first = parseExpr();
      while (match(TokenKind::Comma))
      {
         Node* next = parseExpr();
         if (next) nodeAppend(first, next);
      }
      return first;
   }

   Node* Parser::parseAidxExpr()
   {
      Node* first = parseExpr();
      while (match(TokenKind::Comma))
      {
         Node* next = parseExpr();
         if (next)
            first = mCtx.alloc<CommaCatNode>(first->line,
               static_cast<ExprNode*>(first),
               static_cast<ExprNode*>(next));
      }
      return first;
   }

} // namespace TS2
