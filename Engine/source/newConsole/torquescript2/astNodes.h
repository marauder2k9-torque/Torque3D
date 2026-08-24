#ifndef _NEWCONSOLE_TS2_ASTNODES_H_
#define _NEWCONSOLE_TS2_ASTNODES_H_

#ifndef _NEWCONSOLE_TS2_TOKENKINDS_H_
#include "newConsole/torquescript2/tokenKinds.h"
#endif

namespace newConsole
{
   namespace ts2
   {
      namespace ast
      {

         /// @file
         /// AST node set for torquescript2.
         ///
         /// Rules for extending this file:
         ///  1. One struct per distinct meaning, not per grammar production. If
         ///     two grammar rules mean the same thing (`new X(...)` and
         ///     `datablock X(...)` both declare an object), share one node kind
         ///     with distinguishing fields - not desugared into a different shape
         ///     (e.g. `switch` is NOT desugared into nested ifs - see SwitchStmt).
         ///  2. Every node kind is its own named struct, not a generic "Node {
         ///     kind; union of everything }" blob.
         ///  3. Nodes are referenced by Handle (index into Module's arrays), never
         ///     by pointer - keeps the AST relocatable, contiguous, cheap to
         ///     discard after compile, and makes validity a bounds check.
         ///  4. Every node carries a SourceSpan, so a debugger can resolve to a
         ///     real expression, not just a line - see rule 4's use in emit code.

         /// Opaque reference to one expression node, indexing into Module::exprs.
         struct ExprHandle
         {
            U32 index = ~0u;
            bool isValid() const { return index != ~0u; }
            bool operator==(ExprHandle o) const { return index == o.index; }
         };

         /// Opaque reference to one statement node, indexing into Module::stmts.
         struct StmtHandle
         {
            U32 index = ~0u;
            bool isValid() const { return index != ~0u; }
            bool operator==(StmtHandle o) const { return index == o.index; }
         };

         /// Opaque reference into a variable-length list arena (argument lists,
         /// slot-assignment lists, case-value lists, ...). Always resolved
         /// through Module's list-arena accessors, never dereferenced directly.
         struct ListHandle
         {
            U32 index = ~0u;
            U32 count = 0;
            bool isEmpty() const { return count == 0; }
         };

         /// Where in source this node came from. originIndex identifies which
         /// file/frontend produced it in a multi-file compile (index into a
         /// source table the compiler owns, not the AST).
         struct SourceSpan
         {
            U32 originIndex = 0;
            U32 line = 0;
            U32 column = 0;
         };

         // =============================================================================
         // Expressions
         // =============================================================================

         enum class ExprKind : U8
         {
            IntLiteral,
            FloatLiteral,
            StringLiteral,     // "..." - StrAtom
            TaggedLiteral,      // '...' - TagAtom, the networked tagged-string form
            GlobalVar,          // $Name or $Namespace::Name
            LocalVar,           // %name
            Assign,             // lhs = rhs  (lhs is itself an ExprHandle: GlobalVar/LocalVar/FieldAccess/Index)
            CompoundAssign,     // lhs OP= rhs  (+=, -=, etc.)
            Binary,             // lhs OP rhs  (arithmetic, comparison, logical, bitwise)
            StringConcat,       // lhs @ rhs, with an optional separator char (see tokenKinds.h's At payload)
            StringCompare,      // lhs $= rhs / lhs !$= rhs
            Unary,              // OP operand  (-x, !x, ~x)
            PreIncDec,          // ++x / --x
            PostIncDec,         // x++ / x--
            Ternary,            // cond ? a : b
            FieldAccess,        // object.field  or  object->field (internal name access)
            FieldAssign,        // object.field = value
            IndexAccess,        // array[index]  (also covers %var[index] indexed-local access)
            IndexAssign,        // array[index] = value
            Call,               // bareword function call: name(args...) or Namespace::name(args...)
            MethodCall,         // object.method(args...)
            ObjectDecl,         // new/datablock/singleton ClassName(...) { ... } - see ObjectDeclExpr
            DocComment,         // a floating ///-doc-block that appears where an expression/statement is expected
         };

         /// One binary/compound-assign/comparison operator. Not TokenKind
         /// directly - only a subset is valid here, and this keeps that subset
         /// visible without cross-referencing the full token list.
         enum class BinaryOp : U8
         {
            Add, Sub, Mul, Div, Mod,
            Shl, Shr,
            BitAnd, BitOr, BitXor,
            LogicalAnd, LogicalOr,
            Eq, Ne, Lt, Le, Gt, Ge,
         };

         enum class UnaryOp : U8 { Negate, LogicalNot, BitNot };

         struct IntLiteralExpr { S64 value; };
         struct FloatLiteralExpr { F64 value; };
         struct StringLiteralExpr { StringTableEntry text; };
         /// 'quoted' literal - a tagged string. Plain literal syntax; the
         /// "tagged" runtime behavior (sent once, referenced by a small int tag)
         /// is a value-type concern, not a parser/AST one. Assigned with '='
         /// like any other value; no dedicated assignment statement.
         struct TaggedLiteralExpr { StringTableEntry text; };

