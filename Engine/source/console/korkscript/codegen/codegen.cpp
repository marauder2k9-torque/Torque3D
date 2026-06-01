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

#include "codegen.h"
#include "console/console.h"

namespace KorkScript
{
   bool CodeGen::compile(StmtNode* root, CompilationUnit& unit, CodeStream& cs, Vector<DiagnosticMessage>& diags)
   {
      mDiags = &diags;
      if (root)
         visitStmtList(root, unit, cs);
      mDiags = nullptr;
      return true;
   }

   //-------------------------------------------------------------------
   // Diagnostics
   //-------------------------------------------------------------------
   void CodeGen::error(S32 lin, const char* fmt, ...)
   {
      if (!mDiags) return;
      char buf[512];
      va_list a;
      va_start(a, fmt);
      vsnprintf(buf, sizeof(buf), fmt, a);
      va_end(a);
      mDiags->push_back({ DiagnosticMessage::Severity::Error, SourceRange{}, buf });
   }

   void CodeGen::warning(S32 line, const char* fmt, ...)
   {
      if (!mDiags) return;
      char buf[512];
      va_list a;
      va_start(a, fmt);
      vsnprintf(buf, sizeof(buf), fmt, a);
      va_end(a);
      mDiags->push_back({ DiagnosticMessage::Severity::Warning, SourceRange{}, buf });
   }

   //-------------------------------------------------------------------
   // Helpers
   //-------------------------------------------------------------------
   void CodeGen::emitBreakLine(S32 line, CodeStream& cs)
   {
      cs.addBreakLine((U32)line, cs.tell());
   }

   void CodeGen::emitSTE(StringTableEntry ste, CompilationUnit& u, CodeStream& cs)
   {
      U32 ip = cs.emitSTE(ste, u.steToCode);
      if (!u.isEval && ste)
         u.identTable.add(ste, ip);
   }

   U32 CodeGen::addString(const char* str, CompilationUnit& u, bool caseSens, bool tag)
   {
      return u.currentStringPool().add(str, caseSens, tag);
   }

   U32 CodeGen::addFloat(F64 val, CompilationUnit& u)
   {
      return u.currentFloatPool().add(val);
   }

   F64 CodeGen::stringToNumber(const char* str)
   {
      F64 val = strtod(str, nullptr);
      if (val != 0.0) return val;
      if (dStricmp(str, "true") == 0) return 1.0;
      if (dStricmp(str, "false") == 0) return 0.0;
      return 0.0;
   }

   TypeReq CodeGen::naturalType(ExprNode* node) const
   {
      if (!node) return TypeReq::None;
      switch (node->kind)
      {
      case NodeKind::IntLit:      return TypeReq::UInt;
      case NodeKind::FloatLit:    return TypeReq::Float;
      case NodeKind::StrLit:
      case NodeKind::TagLit:
      case NodeKind::DocBlock:
      case NodeKind::Const:       return TypeReq::String;
      case NodeKind::BinaryFloat:
      case NodeKind::UnaryFloat:  return TypeReq::Float;
      case NodeKind::BinaryInt:
      case NodeKind::UnaryInt:
      case NodeKind::StrEq:
      case NodeKind::TTagExpr:    return TypeReq::UInt;
      case NodeKind::StrCat:
      case NodeKind::CommaCat:    return TypeReq::String;
      case NodeKind::Var:
      {
         const VarNode* v = static_cast<const VarNode*>(node);
         if (!v->isLocal) return TypeReq::None;
         // type inference deferred — return None and let caller decide
         return TypeReq::None;
      }
      default:                    return TypeReq::String;
      }
   }

   CodeGen::OpInfo CodeGen::assignOpInfo(S32 tokenOp)
   {
      // tokenOp is the ASCII operator character or a KorkScript TokenKind cast.
      // Map to bytecode Op + the TypeReq the operands need.
      switch (tokenOp)
      {
      case '+': return { Op::Add, TypeReq::Float };
      case '-': return { Op::Sub, TypeReq::Float };
      case '*': return { Op::Mul, TypeReq::Float };
      case '/': return { Op::Div, TypeReq::Float };
      case '%': return { Op::Mod, TypeReq::UInt };
      case '&': return { Op::BitAnd, TypeReq::UInt };
      case '^': return { Op::Xor,    TypeReq::UInt };
      case '|': return { Op::BitOr,  TypeReq::UInt };
      // opPLUSPLUS / opMINUSMINUS map to Add/Sub on float
      default:  return { Op::Add, TypeReq::Float };
      }
   }

   //-------------------------------------------------------------------
   // Statement visitors
   //-------------------------------------------------------------------
   void CodeGen::visitStmtList(StmtNode* list, CompilationUnit& u, CodeStream& cs)
   {
      for (StmtNode* n = list; n; n = n->next)
         visitStmt(n, u, cs);
   }

