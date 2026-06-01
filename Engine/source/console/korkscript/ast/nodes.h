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

#include "../korkTypes.h"
#include "../Diagnostics.h"    // SourceRange
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif

namespace KorkScript
{
   //---------------------------------------------------------------------------
   // Node kind tag — one entry per concrete node type.
   // CodeGen switches on this.
   //---------------------------------------------------------------------------
   enum class NodeKind : U32
   {
      // Statements
      StmtBlock,
      FunctionDecl,
      Return,
      Break,
      Continue,
      If,
      Loop,          // for / while / do-while
      Foreach,
      Switch,
      ExprStmt,
      TTagSet,
      DocBlock,
      ObjectDecl,
      DatablockDecl,
      PackageDecl,

      // Expressions
      IntLit,
      FloatLit,
      StrLit,
      TagLit,
      Const,         // identifier constant (e.g. enum value or class name)
      Var,           // %local or $global
      Assign,        // %var = expr
      AssignOp,      // %var += expr  etc.
      BinaryFloat,   // + - * /
      BinaryInt,     // & | ^ % << >> == != < > <= >= && ||
      UnaryInt,      // ! ~
      UnaryFloat,    // - (negate)
      Ternary,       // ? :
      StrCat,        // @ SPC TAB NL
      StrEq,         // $= !$=
      CommaCat,      // array index comma concatenation
      FuncCall,
      SlotAccess,    // expr.slot [ [idx] ]
      SlotAssign,
      SlotAssignOp,
      InternalSlot,  // expr -> slot  or  expr --> slot
      TTagDeref,     // *expr
      TTagExpr,      // %0 style tag
      Assert,
   };

   //---------------------------------------------------------------------------
   // Base node - every node derives from this
   //---------------------------------------------------------------------------
   struct Node
   {
      NodeKind    kind;
      S32         line;      // 1-based source line
      SourceRange range;     // full source range for debugger
      Node* next = nullptr;  // sibling list

      virtual ~Node() = default;

      Node() = default;
      Node(NodeKind k, S32 ln) : kind(k), line(ln) {}

   };

   // Convenience — most callers treat stmt/expr chains uniformly
   using StmtNode = Node;
   using ExprNode = Node;

   //---------------------------------------------------------------------------
   // Helper functions
   //---------------------------------------------------------------------------
   inline void nodeAppend(Node*& list, Node* node)
   {
      if (!list) { list = node; return; }
      Node* w = list; while (w->next) w = w->next; w->next = node;
   }

   //---------------------------------------------------------------------------
   // Literals nodes
   //---------------------------------------------------------------------------
   struct IntLitNode : Node
   {
      S32 value;
      IntLitNode(S32 ln, S32 v) : Node(NodeKind::IntLit, ln), value(v) {}
   };

   struct FloatLitNode : Node
   {
      F64 value;
      FloatLitNode(S32 ln, F64 v) : Node(NodeKind::FloatLit, ln), value(v) {}
   };

   struct StrLitNode : Node
   {
      const char* str;   // arena-allocated
      bool        tag;   // true = tag string ('')
      bool        doc;   // true = doc block (///)
      StrLitNode(S32 ln, const char* s, bool t, bool d = false)
         : Node(d ? NodeKind::DocBlock : (t ? NodeKind::TagLit : NodeKind::StrLit), ln)
         , str(s), tag(t), doc(d) {
      }
   };

   struct ConstNode : Node   // bare identifier — enum value / class name etc.
   {
      StringTableEntry value;
      ConstNode(S32 ln, StringTableEntry v) : Node(NodeKind::Const, ln), value(v) {}
   };

   //---------------------------------------------------------------------------
   // Var access nodes
   //---------------------------------------------------------------------------
   struct VarNode : Node
   {
      StringTableEntry name;       // includes sigil: %foo or $foo
      ExprNode*        index;      // array subscript, or nullptr
      ExprNode*        defaultVal; // for function params only, else nullptr
      bool             isLocal;    // name[0] == '%'

