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

#include "compilationUnit.h"
#include "compilerTables.h"
#include "../ast/nodes.h"
#include "../ast/opcodes.h"
#include "../Diagnostics.h"

namespace KorkScript
{
   class CodeGen
   {
   public:

      // Compile root statment node into the compilation
      // unit, Diagnostics are appended to diags.
      // returns fals if any errors were emitted.
      bool compile(  StmtNode* root,
                     CompilationUnit& unit,
                     CodeStream& cs,
                     Vector<DiagnosticMessage>& diags);

   private:
      //-------------------------------------------------------------------
      // Statement visitors
      //-------------------------------------------------------------------
      void visitStmtList(StmtNode* list, CompilationUnit& u, CodeStream& cs);
      void visitStmt(StmtNode* node, CompilationUnit& u, CodeStream& cs);
      void visitFunctionDecl(FunctionDeclNode* n, CompilationUnit& u, CodeStream& cs);
      void visitReturn(ReturnNode* n, CompilationUnit& u, CodeStream& cs);
      void visitBreak(BreakNode* n, CompilationUnit& u, CodeStream& cs);
      void visitContinue(ContinueNode* n, CompilationUnit& u, CodeStream& cs);
      void visitIf(IfNode* n, CompilationUnit& u, CodeStream& cs);
      void visitLoop(LoopNode* n, CompilationUnit& u, CodeStream& cs);
      void visitForeach(ForeachNode* n, CompilationUnit& u, CodeStream& cs);
      void visitExprStmt(ExprStmtNode* n, CompilationUnit& u, CodeStream& cs);
      void visitTTagSet(TTagSetNode* n, CompilationUnit& u, CodeStream& cs);
      void visitObjectDecl(ObjectDeclNode* n, CompilationUnit& u, CodeStream& cs);
      void visitObjectBody(ObjectDeclNode* n, CompilationUnit& u, CodeStream& cs, bool root);

      //-------------------------------------------------------------------
      // Expression visitors
      //-------------------------------------------------------------------
      // req is the type the parent context wants.
      // Each visitor may emit a type conversion at the end if needed.
      void visitExpr(ExprNode* node, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitIntLit(IntLitNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitFloatLit(FloatLitNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitStrLit(StrLitNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitConst(ConstNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitVar(VarNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitAssign(AssignNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitAssignOp(AssignOpNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitBinaryFloat(BinaryFloatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitBinaryInt(BinaryIntNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitUnaryInt(UnaryIntNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitUnaryFloat(UnaryFloatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitTernary(TernaryNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitStrCat(StrCatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitStrEq(StrEqNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitCommaCat(CommaCatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitFuncCall(FuncCallNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitSlotAccess(SlotAccessNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitSlotAssign(SlotAssignNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitSlotAssignOp(SlotAssignOpNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitInternalSlot(InternalSlotNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitAssert(AssertNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitTTagDeref(TTagDerefNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);
      void visitTTagExpr(TTagExprNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs);

      //-------------------------------------------------------------------
      // Helpers
      //-------------------------------------------------------------------

      /// Return the natural TypeReq a node produces without conversion.
      TypeReq naturalType(ExprNode* node) const;

      /// Emit an STE + record in identTable if in DSO mode.
      void emitSTE(  StringTableEntry ste,
                     CompilationUnit& u,
                     CodeStream& cs);

      /// Add a string to the current pool, return its offset.
      U32 addString(const char* str, CompilationUnit& u, bool caseSens = true, bool tag = false);

      /// Add a float to the current pool, return its index.
      U32 addFloat(F64 val, CompilationUnit& u);

      /// Convert a string constant to a number (same semantics as legacy).
      static F64 stringToNumber(const char* str);

      /// Emit a break line record for debugger.
      void emitBreakLine(S32 line, CodeStream& cs);

      /// Map an AssignOp token to the float/int opcode and TypeReq.
      struct OpInfo { Op opcode; TypeReq type; };
      static OpInfo assignOpInfo(S32 tokenOp);

      /// Whether a local variable name is actually global (starts with $).
      static bool isGlobal(StringTableEntry name) { return name && name[0] == '$'; }

      //-------------------------------------------------------------------
      // Error tracking
      //-------------------------------------------------------------------
      Vector<DiagnosticMessage>* mDiags = nullptr;

      void error(S32 line, const char* fmt, ...);
      void warning(S32 line, const char* fmt, ...);
   };
}