   void CodeGen::visitStmt(StmtNode* node, CompilationUnit& u, CodeStream& cs)
   {
      if (!node) return;
      switch (node->kind)
      {
      case NodeKind::FunctionDecl:
         visitFunctionDecl(static_cast<FunctionDeclNode*>(node), u, cs); break;
      case NodeKind::Return:
         visitReturn(static_cast<ReturnNode*>(node), u, cs); break;
      case NodeKind::Break:
         visitBreak(static_cast<BreakNode*>(node), u, cs); break;
      case NodeKind::Continue:
         visitContinue(static_cast<ContinueNode*>(node), u, cs); break;
      case NodeKind::If:
         visitIf(static_cast<IfNode*>(node), u, cs); break;
      case NodeKind::Loop:
         visitLoop(static_cast<LoopNode*>(node), u, cs); break;
      case NodeKind::Foreach:
         visitForeach(static_cast<ForeachNode*>(node), u, cs); break;
      case NodeKind::ExprStmt:
         visitExprStmt(static_cast<ExprStmtNode*>(node), u, cs); break;
      case NodeKind::TTagSet:
         visitTTagSet(static_cast<TTagSetNode*>(node), u, cs); break;
      case NodeKind::ObjectDecl:
      case NodeKind::DatablockDecl:
         visitObjectDecl(static_cast<ObjectDeclNode*>(node), u, cs); break;
      case NodeKind::DocBlock:
      {
         StrLitNode* doc = static_cast<StrLitNode*>(node);
         U32 idx = addString(doc->str, u);
         cs.emit(op(Op::DocBlockStr));
         cs.emit(idx);
         break;
      }
      default:
         // Expressions used as statements
         emitBreakLine(node->line, cs);
         visitExpr(node, TypeReq::None, u, cs);
         break;
      }
   }

   //-------------------------------------------------------------------
   // Function decl visitor
   //-------------------------------------------------------------------
   void CodeGen::visitFunctionDecl(FunctionDeclNode* n, CompilationUnit& u, CodeStream& cs)
   {
      u.enterFunction(n->fnName, n->nameSpace);

      // Count params and assign registers
      U32 argc = 0;
      for (VarNode* p = n->params; p; p = static_cast<VarNode*>(p->next))
      {
         u.funcVars.assign(p->name, TypeReq::None);
         argc++;
      }

      // Emit header
      cs.emit(op(Op::FuncDecl));
      emitSTE(n->fnName, u, cs);
      emitSTE(n->nameSpace, u, cs);
      emitSTE(n->package, u, cs);

      cs.emit(U32(n->body != nullptr ? 1 : 0) | U32(n->line << 1));
      const U32 endIpSlot = cs.emit(0);
      cs.emit(argc);
      const U32 localCountSlot = cs.emit(0);

      // Register mappings
      for (VarNode* p = n->params; p; p = static_cast<VarNode*>(p->next))
         cs.emit((U32)u.funcVars.lookup(p->name));

      // Arg flags (bit 0 = has default)
      for (VarNode* p = n->params; p; p = static_cast<VarNode*>(p->next))
         cs.emit(p->defaultVal ? 1u : 0u);

      // Default codelet IP slots
      Vector<U32> defaultSlots;
      defaultSlots.setSize(argc);
      for (U32 i = 0; i < argc; i++)
         defaultSlots[i] = cs.emit(0);

      // Body
      visitStmtList(n->body, u, cs);
      emitBreakLine(n->line, cs);
      cs.emit(op(Op::ReturnVoid));

      // Patch local var count
      cs.patch(localCountSlot, (U32)u.funcVars.count());

      // Emit default codelets
      U32 argIdx = 0;
      for (VarNode* p = n->params; p; p = static_cast<VarNode*>(p->next), ++argIdx)
      {
         if (p->defaultVal)
         {
            U32 codeletStart = cs.tell();
            cs.patch(defaultSlots[argIdx], codeletStart);
            TypeReq t = naturalType(p->defaultVal);
            if (t == TypeReq::None) t = TypeReq::String;
            visitExpr(p->defaultVal, t, u, cs);
            cs.emit(op(Op::DefaultEnd));
         }
      }

      cs.patch(endIpSlot, cs.tell());

      // Record variable → register mapping for debugger
      for (S32 i = 0; i < u.funcVars.count(); i++)
      {
         StringTableEntry vname = u.funcVars.nameForRegister(i);
         if (vname)
            u.variableRegisters.add(n->nameSpace, n->fnName, vname);
      }

      u.exitFunction();
   }

   //-------------------------------------------------------------------
   // Return / Break / Continue visitor
   //-------------------------------------------------------------------
   void CodeGen::visitReturn(ReturnNode* n, CompilationUnit& u, CodeStream& cs)
   {
      emitBreakLine(n->line, cs);
      if (!n->expr)
      {
         cs.emit(op(Op::ReturnVoid));
         return;
      }
      TypeReq t = naturalType(n->expr);
      if (t == TypeReq::None) t = TypeReq::String;
      visitExpr(n->expr, t, u, cs);
      switch (t)
      {
      case TypeReq::UInt:  cs.emit(op(Op::ReturnUInt)); break;
      case TypeReq::Float: cs.emit(op(Op::ReturnFlt));  break;
      default:             cs.emit(op(Op::Return));     break;
      }
   }

