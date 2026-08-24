#include "newConsole/torquescript2/parser.h"

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      Parser::Parser(Lexer& lexer, StringTableEntry originName)
         : mLexer(lexer)
         , mOriginName(originName)
      {
         advanceToken();
      }

      void Parser::advanceToken()
      {
         mPrevious = mCurrent;
         mCurrent = mLexer.next();
      }

      bool Parser::check(TokenKind kind) const
      {
         return mCurrent.kind == kind;
      }

      bool Parser::checkAny(std::initializer_list<TokenKind> kinds) const
      {
         for (TokenKind k : kinds)
            if (mCurrent.kind == k)
               return true;
         return false;
      }

      bool Parser::matchToken(TokenKind kind)
      {
         if (!check(kind))
            return false;
         advanceToken();
         return true;
      }

      const Token& Parser::expect(TokenKind kind, const char* what)
      {
         if (!check(kind))
         {
            String msg = String("expected ") + what;
            reportError(mCurrent, msg.c_str());
            // Do not advance past EOF - repeatedly calling expect() at end of
            // a malformed file must not spin forever re-consuming nothing.
            if (mCurrent.kind != TokenKind::Eof)
               advanceToken();
            return mPrevious;
         }
         advanceToken();
         return mPrevious;
      }

      void Parser::reportError(const Token& at, const char* message)
      {
         ParseDiagnostic d;
         d.line = at.line;
         d.column = at.column;
         d.message = message;
         mDiagnostics.push_back(d);
      }

      void Parser::synchronizeToStatementBoundary()
      {
         // Panic-mode recovery: skip forward until a token that plausibly
         // starts a fresh statement, or a block delimiter, or EOF. Consuming
         // through a ';' is the common case; stopping *before* a '}' (rather
         // than consuming it) matters so an enclosing parseStatementList's own
         // terminator check still sees it.
         while (!check(TokenKind::Eof) && !check(TokenKind::RBrace))
         {
            if (mPrevious.kind == TokenKind::Semicolon)
               return;

            switch (mCurrent.kind)
            {
            case TokenKind::KwIf:
            case TokenKind::KwWhile:
            case TokenKind::KwFor:
            case TokenKind::KwForeach:
            case TokenKind::KwForeachStr:
            case TokenKind::KwSwitch:
            case TokenKind::KwSwitchStr:
            case TokenKind::KwReturn:
            case TokenKind::KwBreak:
            case TokenKind::KwContinue:
            case TokenKind::KwDefine:
            case TokenKind::KwPackage:
            case TokenKind::KwDeclare:
            case TokenKind::KwDeclareSingleton:
            case TokenKind::KwDatablock:
               return;
            default:
               break;
            }
            advanceToken();
         }
      }

      ast::SourceSpan Parser::spanOf(const Token& t) const
      {
         ast::SourceSpan span;
         span.line = t.line;
         span.column = t.column;
         return span;
      }

      // =============================================================================
      // Precedence table
      //
      // Higher number binds tighter. Matches CMDgram.y's %left/%right table
      // (lines 171-187). '[' and '.' are postfix, handled by
      // parsePostfixChain, not through this table - they never reach the
      // infix precedence-climbing loop.
      // =============================================================================

      // Assignment's precedence tier - parsePostfixChain's field/index-assign
      // cases parse their RHS one tier above this so a chained assignment
      // through a field/index doesn't re-enter assignment's own
      // right-associative climbing. Kept as a named constant so it can't
      // silently drift out of sync with infixPrecedence's Assign case below.
      static constexpr S32 kAssignPrecedence = 10;
      static constexpr S32 kAboveAssignPrecedence = kAssignPrecedence + 1;

      S32 Parser::infixPrecedence(TokenKind kind) const
      {
         switch (kind)
         {
         case TokenKind::Assign:
         case TokenKind::PlusAssign: case TokenKind::MinusAssign:
         case TokenKind::MulAssign:  case TokenKind::DivAssign:
         case TokenKind::ModAssign:  case TokenKind::AndAssign:
         case TokenKind::XorAssign:  case TokenKind::OrAssign:
         case TokenKind::ShlAssign:  case TokenKind::ShrAssign:
            return kAssignPrecedence;

         case TokenKind::Question:
            return 20;

         case TokenKind::OrOr:
            return 30;
         case TokenKind::AndAnd:
            return 40;

         case TokenKind::Pipe:
            return 50;
         case TokenKind::Caret:
            return 60;
         case TokenKind::Amp:
            return 70;

         case TokenKind::Eq: case TokenKind::Ne:
            return 80;

         case TokenKind::Less: case TokenKind::Le:
         case TokenKind::Greater: case TokenKind::Ge:
            return 90;

         case TokenKind::At: case TokenKind::StrEq: case TokenKind::StrNe:
            return 100;

         case TokenKind::Shl: case TokenKind::Shr:
            return 110;

         case TokenKind::Plus: case TokenKind::Minus:
            return 120;

         case TokenKind::Star: case TokenKind::Slash: case TokenKind::Percent:
            return 130;

            // '.', '->'/'~>', '[' are postfix - see parsePostfixChain, not here.

         default:
            return -1;
         }
      }

      bool Parser::isRightAssociative(TokenKind kind) const
      {
         switch (kind)
         {
         case TokenKind::Assign:
         case TokenKind::PlusAssign: case TokenKind::MinusAssign:
         case TokenKind::MulAssign:  case TokenKind::DivAssign:
         case TokenKind::ModAssign:  case TokenKind::AndAssign:
         case TokenKind::XorAssign:  case TokenKind::OrAssign:
         case TokenKind::ShlAssign:  case TokenKind::ShrAssign:
            return true;
         default:
            return false;
         }
      }

      // =============================================================================
      // Expressions
      // =============================================================================

      static ast::BinaryOp binaryOpFor(TokenKind kind)
      {
         switch (kind)
         {
         case TokenKind::Plus:  return ast::BinaryOp::Add;
         case TokenKind::Minus: return ast::BinaryOp::Sub;
         case TokenKind::Star:  return ast::BinaryOp::Mul;
         case TokenKind::Slash: return ast::BinaryOp::Div;
         case TokenKind::Percent: return ast::BinaryOp::Mod;
         case TokenKind::Shl:   return ast::BinaryOp::Shl;
         case TokenKind::Shr:   return ast::BinaryOp::Shr;
         case TokenKind::Amp:   return ast::BinaryOp::BitAnd;
         case TokenKind::Pipe:  return ast::BinaryOp::BitOr;
         case TokenKind::Caret: return ast::BinaryOp::BitXor;
         case TokenKind::AndAnd: return ast::BinaryOp::LogicalAnd;
         case TokenKind::OrOr:   return ast::BinaryOp::LogicalOr;
         case TokenKind::Eq:    return ast::BinaryOp::Eq;
         case TokenKind::Ne:    return ast::BinaryOp::Ne;
         case TokenKind::Less:  return ast::BinaryOp::Lt;
         case TokenKind::Le:    return ast::BinaryOp::Le;
         case TokenKind::Greater: return ast::BinaryOp::Gt;
         case TokenKind::Ge:    return ast::BinaryOp::Ge;
         default:
            AssertFatal(false, "binaryOpFor - not a binary operator token");
            return ast::BinaryOp::Add;
         }
      }

      /// Maps a compound-assign token to its underlying binary op (+= does
      /// Add then assigns, etc.) - CompoundAssignExpr stores that op rather
      /// than a separate enum, so codegen shares one binary-op-apply path
      /// with plain Binary instead of duplicating it.
      static ast::BinaryOp compoundAssignOpFor(TokenKind kind)
      {
         switch (kind)
         {
         case TokenKind::PlusAssign:  return ast::BinaryOp::Add;
         case TokenKind::MinusAssign: return ast::BinaryOp::Sub;
         case TokenKind::MulAssign:   return ast::BinaryOp::Mul;
         case TokenKind::DivAssign:   return ast::BinaryOp::Div;
         case TokenKind::ModAssign:   return ast::BinaryOp::Mod;
         case TokenKind::AndAssign:   return ast::BinaryOp::BitAnd;
         case TokenKind::XorAssign:   return ast::BinaryOp::BitXor;
         case TokenKind::OrAssign:    return ast::BinaryOp::BitOr;
         case TokenKind::ShlAssign:   return ast::BinaryOp::Shl;
         case TokenKind::ShrAssign:   return ast::BinaryOp::Shr;
         default:
            AssertFatal(false, "compoundAssignOpFor - not a compound-assign token");
            return ast::BinaryOp::Add;
         }
      }

      ast::ExprHandle Parser::parseExpr(S32 minPrecedence)
      {
         ast::ExprHandle lhs = parsePrefix();
         return parseInfix(lhs, minPrecedence);
      }

      ast::ExprHandle Parser::parseInfix(ast::ExprHandle lhs, S32 minPrecedence)
      {
         for (;;)
         {
            TokenKind opKind = mCurrent.kind;
            S32 prec = infixPrecedence(opKind);
            if (prec < minPrecedence || prec < 0)
               return lhs;

            Token opToken = mCurrent;
            advanceToken();

            // Ternary is its own shape (two sub-expressions, ':' separator),
            // not a plain binary op.
            if (opKind == TokenKind::Question)
            {
               ast::ExprHandle whenTrue = parseExpr(0);
               expect(TokenKind::Colon, "':' in ternary expression");
               ast::ExprHandle whenFalse = parseExpr(prec); // left-assoc among same-precedence chains
               ast::ExprNode node;
               node.kind = ast::ExprKind::Ternary;
               node.span = spanOf(opToken);
               node.ternary = ast::TernaryExpr{ lhs, whenTrue, whenFalse };
               lhs = mUnit.addExpr(node);
               continue;
            }

            S32 nextMinPrec = isRightAssociative(opKind) ? prec : prec + 1;
            ast::ExprHandle rhs = parseExpr(nextMinPrec);

            ast::ExprNode node;
            node.span = spanOf(opToken);

            if (opKind == TokenKind::Assign)
            {
               node.kind = ast::ExprKind::Assign;
               node.assign = ast::AssignExpr{ lhs, rhs };
            }
            else if (opKind == TokenKind::PlusAssign || opKind == TokenKind::MinusAssign ||
               opKind == TokenKind::MulAssign || opKind == TokenKind::DivAssign ||
               opKind == TokenKind::ModAssign || opKind == TokenKind::AndAssign ||
               opKind == TokenKind::XorAssign || opKind == TokenKind::OrAssign ||
               opKind == TokenKind::ShlAssign || opKind == TokenKind::ShrAssign)
            {
               node.kind = ast::ExprKind::CompoundAssign;
               node.compoundAssign = ast::CompoundAssignExpr{ lhs, compoundAssignOpFor(opKind), rhs };
            }
            else if (opKind == TokenKind::At)
            {
               node.kind = ast::ExprKind::StringConcat;
               node.stringConcat = ast::StringConcatExpr{ lhs, rhs, static_cast<char>(opToken.intValue) };
            }
            else if (opKind == TokenKind::StrEq || opKind == TokenKind::StrNe)
            {
               node.kind = ast::ExprKind::StringCompare;
               node.stringCompare = ast::StringCompareExpr{ opKind == TokenKind::StrNe, lhs, rhs };
            }
            else
            {
               node.kind = ast::ExprKind::Binary;
               node.binary = ast::BinaryExpr{ binaryOpFor(opKind), lhs, rhs };
            }

            lhs = mUnit.addExpr(node);
         }
      }

      ast::ExprHandle Parser::parsePrefix()
      {
         Token startToken = mCurrent;

         if (matchToken(TokenKind::IntConst))
         {
            ast::ExprNode node;
            node.kind = ast::ExprKind::IntLiteral;
            node.span = spanOf(startToken);
            node.intLiteral = ast::IntLiteralExpr{ mPrevious.intValue };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::FloatConst))
         {
            ast::ExprNode node;
            node.kind = ast::ExprKind::FloatLiteral;
            node.span = spanOf(startToken);
            node.floatLiteral = ast::FloatLiteralExpr{ mPrevious.floatValue };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::StrAtom))
         {
            ast::ExprNode node;
            node.kind = ast::ExprKind::StringLiteral;
            node.span = spanOf(startToken);
            node.stringLiteral = ast::StringLiteralExpr{ mPrevious.text };
            return parsePostfixChain(mUnit.addExpr(node));
         }
         if (matchToken(TokenKind::TagAtom))
         {
            ast::ExprNode node;
            node.kind = ast::ExprKind::TaggedLiteral;
            node.span = spanOf(startToken);
            node.taggedLiteral = ast::TaggedLiteralExpr{ mPrevious.text };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::DocBlock))
         {
            ast::ExprNode node;
            node.kind = ast::ExprKind::DocComment;
            node.span = spanOf(startToken);
            node.docComment = ast::DocCommentExpr{ mPrevious.text };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::KwNil))
         {
            // No dedicated NIL literal kind - represented as int literal 0;
            // string contexts read it back as "" at the ScriptValue layer.
            ast::ExprNode node;
            node.kind = ast::ExprKind::IntLiteral;
            node.span = spanOf(startToken);
            node.intLiteral = ast::IntLiteralExpr{ 0 };
            return mUnit.addExpr(node);
         }

         if (check(TokenKind::Var))
         {
            advanceToken();
            ast::ExprNode node;
            node.span = spanOf(startToken);
            node.kind = (mPrevious.text[0] == '$') ? ast::ExprKind::GlobalVar : ast::ExprKind::LocalVar;
            if (node.kind == ast::ExprKind::GlobalVar)
               node.globalVar = ast::GlobalVarExpr{ mPrevious.text };
            else
               node.localVar = ast::LocalVarExpr{ mPrevious.text };
            return parsePostfixChain(mUnit.addExpr(node));
         }

         if (matchToken(TokenKind::LParen))
         {
            ast::ExprHandle inner = parseExpr(0);
            expect(TokenKind::RParen, "')' to close parenthesized expression");
            return parsePostfixChain(inner);
         }

         if (matchToken(TokenKind::Minus))
         {
            ast::ExprHandle operand = parseExpr(140); // binds tighter than any binary op - unary precedence
            ast::ExprNode node;
            node.kind = ast::ExprKind::Unary;
            node.span = spanOf(startToken);
            node.unary = ast::UnaryExpr{ ast::UnaryOp::Negate, operand };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::Bang))
         {
            ast::ExprHandle operand = parseExpr(140);
            ast::ExprNode node;
            node.kind = ast::ExprKind::Unary;
            node.span = spanOf(startToken);
            node.unary = ast::UnaryExpr{ ast::UnaryOp::LogicalNot, operand };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::Tilde))
         {
            ast::ExprHandle operand = parseExpr(140);
            ast::ExprNode node;
            node.kind = ast::ExprKind::Unary;
            node.span = spanOf(startToken);
            node.unary = ast::UnaryExpr{ ast::UnaryOp::BitNot, operand };
            return mUnit.addExpr(node);
         }
         if (matchToken(TokenKind::PlusPlus) || matchToken(TokenKind::MinusMinus))
         {
            bool isIncrement = (mPrevious.kind == TokenKind::PlusPlus);
            ast::ExprHandle target = parseExpr(140);
            ast::ExprNode node;
            node.kind = ast::ExprKind::PreIncDec;
            node.span = spanOf(startToken);
            node.preIncDec = ast::PreIncDecExpr{ isIncrement, target };
            return mUnit.addExpr(node);
         }

         if (check(TokenKind::KwDeclare))
            return parseObjectDecl(/*isDatablock*/ false, /*isSingleton*/ false);
         if (check(TokenKind::KwDeclareSingleton))
            return parseObjectDecl(/*isDatablock*/ false, /*isSingleton*/ true);
         if (check(TokenKind::KwDatablock))
            return parseObjectDecl(/*isDatablock*/ true, /*isSingleton*/ false);

         if (check(TokenKind::Ident) || check(TokenKind::TypeIdent))
         {
            StringTableEntry firstName = mCurrent.text;
            advanceToken();

            StringTableEntry namespaceName = nullptr;
            StringTableEntry name = firstName;
            if (matchToken(TokenKind::ColonColon))
            {
               namespaceName = firstName;
               name = expect(TokenKind::Ident, "identifier after '::'").text;
            }

            if (check(TokenKind::LParen))
            {
               advanceToken();
               ast::ListHandle args = parseExprList(TokenKind::RParen);
               expect(TokenKind::RParen, "')' to close call arguments");
               ast::ExprNode node;
               node.kind = ast::ExprKind::Call;
               node.span = spanOf(startToken);
               node.call = ast::CallExpr{ namespaceName, name, args };
               return parsePostfixChain(mUnit.addExpr(node));
            }

            // A bare identifier with no call parens isn't a valid expression
            // here (identifiers are function/type names, not values).
            reportError(startToken, "unexpected bare identifier (expected a call, e.g. 'name(...)')");
            ast::ExprNode node;
            node.kind = ast::ExprKind::IntLiteral;
            node.span = spanOf(startToken);
            node.intLiteral = ast::IntLiteralExpr{ 0 };
            return mUnit.addExpr(node);
         }

         reportError(startToken, "expected an expression");
         advanceToken();
         ast::ExprNode node;
         node.kind = ast::ExprKind::IntLiteral;
         node.span = spanOf(startToken);
         node.intLiteral = ast::IntLiteralExpr{ 0 };
         return mUnit.addExpr(node);
      }

      ast::ExprHandle Parser::parsePostfixChain(ast::ExprHandle base)
      {
         for (;;)
         {
            Token opToken = mCurrent;

            if (matchToken(TokenKind::Dot) || matchToken(TokenKind::IntName) || matchToken(TokenKind::IntNameR))
            {
               bool isInternal = (mPrevious.kind != TokenKind::Dot);
               StringTableEntry field = expect(TokenKind::Ident, "field/method name").text;

               if (check(TokenKind::LParen))
               {
                  advanceToken();
                  ast::ListHandle args = parseExprList(TokenKind::RParen);
                  expect(TokenKind::RParen, "')' to close method call arguments");
                  ast::ExprNode node;
                  node.kind = ast::ExprKind::MethodCall;
                  node.span = spanOf(opToken);
                  node.methodCall = ast::MethodCallExpr{ base, field, args };
                  base = mUnit.addExpr(node);
                  continue;
               }

               if (check(TokenKind::Assign) && !isInternal)
               {
                  advanceToken();
                  ast::ExprHandle value = parseExpr(kAboveAssignPrecedence);
                  ast::ExprNode node;
                  node.kind = ast::ExprKind::FieldAssign;
                  node.span = spanOf(opToken);
                  node.fieldAssign = ast::FieldAssignExpr{ base, field, isInternal, value };
                  base = mUnit.addExpr(node);
                  continue;
               }

               ast::ExprNode node;
               node.kind = ast::ExprKind::FieldAccess;
               node.span = spanOf(opToken);
               node.fieldAccess = ast::FieldAccessExpr{ base, field, isInternal };
               base = mUnit.addExpr(node);
               continue;
            }

            if (matchToken(TokenKind::LBracket))
            {
               ast::ExprHandle index = parseExpr(0);
               expect(TokenKind::RBracket, "']' to close index expression");

               if (check(TokenKind::Assign))
               {
                  advanceToken();
                  ast::ExprHandle value = parseExpr(kAboveAssignPrecedence);
                  ast::ExprNode node;
                  node.kind = ast::ExprKind::IndexAssign;
                  node.span = spanOf(opToken);
                  node.indexAssign = ast::IndexAssignExpr{ base, index, value };
                  base = mUnit.addExpr(node);
                  continue;
               }

               ast::ExprNode node;
               node.kind = ast::ExprKind::IndexAccess;
               node.span = spanOf(opToken);
               node.indexAccess = ast::IndexAccessExpr{ base, index };
               base = mUnit.addExpr(node);
               continue;
            }

            if (matchToken(TokenKind::PlusPlus) || matchToken(TokenKind::MinusMinus))
            {
               bool isIncrement = (mPrevious.kind == TokenKind::PlusPlus);
               ast::ExprNode node;
               node.kind = ast::ExprKind::PostIncDec;
               node.span = spanOf(opToken);
               node.postIncDec = ast::PostIncDecExpr{ isIncrement, base };
               base = mUnit.addExpr(node);
               continue;
            }

            return base;
         }
      }

      ast::ListHandle Parser::parseExprList(TokenKind terminator)
      {
         Vector<ast::ExprHandle> items;
         if (!check(terminator))
         {
            items.push_back(parseExpr(0));
            while (matchToken(TokenKind::Comma))
               items.push_back(parseExpr(0));
         }
         return mUnit.addExprList(items);
      }

      ast::ExprHandle Parser::parseObjectDecl(bool isDatablock, bool isSingleton)
      {
         Token startToken = mCurrent;
         advanceToken(); // consumes KwDeclare / KwDeclareSingleton / KwDatablock

         // class_name_expr (CMDgram.y:540-545): either a bare identifier,
         // read as a string constant naming the class (NOT a function call -
         // "new ScriptObject(...)" does not call a function named
         // ScriptObject), or a parenthesized "(expr)" for a
         // dynamically-computed class name. Not routed through the general
         // parseExpr/parsePrefix path: a bare identifier there is only ever
         // a function-call name, which is the wrong reading here.
         ast::ExprHandle classNameExpr;
         if (check(TokenKind::LParen))
         {
            advanceToken();
            classNameExpr = parseExpr(0);
            expect(TokenKind::RParen, "')' to close dynamic class-name expression");
         }
         else
         {
            // A registered class name lexes as TypeIdent (see lexer.cpp's
            // TypeNameLookup callback), not Ident - accept either. Matters
            // before a class is registered with HostBindingRegistry: uses of
            // its name lex as plain Ident until then.
            Token nameToken = mCurrent;
            StringTableEntry className;
            if (check(TokenKind::TypeIdent))
            {
               className = mCurrent.text;
               advanceToken();
            }
            else
            {
               className = expect(TokenKind::Ident, "class name").text;
            }
            ast::ExprNode node;
            node.kind = ast::ExprKind::StringLiteral;
            node.span = spanOf(nameToken);
            node.stringLiteral = ast::StringLiteralExpr{ className };
            classNameExpr = mUnit.addExpr(node);
         }

         expect(TokenKind::LParen, "'(' after class name in object declaration");

         bool isArrayElement = matchToken(TokenKind::LBracket);

         ast::ExprHandle objectNameExpr;
         if (check(TokenKind::RParen) || check(TokenKind::Colon) || check(TokenKind::Comma) ||
            (isArrayElement && check(TokenKind::RBracket)))
         {
            // Empty object-name position defaults to an empty string
            // literal, matching the old grammar's no-expr object_name rule.
            ast::ExprNode empty;
            empty.kind = ast::ExprKind::StringLiteral;
            empty.span = spanOf(mCurrent);
            empty.stringLiteral = ast::StringLiteralExpr{ StringTable->insert("") };
            objectNameExpr = mUnit.addExpr(empty);
         }
         else if (checkAny({ TokenKind::Ident, TokenKind::TypeIdent }) && !checkAny({ TokenKind::LParen }))
         {
            // A bare identifier in object-name position names the object as
            // a string, same reasoning as class_name_expr above. Only taken
            // when the identifier isn't itself the start of a call - a
            // genuine dynamic name still needs explicit parens, e.g.
            // "new Foo((getName()))", same as class_name_expr's own rule.
            // NOTE: this makes a missing-call-parens typo like
            // "new Foo(getName)" silently become the literal object name
            // "getName" rather than a parse error - inherent to the
            // grammar's ambiguity here, not a bug in this check.
            Token nameToken = mCurrent;
            StringTableEntry name = mCurrent.text;
            advanceToken();
            ast::ExprNode node;
            node.kind = ast::ExprKind::StringLiteral;
            node.span = spanOf(nameToken);
            node.stringLiteral = ast::StringLiteralExpr{ name };
            objectNameExpr = mUnit.addExpr(node);
         }
         else
         {
            objectNameExpr = parseExpr(0);
         }

         if (isArrayElement)
            expect(TokenKind::RBracket, "']' to close bracketed object name");

         StringTableEntry parentName = nullptr;
         if (matchToken(TokenKind::Colon))
            parentName = expect(TokenKind::Ident, "parent class name after ':'").text;

         ast::ListHandle constructorArgs;
         if (matchToken(TokenKind::Comma))
            constructorArgs = parseExprList(TokenKind::RParen);

         expect(TokenKind::RParen, "')' to close object declaration header");

         ast::ListHandle slotAssignments;
         ast::ListHandle childDecls;
         if (matchToken(TokenKind::LBrace))
         {
            Vector<ast::SlotAssignment> slots;
            Vector<ast::ExprHandle> children;

            while (!check(TokenKind::RBrace) && !check(TokenKind::Eof))
            {
               if (checkAny({ TokenKind::KwDeclare, TokenKind::KwDeclareSingleton, TokenKind::KwDatablock }))
               {
                  bool childIsSingleton = check(TokenKind::KwDeclareSingleton);
                  bool childIsDatablock = check(TokenKind::KwDatablock);
                  children.push_back(parseObjectDecl(childIsDatablock, childIsSingleton));
                  expect(TokenKind::Semicolon, "';' after nested object declaration");
                  continue;
               }

               StringTableEntry slotName = expect(TokenKind::Ident, "field name in object body").text;

               // Array-style slot assignment (field[index] = value;) reuses
               // ordinary IndexAssign against a synthesized FieldAccess base,
               // rather than needing a third SlotAssignment shape.
               if (check(TokenKind::LBracket))
               {
                  ast::ExprNode fieldNode;
                  fieldNode.kind = ast::ExprKind::FieldAccess;
                  fieldNode.span = spanOf(mCurrent);
                  fieldNode.fieldAccess = ast::FieldAccessExpr{ ast::ExprHandle{}, slotName, false };
                  ast::ExprHandle fieldHandle = mUnit.addExpr(fieldNode);
                  ast::ExprHandle indexedAssign = parsePostfixChain(fieldHandle);
                  expect(TokenKind::Semicolon, "';' after field assignment");

                  ast::SlotAssignment slot;
                  slot.slotName = slotName;
                  slot.value = indexedAssign;
                  slots.push_back(slot);
                  continue;
               }

               expect(TokenKind::Assign, "'=' in field assignment");
               ast::ExprHandle value = parseExpr(0);
               expect(TokenKind::Semicolon, "';' after field assignment");

               ast::SlotAssignment slot;
               slot.slotName = slotName;
               slot.value = value;
               slots.push_back(slot);
            }
            expect(TokenKind::RBrace, "'}' to close object declaration body");

            slotAssignments = mUnit.addSlotList(slots);
            childDecls = mUnit.addExprList(children);
         }

         ast::ExprNode node;
         node.kind = ast::ExprKind::ObjectDecl;
         node.span = spanOf(startToken);
         node.objectDecl = ast::ObjectDeclExpr{
            classNameExpr, objectNameExpr, parentName, constructorArgs,
            slotAssignments, childDecls, isDatablock, isArrayElement, isSingleton
         };
         return mUnit.addExpr(node);
      }

      // =============================================================================
      // Statements
      // =============================================================================

      CompilationUnit Parser::parse()
      {
         mUnit.originName = mOriginName;
         Vector<ast::StmtHandle> top;
         while (!check(TokenKind::Eof))
            top.push_back(parseStatement());
         mUnit.topLevel = mUnit.addStmtList(top);
         return std::move(mUnit);
      }

      ast::ListHandle Parser::parseStatementList(TokenKind terminator)
      {
         Vector<ast::StmtHandle> items;
         while (!check(terminator) && !check(TokenKind::Eof))
            items.push_back(parseStatement());
         return mUnit.addStmtList(items);
      }

      ast::StmtHandle Parser::parseBlockStatement()
      {
         Token startToken = mCurrent;
         expect(TokenKind::LBrace, "'{'");
         ast::ListHandle statements = parseStatementList(TokenKind::RBrace);
         expect(TokenKind::RBrace, "'}' to close block");

         ast::StmtNode node;
         node.kind = ast::StmtKind::Block;
         node.span = spanOf(startToken);
         node.block = ast::BlockStmt{ statements };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseStatement()
      {
         switch (mCurrent.kind)
         {
         case TokenKind::LBrace:        return parseBlockStatement();
         case TokenKind::KwIf:          return parseIfStatement();
         case TokenKind::KwWhile:       return parseWhileStatement();
         case TokenKind::KwDo:          return parseDoWhileStatement();
         case TokenKind::KwFor:         return parseForStatement();
         case TokenKind::KwForeach:     return parseForeachStatement(false);
         case TokenKind::KwForeachStr:  return parseForeachStatement(true);
         case TokenKind::KwSwitch:      return parseSwitchStatement(false);
         case TokenKind::KwSwitchStr:   return parseSwitchStatement(true);
         case TokenKind::KwBreak:       return parseBreakStatement();
         case TokenKind::KwContinue:    return parseContinueStatement();
         case TokenKind::KwReturn:      return parseReturnStatement();
         case TokenKind::KwDefine:      return parseFunctionDeclStatement();
         case TokenKind::KwPackage:     return parsePackageDeclStatement();
         case TokenKind::KwDatablock:
         case TokenKind::KwDeclare:
         case TokenKind::KwDeclareSingleton:
            return parseExprStatement(); // object decls are expressions; wrapped as an expr-stmt here
         default:
            return parseExprStatement();
         }
      }

      ast::StmtHandle Parser::parseIfStatement()
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::LParen, "'(' after 'if'");
         ast::ExprHandle condition = parseExpr(0);
         expect(TokenKind::RParen, "')' after if condition");
         ast::StmtHandle thenBranch = parseStatement();

         ast::StmtHandle elseBranch;
         if (matchToken(TokenKind::KwElse))
            elseBranch = parseStatement();

         ast::StmtNode node;
         node.kind = ast::StmtKind::If;
         node.span = spanOf(startToken);
         node.ifStmt = ast::IfStmt{ condition, thenBranch, elseBranch };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseWhileStatement()
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::LParen, "'(' after 'while'");
         ast::ExprHandle condition = parseExpr(0);
         expect(TokenKind::RParen, "')' after while condition");
         ast::StmtHandle body = parseStatement();

         ast::StmtNode node;
         node.kind = ast::StmtKind::While;
         node.span = spanOf(startToken);
         node.whileStmt = ast::WhileStmt{ condition, body };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseDoWhileStatement()
      {
         Token startToken = mCurrent;
         advanceToken(); // consumes 'do'
         ast::StmtHandle body = parseStatement();
         expect(TokenKind::KwWhile, "'while' after do-block");
         expect(TokenKind::LParen, "'(' after 'while'");
         ast::ExprHandle condition = parseExpr(0);
         expect(TokenKind::RParen, "')' after while condition");
         expect(TokenKind::Semicolon, "';' after do-while statement");

         ast::StmtNode node;
         node.kind = ast::StmtKind::DoWhile;
         node.span = spanOf(startToken);
         node.doWhileStmt = ast::DoWhileStmt{ body, condition };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseForStatement()
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::LParen, "'(' after 'for'");

         ast::StmtHandle init;
         if (!check(TokenKind::Semicolon))
            init = parseExprStatement(); // consumes its own trailing ';'
         else
            advanceToken();

         ast::ExprHandle condition;
         if (!check(TokenKind::Semicolon))
            condition = parseExpr(0);
         expect(TokenKind::Semicolon, "';' after for-loop condition");

         ast::ExprHandle increment;
         if (!check(TokenKind::RParen))
            increment = parseExpr(0);
         expect(TokenKind::RParen, "')' after for-loop clauses");

         ast::StmtHandle body = parseStatement();

         ast::StmtNode node;
         node.kind = ast::StmtKind::For;
         node.span = spanOf(startToken);
         node.forStmt = ast::ForStmt{ init, condition, increment, body };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseForeachStatement(bool isStringForm)
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::LParen, "'(' after 'foreach'");
         StringTableEntry loopVar = expect(TokenKind::Var, "loop variable").text;
         expect(TokenKind::KwIn, "'in' in foreach statement");
         ast::ExprHandle collection = parseExpr(0);
         expect(TokenKind::RParen, "')' after foreach clauses");
         ast::StmtHandle body = parseStatement();

         ast::StmtNode node;
         node.kind = ast::StmtKind::Foreach;
         node.span = spanOf(startToken);
         node.foreachStmt = ast::ForeachStmt{ loopVar, collection, body, isStringForm };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseSwitchStatement(bool isStringForm)
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::LParen, "'(' after 'switch'");
         ast::ExprHandle subject = parseExpr(0);
         expect(TokenKind::RParen, "')' after switch subject");
         expect(TokenKind::LBrace, "'{' to open switch body");

         Vector<ast::SwitchCase> cases;
         Vector<ast::StmtHandle> defaultBody;

         while (check(TokenKind::KwCase))
         {
            advanceToken();
            Vector<ast::ExprHandle> values;
            values.push_back(parseExpr(0));
            while (matchToken(TokenKind::KwCaseOr))
               values.push_back(parseExpr(0));
            expect(TokenKind::Colon, "':' after case value(s)");

            Vector<ast::StmtHandle> body;
            while (!checkAny({ TokenKind::KwCase, TokenKind::KwDefault, TokenKind::RBrace }) && !check(TokenKind::Eof))
               body.push_back(parseStatement());

            ast::SwitchCase c;
            c.valueList = mUnit.addExprList(values);
            c.body = mUnit.addStmtList(body);
            cases.push_back(c);
         }

         if (matchToken(TokenKind::KwDefault))
         {
            expect(TokenKind::Colon, "':' after 'default'");
            while (!check(TokenKind::RBrace) && !check(TokenKind::Eof))
               defaultBody.push_back(parseStatement());
         }

         expect(TokenKind::RBrace, "'}' to close switch body");

         ast::StmtNode node;
         node.kind = ast::StmtKind::Switch;
         node.span = spanOf(startToken);
         node.switchStmt = ast::SwitchStmt{ subject, mUnit.addCaseList(cases), mUnit.addStmtList(defaultBody), isStringForm };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseBreakStatement()
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::Semicolon, "';' after 'break'");
         ast::StmtNode node;
         node.kind = ast::StmtKind::Break;
         node.span = spanOf(startToken);
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseContinueStatement()
      {
         Token startToken = mCurrent;
         advanceToken();
         expect(TokenKind::Semicolon, "';' after 'continue'");
         ast::StmtNode node;
         node.kind = ast::StmtKind::Continue;
         node.span = spanOf(startToken);
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseReturnStatement()
      {
         Token startToken = mCurrent;
         advanceToken();
         ast::ExprHandle value;
         if (!check(TokenKind::Semicolon))
            value = parseExpr(0);
         expect(TokenKind::Semicolon, "';' after return statement");

         ast::StmtNode node;
         node.kind = ast::StmtKind::Return;
         node.span = spanOf(startToken);
         node.returnStmt = ast::ReturnStmt{ value };
         return mUnit.addStmt(node);
      }

      ast::Param Parser::parseParam()
      {
         StringTableEntry name = expect(TokenKind::Var, "parameter name").text;
         bool hasDefault = false;
         ast::ExprHandle defaultValue;
         // '?' (optional-no-default) and '= expr' both collapse to
         // hasDefault based on whether an actual default was supplied - see
         // astStmts.h's Param comment.
         matchToken(TokenKind::Question);
         if (matchToken(TokenKind::Assign))
         {
            defaultValue = parseExpr(0);
            hasDefault = true;
         }
         return ast::Param{ name, defaultValue, hasDefault };
      }

      ast::StmtHandle Parser::parseFunctionDeclStatement()
      {
         Token startToken = mCurrent;
         advanceToken(); // consumes 'function'

         // Namespace-position name may be a registered class name (e.g.
         // "function ConstructWidget::method(...)"), lexing as TypeIdent -
         // same as parseObjectDecl's class-name check.
         StringTableEntry firstName;
         if (check(TokenKind::TypeIdent))
         {
            firstName = mCurrent.text;
            advanceToken();
         }
         else
         {
            firstName = expect(TokenKind::Ident, "function name").text;
         }
         StringTableEntry namespaceName = nullptr;
         StringTableEntry functionName = firstName;
         if (matchToken(TokenKind::ColonColon))
         {
            namespaceName = firstName;
            functionName = expect(TokenKind::Ident, "function name after '::'").text;
         }

         expect(TokenKind::LParen, "'(' after function name");
         Vector<ast::Param> params;
         if (!check(TokenKind::RParen))
         {
            params.push_back(parseParam());
            while (matchToken(TokenKind::Comma))
               params.push_back(parseParam());
         }
         expect(TokenKind::RParen, "')' after parameter list");

         expect(TokenKind::LBrace, "'{' to open function body");
         ast::ListHandle body = parseStatementList(TokenKind::RBrace);
         expect(TokenKind::RBrace, "'}' to close function body");

         ast::StmtNode node;
         node.kind = ast::StmtKind::FunctionDecl;
         node.span = spanOf(startToken);
         node.functionDecl = ast::FunctionDeclStmt{ namespaceName, functionName, mUnit.addParamList(params), body };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parsePackageDeclStatement()
      {
         Token startToken = mCurrent;
         advanceToken(); // consumes 'package'
         StringTableEntry packageName = expect(TokenKind::Ident, "package name").text;
         expect(TokenKind::LBrace, "'{' to open package body");

         Vector<ast::StmtHandle> decls;
         while (!check(TokenKind::RBrace) && !check(TokenKind::Eof))
         {
            if (!check(TokenKind::KwDefine))
            {
               reportError(mCurrent, "package body may only contain function declarations");
               synchronizeToStatementBoundary();
               continue;
            }
            decls.push_back(parseFunctionDeclStatement());
         }
         expect(TokenKind::RBrace, "'}' to close package body");
         expect(TokenKind::Semicolon, "';' after package declaration");

         ast::StmtNode node;
         node.kind = ast::StmtKind::PackageDecl;
         node.span = spanOf(startToken);
         node.packageDecl = ast::PackageDeclStmt{ packageName, mUnit.addStmtList(decls) };
         return mUnit.addStmt(node);
      }

      ast::StmtHandle Parser::parseExprStatement()
      {
         Token startToken = mCurrent;

         if (check(TokenKind::DocBlock))
         {
            // A doc-block at statement position, not attached to a following
            // function/object decl - wrapped as an expr-stmt around a
            // DocComment expression so it still round-trips through the tree.
            ast::ExprHandle expr = parsePrefix();
            ast::StmtNode node;
            node.kind = ast::StmtKind::ExprStmt;
            node.span = spanOf(startToken);
            node.exprStmt = ast::ExprStmt{ expr };
            return mUnit.addStmt(node);
         }

         ast::ExprHandle expr = parseExpr(0);
         if (!matchToken(TokenKind::Semicolon))
         {
            reportError(mCurrent, "expected ';' after expression statement");
            synchronizeToStatementBoundary();
         }

         ast::StmtNode node;
         node.kind = ast::StmtKind::ExprStmt;
         node.span = spanOf(startToken);
         node.exprStmt = ast::ExprStmt{ expr };
         return mUnit.addStmt(node);
      }

   } // namespace ts2
} // namespace newConsole
