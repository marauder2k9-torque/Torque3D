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
/// Design rules for anyone extending this file:
///  1. One struct per distinct *meaning*, not one struct per grammar
///     production. If two grammar rules produce the same semantic thing
///     (e.g. `new X(...)` and `datablock X(...)` are both "declare an
///     object"), they share one node kind with fields that distinguish
///     them - readable at a glance, not desugared into a shape that only
///     makes sense if you already know the trick (the old TorqueScript
///     grammar desugared `switch` into nested if-statements at parse
///     time; that shape is NOT carried forward here - see SwitchStmt).
///  2. Every node kind gets its own named struct, not a generic
///     "Node { kind; union of everything }" blob. A reader grepping for
///     `struct WhileStmt` should find exactly the fields a while loop
///     needs, nothing else.
///  3. Nodes are referenced by Handle (an index into Module's arrays),
///     never by pointer. This keeps the whole AST relocatable, keeps
///     memory contiguous (cache-friendly to walk, and cheap to discard
///     wholesale once bytecode has been emitted - the AST is transient,
///     built fresh per compile and thrown away afterward), and makes
///     "does this handle point at something real" a bounds check instead
///     of a null check.
///  4. Every node carries a SourceSpan. This is not optional decoration -
///     it is what lets a debugger answer "what expression is executing
///     right now" richly (see the project's hot-reload/debugging design),
///     not just "what line". Emit code always keyed off node identity,
///     not just line numbers, so a breakpoint or step operation can point
///     at a real semantic unit.

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
/// slot-assignment lists, case-value lists, ...). Never dereferenced
/// directly - always resolved through Module's list-arena accessors.
struct ListHandle
{
   U32 index = ~0u;
   U32 count = 0;
   bool isEmpty() const { return count == 0; }
};

/// Where in source this node came from. Carried by every node - see rule
/// 4 above. originIndex identifies which file/frontend produced this
/// node in a multi-file compile (module-local index into a source table
/// the compiler owns, not the AST itself).
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

/// One binary/compound-assign/comparison operator. Deliberately not
/// reusing TokenKind directly for this - only a subset of TokenKind
/// values are valid operators here, and a reader of this enum should not
/// have to cross-reference the full token list to know what's legal.
enum class BinaryOp : U8
{
   Add, Sub, Mul, Div, Mod,
   Shl, Shr,
   BitAnd, BitOr, BitXor,
   LogicalAnd, LogicalOr,
   Eq, Ne, Lt, Le, Gt, Ge,
};

enum class UnaryOp : U8 { Negate, LogicalNot, BitNot };

struct IntLiteralExpr    { S64 value; };
struct FloatLiteralExpr  { F64 value; };
struct StringLiteralExpr { StringTableEntry text; };
/// 'quoted' literal - a tagged string. Ordinary literal syntax; the
/// "tagged" behavior (transmitted once over the network, referenced
/// afterward by a small integer tag, read back with detag()) is a
/// runtime/value-type concern handled by whatever ScriptValue variant or
/// NetStringHandle-equivalent this compiles to - not a parser or AST
/// concern. Assigned with ordinary '=' like any other value
/// ($a = 'text';); there is no dedicated assignment statement for it.
struct TaggedLiteralExpr { StringTableEntry text; };

/// $Name or $Namespace::Name. The sigil is not stored separately - name
/// is the interned text exactly as the lexer produced it (sigil
/// included), matching how the lexer already treats Var as one token
/// regardless of which sigil introduced it (see tokenKinds.inc).
struct GlobalVarExpr { StringTableEntry name; };
struct LocalVarExpr  { StringTableEntry name; };

struct AssignExpr         { ExprHandle target; ExprHandle value; };
struct CompoundAssignExpr { ExprHandle target; BinaryOp op; ExprHandle value; };
struct BinaryExpr         { BinaryOp op; ExprHandle lhs; ExprHandle rhs; };

/// separatorChar is 0 for plain '@', or '\n'/'\t'/' ' for the NL/TAB/SPC
/// forms - see tokenKinds.inc's note on why these collapse to one lexer
/// token; they get their own dedicated expression fields here (not a
/// generic "extra int payload" on BinaryExpr) so a reader sees at a
/// glance this is string concatenation, not arithmetic.
struct StringConcatExpr { ExprHandle lhs; ExprHandle rhs; char separatorChar; };

struct StringCompareExpr { bool negated; ExprHandle lhs; ExprHandle rhs; };
struct UnaryExpr         { UnaryOp op; ExprHandle operand; };
struct PreIncDecExpr     { bool isIncrement; ExprHandle target; };
struct PostIncDecExpr    { bool isIncrement; ExprHandle target; };
struct TernaryExpr       { ExprHandle cond; ExprHandle whenTrue; ExprHandle whenFalse; };

/// object.field or object->field. isInternal distinguishes the two - a
/// separate bool rather than a separate node kind, since every other
/// aspect of the access (get/set, argument shape) is identical between
/// them; only the resolution rule at the object model layer differs.
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
/// One node kind for all three declaration forms (new/datablock/
/// singleton), same reasoning as the file header comment: they are the
/// same underlying operation (declare an object) with different flags,
/// not three different meanings.
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

/// One expression node. A tagged union in layout only - readers should
/// treat the individual *Expr structs above as the real interface; this
/// struct exists purely so Module can store a homogeneous array.
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

   // Non-trivial members (none currently - all fields above are POD/
   // trivially-copyable: StringTableEntry is a raw interned pointer,
   // ExprHandle/ListHandle are plain index structs). If a future node
   // kind needs a non-trivial member, this union stops being legal and
   // ExprNode needs to become variant-based instead - flagged here so
   // that constraint stays visible rather than silently broken.
   ExprNode() : kind(ExprKind::IntLiteral), intLiteral{0} {}
};

} // namespace ast
} // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_ASTNODES_H_