   void CodeGen::visitBreak(BreakNode* n, CompilationUnit& u, CodeStream& cs)
   {
      if (!cs.inLoop())
      {
         warning(n->line, "break outside of loop — ignored.");
         return;
      }
      emitBreakLine(n->line, cs);
      cs.emit(op(Op::Jmp));
      cs.emitFix(CodeStream::FixType::Break);
   }

   void CodeGen::visitContinue(ContinueNode* n, CompilationUnit& u, CodeStream& cs)
   {
      if (!cs.inLoop())
      {
         warning(n->line, "continue outside of loop — ignored.");
         return;
      }
      emitBreakLine(n->line, cs);
      cs.emit(op(Op::Jmp));
      cs.emitFix(CodeStream::FixType::Continue);
   }

   //-------------------------------------------------------------------
   // If visitor
   //-------------------------------------------------------------------
   void CodeGen::visitIf(IfNode* n, CompilationUnit& u, CodeStream& cs)
   {
      emitBreakLine(n->line, cs);

      TypeReq testType = naturalType(n->testExpr);
      bool intTest = (testType == TypeReq::UInt);

      if (testType == TypeReq::String || testType == TypeReq::None)
      {
         visitExpr(n->testExpr, TypeReq::String, u, cs);
         cs.emit(op(Op::JmpNotString));
      }
      else
      {
         visitExpr(n->testExpr, intTest ? TypeReq::UInt : TypeReq::Float, u, cs);
         cs.emit(op(intTest ? Op::JmpIfNot : Op::JmpIffNot));
      }

      if (n->elseBlock)
      {
         U32 elseIp = cs.emit(0);
         visitStmtList(n->thenBlock, u, cs);
         cs.emit(op(Op::Jmp));
         U32 endIp = cs.emit(0);
         cs.patch(elseIp, cs.tell());
         visitStmtList(n->elseBlock, u, cs);
         cs.patch(endIp, cs.tell());
      }
      else
      {
         U32 endIp = cs.emit(0);
         visitStmtList(n->thenBlock, u, cs);
         cs.patch(endIp, cs.tell());
      }
   }

   //-------------------------------------------------------------------
   // Loop visitor
   //-------------------------------------------------------------------
   void CodeGen::visitLoop(LoopNode* n, CompilationUnit& u, CodeStream& cs)
   {
      emitBreakLine(n->line, cs);
      cs.pushFixScope(true);

      TypeReq testType = naturalType(n->testExpr);
      bool intTest = (testType == TypeReq::UInt);

      if (n->initExpr)
         visitExpr(n->initExpr, TypeReq::None, u, cs);

      if (!n->isDoLoop)
      {
         visitExpr(n->testExpr, intTest ? TypeReq::UInt : TypeReq::Float, u, cs);
         cs.emit(op(intTest ? Op::JmpIfNot : Op::JmpIffNot));
         cs.emitFix(CodeStream::FixType::Break);
      }

      const U32 loopStart = cs.tell();
      visitStmtList(n->body, u, cs);
      const U32 continuePoint = cs.tell();

      if (n->incrExpr)
         visitExpr(n->incrExpr, TypeReq::None, u, cs);

      visitExpr(n->testExpr, intTest ? TypeReq::UInt : TypeReq::Float, u, cs);
      cs.emit(op(intTest ? Op::JmpIf : Op::JmpIff));
      cs.emitFix(CodeStream::FixType::LoopBlockStart);

      const U32 breakPoint = cs.tell();
      cs.fixLoop(loopStart, breakPoint, continuePoint);
      cs.popFixScope();
   }

   //-------------------------------------------------------------------
   // Foreach/Foreach$ visitor
   //-------------------------------------------------------------------
   void CodeGen::visitForeach(ForeachNode* n, CompilationUnit& u, CodeStream& cs)
   {
      emitBreakLine(n->line, cs);
      cs.pushFixScope(true);

      bool isGlobal_ = isGlobal(n->varName);
      TypeReq varType = n->isStringIter ? TypeReq::String : TypeReq::UInt;

      visitExpr(n->containerExpr, TypeReq::String, u, cs);

      cs.emit(op(n->isStringIter ? Op::IterBeginStr : Op::IterBegin));
      cs.emit((U32)isGlobal_);
      if (isGlobal_)
         emitSTE(n->varName, u, cs);
      else
         cs.emit((U32)u.funcVars.assign(n->varName, varType));

      const U32 finalFix = cs.emit(0);
      const U32 continueIp = cs.tell();
      cs.emit(op(Op::Iter));
      cs.emitFix(CodeStream::FixType::Break);
      const U32 bodyIp = cs.tell();

      visitStmtList(n->body, u, cs);

      const U32 breakIp = cs.tell() + 2;
      const U32 finalIp = breakIp + 1;

      cs.emit(op(Op::Jmp));
      cs.emitFix(CodeStream::FixType::Continue);
      cs.emit(op(Op::IterEnd));

      cs.patch(finalFix, finalIp);
      cs.fixLoop(bodyIp, breakIp, continueIp);
      cs.popFixScope();
   }