      VarNode(S32 ln, StringTableEntry n, ExprNode* idx, ExprNode* def = nullptr)
         : Node(NodeKind::Var, ln)
         , name(n), index(idx), defaultVal(def)
         , isLocal(n&& n[0] == '%') {
      }
   };

   //---------------------------------------------------------------------------
   // Assignment nodes
   //---------------------------------------------------------------------------
   struct AssignNode : Node
   {
      StringTableEntry  varName;
      ExprNode*         arrayIndex;
      ExprNode*         expr;
      AssignNode(S32 ln, StringTableEntry v, ExprNode* idx, ExprNode* e)
         : Node(NodeKind::Assign, ln), varName(v), arrayIndex(idx), expr(e) {
      }
   };

   struct AssignOpNode : Node
   {
      StringTableEntry  varName;
      ExprNode*         arrayIndex;
      ExprNode*         expr;
      S32               op;    // ASCII operator: '+' '-' '*' '/' etc., or token value
      AssignOpNode(S32 ln, StringTableEntry v, ExprNode* idx, ExprNode* e, S32 o)
         : Node(NodeKind::AssignOp, ln), varName(v), arrayIndex(idx), expr(e), op(o) {
      }
   };

   //---------------------------------------------------------------------------
   // Binary / Unary nodes
   //---------------------------------------------------------------------------
   struct BinaryFloatNode : Node
   {
      S32       op;   // '+' '-' '*' '/'
      ExprNode* left;
      ExprNode* right;
      BinaryFloatNode(S32 ln, S32 o, ExprNode* l, ExprNode* r)
         : Node(NodeKind::BinaryFloat, ln), op(o), left(l), right(r) {
      }
   };

   struct BinaryIntNode : Node
   {
      S32       op;
      ExprNode* left;
      ExprNode* right;
      BinaryIntNode(S32 ln, S32 o, ExprNode* l, ExprNode* r)
         : Node(NodeKind::BinaryInt, ln), op(o), left(l), right(r) {
      }
   };

   struct UnaryIntNode : Node
   {
      S32       op;   // '!' or '~'
      ExprNode* expr;
      UnaryIntNode(S32 ln, S32 o, ExprNode* e)
         : Node(NodeKind::UnaryInt, ln), op(o), expr(e) {
      }
   };

   struct UnaryFloatNode : Node
   {
      ExprNode* expr;
      UnaryFloatNode(S32 ln, ExprNode* e)
         : Node(NodeKind::UnaryFloat, ln), expr(e) {
      }
   };

   struct TernaryNode : Node
   {
      ExprNode* test;
      ExprNode* trueExpr;
      ExprNode* falseExpr;
      TernaryNode(S32 ln, ExprNode* t, ExprNode* tr, ExprNode* fa)
         : Node(NodeKind::Ternary, ln), test(t), trueExpr(tr), falseExpr(fa) {
      }
   };

   //---------------------------------------------------------------------------
   // String operation nodes
   //---------------------------------------------------------------------------
   struct StrCatNode : Node
   {
      ExprNode* left;
      ExprNode* right;
      char      appendChar;   // 0=none, ' '=SPC, '\n'=NL, '\t'=TAB
      StrCatNode(S32 ln, ExprNode* l, ExprNode* r, char c)
         : Node(NodeKind::StrCat, ln), left(l), right(r), appendChar(c) {
      }
   };

   struct StrEqNode : Node
   {
      ExprNode* left;
      ExprNode* right;
      bool      eq;    // true=$=  false=!$=
      StrEqNode(S32 ln, ExprNode* l, ExprNode* r, bool e)
         : Node(NodeKind::StrEq, ln), left(l), right(r), eq(e) {
      }
   };

   struct CommaCatNode : Node
   {
      ExprNode* left;
      ExprNode* right;
      CommaCatNode(S32 ln, ExprNode* l, ExprNode* r)
         : Node(NodeKind::CommaCat, ln), left(l), right(r) {
      }
   };

