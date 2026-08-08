#ifndef _NEWCONSOLE_TS2_ASTSTMTS_H_
#define _NEWCONSOLE_TS2_ASTSTMTS_H_

#ifndef _NEWCONSOLE_TS2_ASTNODES_H_
#include "newConsole/torquescript2/astNodes.h"
#endif

namespace newConsole
{
namespace ts2
{
namespace ast
{

// =============================================================================
// Statements
// =============================================================================

enum class StmtKind : U8
{
   ExprStmt,       // any expression used as a statement (assignment, call, ++, ...)
   Block,          // { statement* }
   If,
   While,
   DoWhile,
   For,
   Foreach,
   Switch,
   Break,
   Continue,
   Return,
   FunctionDecl,
   PackageDecl,
};

struct ExprStmt { ExprHandle expr; };

/// { statement* } - a plain brace-delimited block. Not implicit in every
/// place a statement is expected the way the old grammar's stmt_block
/// production folded "{ list }" and "single stmt" into one rule; here a
/// single bare statement (e.g. `if (x) doThing();`) is just that
/// statement's own node, not wrapped in a Block - Block only exists
/// where source actually wrote braces. Keeps the tree's shape a direct
/// reflection of what was written, which is what a reader/tool
/// (formatter, debugger stepping) wants.
struct BlockStmt { ListHandle statements; };

struct IfStmt
{
   ExprHandle condition;
   StmtHandle thenBranch;
   StmtHandle elseBranch; // invalid handle (isValid() == false) if no else
};

struct WhileStmt { ExprHandle condition; StmtHandle body; };
struct DoWhileStmt { StmtHandle body; ExprHandle condition; };

struct ForStmt
{
   StmtHandle init;      // invalid handle if the init clause was empty
   ExprHandle condition;  // invalid handle if the condition clause was empty
   ExprHandle increment;  // invalid handle if the increment clause was empty
   StmtHandle body;
};

/// foreach(%var in %collection) body   or   foreach$(%var in %collectionExpr) body
/// isStringForm distinguishes foreach$ (iterates string-tokenized
/// elements) from foreach (iterates a SimSet/object collection) - kept
/// as a bool rather than a second node kind since the loop shape and
/// every other field are identical; only what "iterate" means differs,
/// which is a codegen concern, not a structural one.
struct ForeachStmt
{
   StringTableEntry loopVar;
   ExprHandle collection;
   StmtHandle body;
   bool isStringForm;
};

/// One `case value[, orValue...]: statements` arm. valueList holds one
/// or more ExprHandles - `case 1 or 2 or 3:` is one arm with three
/// values, not three arms, matching how a reader would describe it in
/// English ("case 1, 2, or 3").
struct SwitchCase
{
   ListHandle valueList;   // ListHandle over ExprHandle
   ListHandle body;        // ListHandle over StmtHandle
};

/// switch(expr) { case ...: ...; default: ...; } / switch$(...) { ... }
///
/// Gets its own real node - deliberately NOT desugared into nested
/// if-statements the way the old grammar's parser did. That desugaring
/// was a workaround for not having a dedicated node, came with a
/// documented bug (recursion depth proportional to case count, able to
/// overflow the compiler's own stack for 100+ cases), and it made the
/// tree lie about what the source actually said - anyone reading a
/// desugared if-chain has to recognize the pattern before realizing
/// "oh, this is a switch". A dedicated node is more readable, lets
/// codegen choose a jump table or a plain compare chain as a backend
/// decision, and has no case-count-dependent stack risk at compile time.
struct SwitchStmt
{
   ExprHandle subject;
   ListHandle cases;         // ListHandle over SwitchCase
   ListHandle defaultBody;   // ListHandle over StmtHandle; empty list if no default
   bool isStringForm;        // switch$ - string comparison instead of numeric
};

struct BreakStmt {};
struct ContinueStmt {};
struct ReturnStmt { ExprHandle value; }; // invalid handle for a bare `return;`

/// One function parameter. hasDefault distinguishes a required parameter
/// from one with a default expression - the old grammar's `%var ?` form
/// (optional-but-no-default) collapses to hasDefault == false with no
/// separate marker, since at the object-model level "optional, defaults
/// to empty/zero" and "required" behave identically until a default
/// expression is actually supplied; see this file's header note if a
/// future change needs to distinguish them again.
struct Param
{
   StringTableEntry name;
   ExprHandle defaultValue; // invalid handle if hasDefault == false
   bool hasDefault;
};

struct FunctionDeclStmt
{
   StringTableEntry namespaceName; // may be null - plain global function
   StringTableEntry functionName;
   ListHandle params;              // ListHandle over Param
   ListHandle body;                // ListHandle over StmtHandle
};

/// package Name { function ...; function ...; }
/// body holds only FunctionDecl statements - enforced by the parser, not
/// by this struct's shape (a package containing something other than
/// function decls is a parse error, not a different node kind).
struct PackageDeclStmt
{
   StringTableEntry packageName;
   ListHandle functionDecls; // ListHandle over StmtHandle, each a FunctionDeclStmt
};

struct StmtNode
{
   StmtKind kind;
   SourceSpan span;
   union
   {
      ExprStmt exprStmt;
      BlockStmt block;
      IfStmt ifStmt;
      WhileStmt whileStmt;
      DoWhileStmt doWhileStmt;
      ForStmt forStmt;
      ForeachStmt foreachStmt;
      SwitchStmt switchStmt;
      BreakStmt breakStmt;
      ContinueStmt continueStmt;
      ReturnStmt returnStmt;
      FunctionDeclStmt functionDecl;
      PackageDeclStmt packageDecl;
   };

   StmtNode() : kind(StmtKind::ExprStmt), exprStmt{ExprHandle{}} {}
};

} // namespace ast
} // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_ASTSTMTS_H_