   //-------------------------------------------------------------------
   // Expression visitor
   //-------------------------------------------------------------------
   void CodeGen::visitExprStmt(ExprStmtNode* n, CompilationUnit& u, CodeStream& cs)
   {
      emitBreakLine(n->line, cs);
      visitExpr(n->expr, TypeReq::None, u, cs);
   }

   void CodeGen::visitExpr(ExprNode* node, TypeReq req,
      CompilationUnit& u, CodeStream& cs)
   {
      if (!node) return;
      switch (node->kind)
      {
      case NodeKind::IntLit:       visitIntLit(static_cast<IntLitNode*>      (node), req, u, cs); break;
      case NodeKind::FloatLit:     visitFloatLit(static_cast<FloatLitNode*>    (node), req, u, cs); break;
      case NodeKind::StrLit:
      case NodeKind::TagLit:
      case NodeKind::DocBlock:     visitStrLit(static_cast<StrLitNode*>      (node), req, u, cs); break;
      case NodeKind::Const:        visitConst(static_cast<ConstNode*>       (node), req, u, cs); break;
      case NodeKind::Var:          visitVar(static_cast<VarNode*>         (node), req, u, cs); break;
      case NodeKind::Assign:       visitAssign(static_cast<AssignNode*>      (node), req, u, cs); break;
      case NodeKind::AssignOp:     visitAssignOp(static_cast<AssignOpNode*>    (node), req, u, cs); break;
      case NodeKind::BinaryFloat:  visitBinaryFloat(static_cast<BinaryFloatNode*> (node), req, u, cs); break;
      case NodeKind::BinaryInt:    visitBinaryInt(static_cast<BinaryIntNode*>   (node), req, u, cs); break;
      case NodeKind::UnaryInt:     visitUnaryInt(static_cast<UnaryIntNode*>    (node), req, u, cs); break;
      case NodeKind::UnaryFloat:   visitUnaryFloat(static_cast<UnaryFloatNode*>  (node), req, u, cs); break;
      case NodeKind::Ternary:      visitTernary(static_cast<TernaryNode*>     (node), req, u, cs); break;
      case NodeKind::StrCat:       visitStrCat(static_cast<StrCatNode*>      (node), req, u, cs); break;
      case NodeKind::StrEq:        visitStrEq(static_cast<StrEqNode*>       (node), req, u, cs); break;
      case NodeKind::CommaCat:     visitCommaCat(static_cast<CommaCatNode*>    (node), req, u, cs); break;
      case NodeKind::FuncCall:     visitFuncCall(static_cast<FuncCallNode*>    (node), req, u, cs); break;
      case NodeKind::SlotAccess:   visitSlotAccess(static_cast<SlotAccessNode*>  (node), req, u, cs); break;
      case NodeKind::SlotAssign:   visitSlotAssign(static_cast<SlotAssignNode*>  (node), req, u, cs); break;
      case NodeKind::SlotAssignOp: visitSlotAssignOp(static_cast<SlotAssignOpNode*>(node), req, u, cs); break;
      case NodeKind::InternalSlot: visitInternalSlot(static_cast<InternalSlotNode*>(node), req, u, cs); break;
      case NodeKind::Assert:       visitAssert(static_cast<AssertNode*>      (node), req, u, cs); break;
      case NodeKind::TTagDeref:    visitTTagDeref(static_cast<TTagDerefNode*>   (node), req, u, cs); break;
      case NodeKind::TTagExpr:     visitTTagExpr(static_cast<TTagExprNode*>    (node), req, u, cs); break;
      case NodeKind::ObjectDecl:
      case NodeKind::DatablockDecl:visitObjectDecl(static_cast<ObjectDeclNode*>  (node), u, cs); break;
      default: break;
      }
   }

   //-------------------------------------------------------------------
   // TTag visitor
   //-------------------------------------------------------------------
   void CodeGen::visitTTagSet(TTagSetNode* n, CompilationUnit& u, CodeStream& cs)
   {
      // TTag operations are no-ops in the current VM — emit nothing.
      (void)n; (void)u; (void)cs;
   }

   void CodeGen::visitTTagDeref(TTagDerefNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      (void)n; (void)req; (void)u; (void)cs;
   }

   void CodeGen::visitTTagExpr(TTagExprNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      (void)n; (void)req; (void)u; (void)cs;
   }

   //-------------------------------------------------------------------
   // Object / datablock visitor
   //-------------------------------------------------------------------
   void CodeGen::visitObjectDecl(ObjectDeclNode* n, CompilationUnit& u, CodeStream& cs)
   {
      cs.emit(op(Op::LoadImmedUInt));
      cs.emit(0u);
      visitObjectBody(n, u, cs, true);
   }