   //---------------------------------------------------------------------------
   // Function decl nodes
   //---------------------------------------------------------------------------
   struct FunctionDeclNode : Node
   {
      StringTableEntry fnName;
      StringTableEntry nameSpace;
      StringTableEntry package;
      VarNode*         params;     // linked list — VarNode.next
      StmtNode*        body;
      FunctionDeclNode(S32 ln, StringTableEntry fn, StringTableEntry ns,
         VarNode* p, StmtNode* b)
         : Node(NodeKind::FunctionDecl, ln)
         , fnName(fn), nameSpace(ns), package(nullptr), params(p), body(b) {
      }
   };

   //---------------------------------------------------------------------------
   // Function call nodes
   //---------------------------------------------------------------------------
   enum class CallType { Function, Static, Method, Parent };

   struct FuncCallNode : Node
   {
      StringTableEntry funcName;
      StringTableEntry nameSpace;   // nullptr for plain calls
      ExprNode*        args;        // linked list via next
      CallType         callType;
      FuncCallNode(S32 ln, StringTableEntry fn, StringTableEntry ns,
         ExprNode* a, CallType ct)
         : Node(NodeKind::FuncCall, ln)
         , funcName(fn), nameSpace(ns), args(a), callType(ct) {
      }
   };

   struct AssertNode : Node
   {
      ExprNode* testExpr;
      const char* message;   // arena-allocated, may be nullptr
      AssertNode(S32 ln, ExprNode* t, const char* m)
         : Node(NodeKind::Assert, ln), testExpr(t), message(m) {
      }
   };

   //---------------------------------------------------------------------------
   // Slot and field access / assignment nodes
   //---------------------------------------------------------------------------
   struct SlotAccessNode : Node
   {
      ExprNode* objectExpr;
      ExprNode* arrayExpr;              // nullptr if no index
      StringTableEntry slotName;
      SlotAccessNode(S32 ln, ExprNode* o, ExprNode* a, StringTableEntry s)
         : Node(NodeKind::SlotAccess, ln), objectExpr(o), arrayExpr(a), slotName(s) {
      }
   };

   struct SlotAssignNode : Node
   {
      ExprNode*         objectExpr;     // nullptr inside object body (use SETCUROBJECT_NEW)
      ExprNode*         arrayExpr;
      StringTableEntry  slotName;
      ExprNode*         valueExpr;
      U32               typeID;         // (U32)-1 = no type
      SlotAssignNode(S32 ln, ExprNode* o, ExprNode* a,
         StringTableEntry s, ExprNode* v, U32 tid)
         : Node(NodeKind::SlotAssign, ln)
         , objectExpr(o), arrayExpr(a), slotName(s), valueExpr(v), typeID(tid) {
      }
   };

   struct SlotAssignOpNode : Node
   {
      ExprNode*        objectExpr;
      ExprNode*        arrayExpr;
      StringTableEntry slotName;
      ExprNode*        valueExpr;
      S32              op;
      SlotAssignOpNode(S32 ln, ExprNode* o, ExprNode* a,
         StringTableEntry s, ExprNode* v, S32 op_)
         : Node(NodeKind::SlotAssignOp, ln)
         , objectExpr(o), arrayExpr(a), slotName(s), valueExpr(v), op(op_) {
      }
   };

   struct InternalSlotNode : Node
   {
      ExprNode* objectExpr;
      ExprNode* slotExpr;
      bool      recurse;   // true = --> (deep), false = -> (shallow)
      InternalSlotNode(S32 ln, ExprNode* o, ExprNode* s, bool r)
         : Node(NodeKind::InternalSlot, ln), objectExpr(o), slotExpr(s), recurse(r) {
      }
   };

   //---------------------------------------------------------------------------
   // Tag op nodes
   //---------------------------------------------------------------------------
   struct TTagSetNode : Node
   {
      StringTableEntry  tag;
      ExprNode*         valueExpr;
      ExprNode*         stringExpr;
      TTagSetNode(S32 ln, StringTableEntry t, ExprNode* v, ExprNode* s)
         : Node(NodeKind::TTagSet, ln), tag(t), valueExpr(v), stringExpr(s) {
      }
   };