         /// $Name or $Namespace::Name. name is the interned text including the
         /// sigil, exactly as the lexer produced it (see tokenKinds.inc).
         struct GlobalVarExpr { StringTableEntry name; };
         struct LocalVarExpr { StringTableEntry name; };

         struct AssignExpr { ExprHandle target; ExprHandle value; };
         struct CompoundAssignExpr { ExprHandle target; BinaryOp op; ExprHandle value; };
         struct BinaryExpr { BinaryOp op; ExprHandle lhs; ExprHandle rhs; };

         /// separatorChar is 0 for plain '@', or '\n'/'\t'/' ' for the NL/TAB/SPC
         /// forms (see tokenKinds.inc). Own dedicated fields, not a generic
         /// payload on BinaryExpr, so this reads as string concat at a glance.
         struct StringConcatExpr { ExprHandle lhs; ExprHandle rhs; char separatorChar; };

         struct StringCompareExpr { bool negated; ExprHandle lhs; ExprHandle rhs; };
         struct UnaryExpr { UnaryOp op; ExprHandle operand; };
         struct PreIncDecExpr { bool isIncrement; ExprHandle target; };
         struct PostIncDecExpr { bool isIncrement; ExprHandle target; };
         struct TernaryExpr { ExprHandle cond; ExprHandle whenTrue; ExprHandle whenFalse; };

         /// object.field or object->field. isInternal is a bool flag, not a
         /// separate node kind, since only the object-model resolution differs.
         struct FieldAccessExpr { ExprHandle object; StringTableEntry field; bool isInternal; };
         struct FieldAssignExpr { ExprHandle object; StringTableEntry field; bool isInternal; ExprHandle value; };

         struct IndexAccessExpr { ExprHandle base; ExprHandle index; };
         struct IndexAssignExpr { ExprHandle base; ExprHandle index; ExprHandle value; };

         struct CallExpr
         {
            StringTableEntry namespaceName; // may be null - bareword call, no Namespace::
            StringTableEntry functionName;
            ListHandle args;
         };
         struct MethodCallExpr { ExprHandle object; StringTableEntry methodName; ListHandle args; };

         /// One `field = expr;` line inside an object declaration body.
         struct SlotAssignment { StringTableEntry slotName; ExprHandle value; };

         /// new/datablock/singleton ClassName(nameExpr : parentIdent, args...) { slots...; children...; }
         ///
         /// One node kind covers all three declaration forms - same operation
         /// (declare an object) with different flags, not different meanings.
         struct ObjectDeclExpr
         {
            ExprHandle classNameExpr;      // may itself be dynamic (an expression yielding a class name string)
            ExprHandle objectNameExpr;     // may be an empty-string literal if unnamed
            StringTableEntry parentName;   // may be null - no ": Parent" clause
            ListHandle constructorArgs;    // the ", expr, expr" args after the name/parent
            ListHandle slotAssignments;    // ListHandle over SlotAssignment, not ExprHandle
            ListHandle childDecls;         // ListHandle over ExprHandle, each itself an ObjectDeclExpr
            bool isDatablock;
            bool isArrayElement;           // the "[ name ]" bracketed form
            bool isSingleton;
         };

         struct DocCommentExpr { StringTableEntry text; };

         /// One expression node. Tagged union in layout only - the individual
         /// *Expr structs above are the real interface; this just lets Module
         /// store a homogeneous array.
         struct ExprNode
         {
            ExprKind kind;
            SourceSpan span;
            union
            {
               IntLiteralExpr intLiteral;
               FloatLiteralExpr floatLiteral;
               StringLiteralExpr stringLiteral;
               TaggedLiteralExpr taggedLiteral;
               GlobalVarExpr globalVar;
               LocalVarExpr localVar;
               AssignExpr assign;
               CompoundAssignExpr compoundAssign;
               BinaryExpr binary;
               StringConcatExpr stringConcat;
               StringCompareExpr stringCompare;
               UnaryExpr unary;
               PreIncDecExpr preIncDec;
               PostIncDecExpr postIncDec;
               TernaryExpr ternary;
               FieldAccessExpr fieldAccess;
               FieldAssignExpr fieldAssign;
               IndexAccessExpr indexAccess;
               IndexAssignExpr indexAssign;
               CallExpr call;
               MethodCallExpr methodCall;
               ObjectDeclExpr objectDecl;
               DocCommentExpr docComment;
            };

            // All fields above are POD/trivially-copyable, so the union is legal.
            // A future non-trivial field would break that - flagged here so the
            // constraint stays visible.
            ExprNode() : kind(ExprKind::IntLiteral), intLiteral{ 0 } {}
         };

      } // namespace ast
   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_ASTNODES_H_