   void CodeGen::visitObjectBody(ObjectDeclNode* n, CompilationUnit& u, CodeStream& cs, bool root)
   {
      // Count args
      S32 count = 2;
      for (ExprNode* w = n->argList; w; w = w->next) count++;

      cs.emit(op(Op::PushFrame));
      cs.emit((U32)count);

      visitExpr(n->classNameExpr, TypeReq::String, u, cs);
      cs.emit(op(Op::Push));
      visitExpr(n->objectNameExpr, TypeReq::String, u, cs);
      cs.emit(op(Op::Push));

      for (ExprNode* w = n->argList; w; w = w->next)
      {
         TypeReq t = naturalType(w);
         if (t == TypeReq::None) t = TypeReq::String;
         visitExpr(w, t, u, cs);
         cs.emit(op(Op::Push));
      }

      cs.emit(op(Op::CreateObject));
      emitSTE(n->parentObject, u, cs);
      cs.emit((U32)n->isDatablock);
      cs.emit((U32)n->isClassNameInternal);
      cs.emit((U32)n->isSingleton);
      cs.emit((U32)n->line);
      const U32 failIp = cs.emit(0);

      for (SlotAssignNode* s = n->slotDecls; s; s = static_cast<SlotAssignNode*>(s->next))
         visitSlotAssign(s, TypeReq::None, u, cs);

      cs.emit(op(Op::AddObject));
      cs.emit((U32)root);

      for (ObjectDeclNode* sub = n->subObjects; sub;
         sub = static_cast<ObjectDeclNode*>(sub->next))
         visitObjectBody(sub, u, cs, false);

      cs.emit(op(Op::EndObject));
      cs.emit((U32)(root || n->isDatablock));
      const U32 failOffset = cs.emit(op(Op::FinishObject));

      cs.patch(failIp, failOffset);
   }

   //-------------------------------------------------------------------
   // Literals visitors
   //-------------------------------------------------------------------
   void CodeGen::visitIntLit(IntLitNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      switch (req)
      {
      case TypeReq::UInt:
         cs.emit(op(Op::LoadImmedUInt));
         cs.emit((U32)n->value);
         break;
      case TypeReq::Float:
         cs.emit(op(Op::LoadImmedFlt));
         cs.emit(addFloat((F64)n->value, u));
         break;
      case TypeReq::String:
         cs.emit(op(Op::LoadImmedStr));
         cs.emit(u.currentStringPool().addInt((U32)n->value));
         break;
      default: break;
      }
   }

   void CodeGen::visitFloatLit(FloatLitNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      switch (req)
      {
      case TypeReq::UInt:
         cs.emit(op(Op::LoadImmedUInt));
         cs.emit((U32)(S32)n->value);
         break;
      case TypeReq::Float:
         cs.emit(op(Op::LoadImmedFlt));
         cs.emit(addFloat(n->value, u));
         break;
      case TypeReq::String:
         cs.emit(op(Op::LoadImmedStr));
         cs.emit(u.currentStringPool().addFloat(n->value));
         break;
      default: break;
      }
   }

   void CodeGen::visitStrLit(StrLitNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      if (n->doc)
      {
         cs.emit(op(Op::DocBlockStr));
         cs.emit(addString(n->str, u));
         return;
      }
      F64 fval = stringToNumber(n->str);
      switch (req)
      {
      case TypeReq::String:
         cs.emit(op(n->tag ? Op::TagToStr : Op::LoadImmedStr));
         cs.emit(addString(n->str, u, true, n->tag));
         break;
      case TypeReq::UInt:
         cs.emit(op(Op::LoadImmedUInt));
         cs.emit((U32)(S32)fval);
         break;
      case TypeReq::Float:
         cs.emit(op(Op::LoadImmedFlt));
         cs.emit(addFloat(fval, u));
         break;
      default: break;
      }
   }

   void CodeGen::visitConst(ConstNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      if (req == TypeReq::None) return;
      F64 fval = stringToNumber(n->value);
      switch (req)
      {
      case TypeReq::String:
         cs.emit(op(Op::LoadImmedIdent));
         emitSTE(n->value, u, cs);
         break;
      case TypeReq::UInt:
         cs.emit(op(Op::LoadImmedUInt));
         cs.emit((U32)(S32)fval);
         break;
      case TypeReq::Float:
         cs.emit(op(Op::LoadImmedFlt));
         cs.emit(addFloat(fval, u));
         break;
      default: break;
      }
   }