   struct TTagDerefNode : Node
   {
      ExprNode* expr;
      TTagDerefNode(S32 ln, ExprNode* e)
         : Node(NodeKind::TTagDeref, ln), expr(e) {
      }
   };

   struct TTagExprNode : Node
   {
      StringTableEntry tag;
      TTagExprNode(S32 ln, StringTableEntry t)
         : Node(NodeKind::TTagExpr, ln), tag(t) {
      }
   };

   //---------------------------------------------------------------------------
   // Flow control nodes
   //---------------------------------------------------------------------------
   struct ReturnNode : Node
   {
      ExprNode* expr;   // nullptr = return void
      ReturnNode(S32 ln, ExprNode* e) : Node(NodeKind::Return, ln), expr(e) {}
   };

   struct BreakNode : Node
   {
      BreakNode(S32 ln) : Node(NodeKind::Break, ln) {}
   };

   struct ContinueNode : Node
   {
      ContinueNode(S32 ln) : Node(NodeKind::Continue, ln) {}
   };

   struct IfNode : Node
   {
      ExprNode* testExpr;
      StmtNode* thenBlock;
      StmtNode* elseBlock;       // nullptr if no else
      bool      propagate;       // true when this is a switch case chain
      IfNode(S32 ln, ExprNode* t, StmtNode* th, StmtNode* el, bool p)
         : Node(NodeKind::If, ln)
         , testExpr(t), thenBlock(th), elseBlock(el), propagate(p) {
      }
   };

   struct LoopNode : Node
   {
      ExprNode* initExpr;        // nullptr for while / do-while
      ExprNode* testExpr;        // always present (IntLit 1 for infinite)
      ExprNode* incrExpr;        // nullptr for while / do-while
      StmtNode* body;
      bool      isDoLoop;
      LoopNode(S32 ln, ExprNode* ini, ExprNode* tst,
         ExprNode* inc, StmtNode* b, bool doLoop)
         : Node(NodeKind::Loop, ln)
         , initExpr(ini), testExpr(tst), incrExpr(inc), body(b), isDoLoop(doLoop) {
      }
   };

   struct ForeachNode : Node
   {
      StringTableEntry  varName;
      ExprNode*         containerExpr;
      StmtNode*         body;
      bool              isStringIter;   // true = foreach$
      ForeachNode(S32 ln, StringTableEntry v, ExprNode* c, StmtNode* b, bool str)
         : Node(NodeKind::Foreach, ln)
         , varName(v), containerExpr(c), body(b), isStringIter(str) {
      }
   };

   struct ExprStmtNode : Node
   {
      ExprNode* expr;
      ExprStmtNode(S32 ln, ExprNode* e) : Node(NodeKind::ExprStmt, ln), expr(e) {}
   };

   //---------------------------------------------------------------------------
   // Object and Datablock nodes
   //---------------------------------------------------------------------------
   struct ObjectDeclNode : Node
   {
      ExprNode*         classNameExpr;
      ExprNode*         objectNameExpr;
      ExprNode*         argList;
      StringTableEntry  parentObject;
      SlotAssignNode*   slotDecls;
      ObjectDeclNode*   subObjects;
      bool              isDatablock;
      bool              isClassNameInternal;
      bool              isSingleton;
      ObjectDeclNode(S32 ln, ExprNode* cls, ExprNode* name, ExprNode* args,
         StringTableEntry parent, SlotAssignNode* slots,
         ObjectDeclNode* subs, bool db, bool internal, bool single)
         : Node(db ? NodeKind::DatablockDecl : NodeKind::ObjectDecl, ln)
         , classNameExpr(cls), objectNameExpr(name), argList(args)
         , parentObject(parent), slotDecls(slots), subObjects(subs)
         , isDatablock(db), isClassNameInternal(internal), isSingleton(single) {
      }
   };

}