   //-------------------------------------------------------------------
   // Variable visitors
   //-------------------------------------------------------------------
   void CodeGen::visitVar(VarNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      if (req == TypeReq::None) return;

      bool global = isGlobal(n->name) || n->index;

      if (global)
      {
         if (n->index)
         {
            cs.emit(op(Op::LoadImmedIdent));
            emitSTE(n->name, u, cs);
            visitExpr(n->index, TypeReq::String, u, cs);
            cs.emit(op(Op::RewindStr));
            cs.emit(op(Op::SetCurVarArray));
            cs.emit(op(Op::PopStk));
         }
         else
         {
            cs.emit(op(Op::SetCurVar));
            emitSTE(n->name, u, cs);
         }
         switch (req)
         {
         case TypeReq::UInt:   cs.emit(op(Op::LoadVarUInt)); break;
         case TypeReq::Float:  cs.emit(op(Op::LoadVarFlt));  break;
         case TypeReq::String: cs.emit(op(Op::LoadVarStr));  break;
         default: break;
         }
      }
      else
      {
         S32 reg = u.funcVars.lookup(n->name);
         if (reg < 0)
         {
            warning(n->line, "Undefined local variable '%s'", n->name);
            reg = u.funcVars.assign(n->name, req);
         }
         switch (req)
         {
         case TypeReq::UInt:   cs.emit(op(Op::LoadLocalVarUInt)); break;
         case TypeReq::Float:  cs.emit(op(Op::LoadLocalVarFlt));  break;
         default:              cs.emit(op(Op::LoadLocalVarStr));  break;
         }
         cs.emit((U32)reg);
      }
   }

   // Assignment
   void CodeGen::visitAssign(AssignNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      TypeReq subType = naturalType(n->expr);
      if (subType == TypeReq::None) subType = req;
      if (subType == TypeReq::None) subType = TypeReq::String;

      visitExpr(n->expr, subType, u, cs);

      bool global = isGlobal(n->varName) || n->arrayIndex;
      if (global)
      {
         if (n->arrayIndex)
         {
            cs.emit(op(Op::LoadImmedIdent));
            emitSTE(n->varName, u, cs);
            visitExpr(n->arrayIndex, TypeReq::String, u, cs);
            cs.emit(op(Op::RewindStr));
            cs.emit(op(Op::SetCurVarArrayCreate));
            if (req == TypeReq::None) cs.emit(op(Op::PopStk));
         }
         else
         {
            cs.emit(op(Op::SetCurVarCreate));
            emitSTE(n->varName, u, cs);
         }
         switch (subType)
         {
         case TypeReq::String: cs.emit(op(Op::SaveVarStr));  break;
         case TypeReq::UInt:   cs.emit(op(Op::SaveVarUInt)); break;
         case TypeReq::Float:  cs.emit(op(Op::SaveVarFlt));  break;
         default: break;
         }
      }
      else
      {
         S32 reg = u.funcVars.assign(n->varName,
            subType == TypeReq::None ? TypeReq::String : subType);
         switch (subType)
         {
         case TypeReq::UInt:   cs.emit(op(Op::SaveLocalVarUInt)); break;
         case TypeReq::Float:  cs.emit(op(Op::SaveLocalVarFlt));  break;
         default:              cs.emit(op(Op::SaveLocalVarStr));  break;
         }
         cs.emit((U32)reg);
      }

      if (req == TypeReq::None) cs.emit(op(Op::PopStk));
   }

   // Compund Assignment
   void CodeGen::visitAssignOp(AssignOpNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      OpInfo info = assignOpInfo(n->op);
      visitExpr(n->expr, info.type, u, cs);

      bool global = isGlobal(n->varName) || n->arrayIndex;
      if (global)
      {
         if (!n->arrayIndex)
         {
            cs.emit(op(Op::SetCurVarCreate));
            emitSTE(n->varName, u, cs);
         }
         else
         {
            cs.emit(op(Op::LoadImmedIdent));
            emitSTE(n->varName, u, cs);
            visitExpr(n->arrayIndex, TypeReq::String, u, cs);
            cs.emit(op(Op::RewindStr));
            cs.emit(op(Op::SetCurVarArrayCreate));
            if (req == TypeReq::None) cs.emit(op(Op::PopStk));
         }
         cs.emit(op(info.type == TypeReq::Float ? Op::LoadVarFlt : Op::LoadVarUInt));
         cs.emit(op(info.opcode));
         cs.emit(op(info.type == TypeReq::Float ? Op::SaveVarFlt : Op::SaveVarUInt));
      }
      else
      {
         S32 reg = u.funcVars.assign(n->varName, info.type);
         cs.emit(op(info.type == TypeReq::Float ? Op::LoadLocalVarFlt : Op::LoadLocalVarUInt));
         cs.emit((U32)reg);
         cs.emit(op(info.opcode));
         cs.emit(op(info.type == TypeReq::Float ? Op::SaveLocalVarFlt : Op::SaveLocalVarUInt));
         cs.emit((U32)reg);
      }
      if (req == TypeReq::None) cs.emit(op(Op::PopStk));
   }

   //-------------------------------------------------------------------
   // Binary expression visitors
   //-------------------------------------------------------------------
   static Op floatBinaryOp(S32 tokenOp)
   {
      switch (tokenOp)
      {
      case '+': return Op::Add;
      case '-': return Op::Sub;
      case '*': return Op::Mul;
      case '/': return Op::Div;
      default:  return Op::Add;
      }
   }

   void CodeGen::visitBinaryFloat(BinaryFloatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      visitExpr(n->right, TypeReq::Float, u, cs);
      visitExpr(n->left, TypeReq::Float, u, cs);
      cs.emit(op(floatBinaryOp(n->op)));
   }

   static Op intBinaryOp(S32 tokenOp, TypeReq& outSubType)
   {
      outSubType = TypeReq::UInt;
      switch (tokenOp)
      {
      case '^': return Op::Xor;
      case '%': return Op::Mod;
      case '&': return Op::BitAnd;
      case '|': return Op::BitOr;
      case '<': outSubType = TypeReq::Float; return Op::CmpLt;
      case '>': outSubType = TypeReq::Float; return Op::CmpGr;
         // opGE / opLE / opEQ / opNE / opOR / opAND / opSHL / opSHR
         // stored as raw token kind values from the parser
      default:  return Op::CmpEq; // fallback
      }
   }

   void CodeGen::visitBinaryInt(BinaryIntNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      TypeReq subType;
      Op opcode = intBinaryOp(n->op, subType);

      // Short-circuit operators (AND / OR)
      if (opcode == Op::Or || opcode == Op::And)
      {
         visitExpr(n->left, subType, u, cs);
         cs.emit(op(opcode == Op::Or ? Op::JmpIfNP : Op::JmpIfNotNP));
         U32 jmpIp = cs.emit(0);
         visitExpr(n->right, subType, u, cs);
         cs.patch(jmpIp, cs.tell());
         return;
      }

      visitExpr(n->right, subType, u, cs);
      visitExpr(n->left, subType, u, cs);
      cs.emit(op(opcode));
   }

   //-------------------------------------------------------------------
   // Unary visitors
   //-------------------------------------------------------------------
   void CodeGen::visitUnaryInt(UnaryIntNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      if (n->op == '!')
      {
         TypeReq pref = naturalType(n->expr);
         if (pref == TypeReq::Float)
         {
            visitExpr(n->expr, TypeReq::Float, u, cs);
            cs.emit(op(Op::NotF));
         }
         else
         {
            visitExpr(n->expr, TypeReq::String, u, cs);
            cs.emit(op(Op::Not));
         }
      }
      else if (n->op == '~')
      {
         visitExpr(n->expr, TypeReq::UInt, u, cs);
         cs.emit(op(Op::OnesComplement));
      }
   }

   void CodeGen::visitUnaryFloat(UnaryFloatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      visitExpr(n->expr, TypeReq::Float, u, cs);
      cs.emit(op(Op::Neg));
   }

   //-------------------------------------------------------------------
   // Ternary visitors
   //-------------------------------------------------------------------
   void CodeGen::visitTernary(TernaryNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      TypeReq pref = naturalType(n->test);
      bool intTest = (pref == TypeReq::UInt);
      visitExpr(n->test, intTest ? TypeReq::UInt : TypeReq::Float, u, cs);
      cs.emit(op(intTest ? Op::JmpIfNot : Op::JmpIffNot));
      U32 elseIp = cs.emit(0);
      visitExpr(n->trueExpr, req, u, cs);
      cs.emit(op(Op::Jmp));
      U32 endIp = cs.emit(0);
      cs.patch(elseIp, cs.tell());
      visitExpr(n->falseExpr, req, u, cs);
      cs.patch(endIp, cs.tell());
   }

   //-------------------------------------------------------------------
   // String op visitors
   //-------------------------------------------------------------------
   void CodeGen::visitStrCat(StrCatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      visitExpr(n->left, TypeReq::String, u, cs);
      if (n->appendChar)
      {
         cs.emit(op(Op::AdvanceStrAppendChar));
         cs.emit((U32)(U8)n->appendChar);
      }
      visitExpr(n->right, TypeReq::String, u, cs);
      cs.emit(op(Op::RewindStr));
   }

   void CodeGen::visitStrEq(StrEqNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      visitExpr(n->left, TypeReq::String, u, cs);
      visitExpr(n->right, TypeReq::String, u, cs);
      cs.emit(op(Op::CompareStr));
      if (!n->eq) cs.emit(op(Op::Not));
   }

   void CodeGen::visitCommaCat(CommaCatNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      visitExpr(n->left, TypeReq::String, u, cs);
      cs.emit(op(Op::AdvanceStrAppendChar));
      cs.emit((U32)'_');
      visitExpr(n->right, TypeReq::String, u, cs);
      cs.emit(op(Op::RewindStr));
   }

   //-------------------------------------------------------------------
   // Function call visitors
   //-------------------------------------------------------------------
   void CodeGen::visitFuncCall(FuncCallNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      /*Con::printf(
         "visitFuncCall: %s req=%d callType=%d",
         n->funcName,
         req,
         n->callType);*/

      S32 count = 0;
      for (ExprNode* w = n->args; w; w = w->next) count++;

      cs.emit(op(Op::PushFrame));
      cs.emit((U32)count);

      for (ExprNode* w = n->args; w; w = w->next)
      {
         TypeReq t = naturalType(w);
         if (t == TypeReq::None) t = TypeReq::String;
         visitExpr(w, t, u, cs);
         cs.emit(op(Op::Push));
      }

      cs.emit(op(Op::CallFunc));
      emitSTE(n->funcName, u, cs);
      emitSTE(n->nameSpace, u, cs);
      cs.emit((U32)n->callType);

      if (req == TypeReq::None) cs.emit(op(Op::PopStk));
   }

   //-------------------------------------------------------------------
   // Slot access visitor
   //-------------------------------------------------------------------
   void CodeGen::visitSlotAccess(SlotAccessNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      if (req == TypeReq::None) return;

      if (n->arrayExpr)
         visitExpr(n->arrayExpr, TypeReq::String, u, cs);

      visitExpr(n->objectExpr, TypeReq::String, u, cs);
      cs.emit(op(Op::SetCurObject));
      cs.emit(op(Op::SetCurField));
      emitSTE(n->slotName, u, cs);
      cs.emit(op(Op::PopStk));

      if (n->arrayExpr)
      {
         cs.emit(op(Op::SetCurFieldArray));
         cs.emit(op(Op::PopStk));
      }

      switch (req)
      {
      case TypeReq::UInt:   cs.emit(op(Op::LoadFieldUInt)); break;
      case TypeReq::Float:  cs.emit(op(Op::LoadFieldFlt));  break;
      default:              cs.emit(op(Op::LoadFieldStr));  break;
      }
   }

   //-------------------------------------------------------------------
   // Slot visitors
   //-------------------------------------------------------------------
   void CodeGen::visitSlotAssign(SlotAssignNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      visitExpr(n->valueExpr, TypeReq::String, u, cs);
      if (n->arrayExpr) visitExpr(n->arrayExpr, TypeReq::String, u, cs);

      if (n->objectExpr)
      {
         visitExpr(n->objectExpr, TypeReq::String, u, cs);
         cs.emit(op(Op::SetCurObject));
      }
      else
         cs.emit(op(Op::SetCurObjectNew));

      cs.emit(op(Op::SetCurField));
      emitSTE(n->slotName, u, cs);

      if (n->objectExpr) cs.emit(op(Op::PopStk));
      if (n->arrayExpr)
      {
         cs.emit(op(Op::SetCurFieldArray));
         cs.emit(op(Op::PopStk));
      }

      cs.emit(op(Op::SaveFieldStr));

      if (n->typeID != (U32)-1)
      {
         cs.emit(op(Op::SetCurFieldType));
         cs.emit(n->typeID);
      }
      if (req == TypeReq::None) cs.emit(op(Op::PopStk));
   }

   void CodeGen::visitSlotAssignOp(SlotAssignOpNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      OpInfo info = assignOpInfo(n->op);
      visitExpr(n->valueExpr, info.type, u, cs);
      if (n->arrayExpr) visitExpr(n->arrayExpr, TypeReq::String, u, cs);
      visitExpr(n->objectExpr, TypeReq::String, u, cs);
      cs.emit(op(Op::SetCurObject));
      cs.emit(op(Op::SetCurField));
      emitSTE(n->slotName, u, cs);
      cs.emit(op(Op::PopStk));
      if (n->arrayExpr)
      {
         cs.emit(op(Op::SetCurFieldArray));
         if (req == TypeReq::None) cs.emit(op(Op::PopStk));
      }
      cs.emit(op(info.type == TypeReq::Float ? Op::LoadFieldFlt : Op::LoadFieldUInt));
      cs.emit(op(info.opcode));
      cs.emit(op(info.type == TypeReq::Float ? Op::SaveFieldFlt : Op::SaveFieldUInt));
      if (req == TypeReq::None) cs.emit(op(Op::PopStk));
   }

   void CodeGen::visitInternalSlot(InternalSlotNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
      if (req == TypeReq::None) return;
      visitExpr(n->objectExpr, TypeReq::String, u, cs);
      cs.emit(op(Op::SetCurObject));
      cs.emit(op(Op::PopStk));
      visitExpr(n->slotExpr, TypeReq::String, u, cs);
      cs.emit(op(Op::SetCurObjectInternal));
      cs.emit((U32)n->recurse);
   }

   //-------------------------------------------------------------------
   // Assert visitors
   //-------------------------------------------------------------------
   void CodeGen::visitAssert(AssertNode* n, TypeReq req, CompilationUnit& u, CodeStream& cs)
   {
#ifdef TORQUE_ENABLE_SCRIPTASSERTS
      U32 msgIdx = addString(n->message ? n->message : "TorqueScript assert!", u);
      visitExpr(n->testExpr, TypeReq::UInt, u, cs);
      cs.emit(op(Op::Assert));
      cs.emit(msgIdx);
#endif
   }

}
