#include "newConsole/torquescript2/emitter.h"

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      Emitter::Emitter(const CompilationUnit& unit)
         : mUnit(unit)
      {
      }

      void Emitter::reportError(U32 line, const char* message)
      {
         EmitDiagnostic d;
         d.line = line;
         d.message = message;
         mDiagnostics.push_back(d);
      }

      U32 Emitter::emitRaw(OpCode op, Reg a, Reg b, Reg c, U32 bx, U32 line)
      {
         Instruction instr;
         instr.op = op;
         instr.a = a;
         instr.b = b;
         instr.c = c;
         instr.bx = bx;
         mOut.code.push_back(instr);
         mOut.lineTable.push_back(line);

         U32 index = static_cast<U32>(mOut.code.size() - 1);
         if (mNextReg > mOut.registerCount)
            mOut.registerCount = mNextReg;
         return index;
      }

      Reg Emitter::allocTemp()
      {
         Reg r = mNextReg;
         ++mNextReg;
         if (mNextReg > mOut.registerCount)
            mOut.registerCount = mNextReg;
         return r;
      }

      Reg Emitter::clampRestore(Reg candidate) const
      {
         Reg highestBound = 0;
         for (U32 i = 0; i < mScopeStack.size(); ++i)
         {
            const Vector<LocalBinding>& scope = mScopeStack[i];
            for (U32 j = 0; j < scope.size(); ++j)
            {
               Reg onePastReg = static_cast<Reg>(scope[j].reg + 1);
               if (onePastReg > highestBound)
                  highestBound = onePastReg;
            }
         }
         return (highestBound > candidate) ? highestBound : candidate;
      }

      void Emitter::pushLexicalScope()
      {
         mScopeStack.push_back(Vector<LocalBinding>());
      }

      void Emitter::popLexicalScope()
      {
         AssertFatal(!mScopeStack.empty(), "Emitter::popLexicalScope - scope stack underflow");
         const Vector<LocalBinding>& closing = mScopeStack.back();
         U32 closeIndex = static_cast<U32>(mOut.code.size());
         for (U32 i = 0; i < closing.size(); ++i)
         {
            BytecodeUnit::LocalDebugInfo info;
            info.name = closing[i].name;
            info.reg = closing[i].reg;
            info.firstValidInstruction = closing[i].firstInstruction;
            info.lastValidInstruction = closeIndex;
            mOut.localDebugInfo.push_back(info);
         }
         mScopeStack.pop_back();
      }

      Reg Emitter::tryResolveLocal(StringTableEntry name) const
      {
         for (S32 i = static_cast<S32>(mScopeStack.size()) - 1; i >= 0; --i)
         {
            const Vector<LocalBinding>& scope = mScopeStack[i];
            for (U32 j = 0; j < scope.size(); ++j)
            {
               if (scope[j].name == name)
                  return scope[j].reg;
            }
         }
         return static_cast<Reg>(~0);
      }

      Reg Emitter::resolveOrDeclareLocal(StringTableEntry name)
      {
         Reg existing = tryResolveLocal(name);
         if (existing != static_cast<Reg>(~0))
            return existing;

         AssertFatal(!mScopeStack.empty(), "Emitter::resolveOrDeclareLocal - no active scope");
         Reg fresh = allocTemp();
         mScopeStack.back().push_back(LocalBinding{ name, fresh, static_cast<U32>(mOut.code.size()) });
         return fresh;
      }

      U32 Emitter::emitJump(OpCode op, Reg conditionReg)
      {
         AssertFatal(op == OpCode::Jump || op == OpCode::JumpIfFalse || op == OpCode::JumpIfTrue,
            "Emitter::emitJump - not a jump opcode");
         // bx left at 0, patched later via patchJumpToHere once the
         // destination is known.
         return emitRaw(op, conditionReg, 0, 0, 0, 0);
      }

      void Emitter::patchJumpToHere(U32 jumpInstructionIndex)
      {
         AssertFatal(jumpInstructionIndex < static_cast<U32>(mOut.code.size()),
            "Emitter::patchJumpToHere - instruction index out of range");
         mOut.code[jumpInstructionIndex].bx = static_cast<U32>(mOut.code.size());
      }

      U32 Emitter::addIntConst(S64 value)
      {
         for (U32 i = 0; i < mOut.intConsts.size(); ++i)
            if (mOut.intConsts[i] == value)
               return i;
         mOut.intConsts.push_back(value);
         return static_cast<U32>(mOut.intConsts.size() - 1);
      }

      U32 Emitter::addFloatConst(F64 value)
      {
         for (U32 i = 0; i < mOut.floatConsts.size(); ++i)
            if (mOut.floatConsts[i] == value)
               return i;
         mOut.floatConsts.push_back(value);
         return static_cast<U32>(mOut.floatConsts.size() - 1);
      }

      U32 Emitter::addStringConst(StringTableEntry value)
      {
         // StringTableEntry equality is pointer equality (interned), so
         // this is pointer comparisons, not strcmp.
         for (U32 i = 0; i < mOut.stringConsts.size(); ++i)
            if (mOut.stringConsts[i] == value)
               return i;
         mOut.stringConsts.push_back(value);
         return static_cast<U32>(mOut.stringConsts.size() - 1);
      }

      U32 Emitter::addTaggedStringConst(StringTableEntry value)
      {
         for (U32 i = 0; i < mOut.taggedStringConsts.size(); ++i)
            if (mOut.taggedStringConsts[i] == value)
               return i;
         mOut.taggedStringConsts.push_back(value);
         return static_cast<U32>(mOut.taggedStringConsts.size() - 1);
      }

      // =============================================================================
      // Expressions
      // =============================================================================

      static OpCode binaryOpCode(ast::BinaryOp op)
      {
         switch (op)
         {
         case ast::BinaryOp::Add: return OpCode::Add;
         case ast::BinaryOp::Sub: return OpCode::Sub;
         case ast::BinaryOp::Mul: return OpCode::Mul;
         case ast::BinaryOp::Div: return OpCode::Div;
         case ast::BinaryOp::Mod: return OpCode::Mod;
         case ast::BinaryOp::Shl: return OpCode::Shl;
         case ast::BinaryOp::Shr: return OpCode::Shr;
         case ast::BinaryOp::BitAnd: return OpCode::BitAnd;
         case ast::BinaryOp::BitOr:  return OpCode::BitOr;
         case ast::BinaryOp::BitXor: return OpCode::BitXor;
         case ast::BinaryOp::Eq: return OpCode::CmpEq;
         case ast::BinaryOp::Ne: return OpCode::CmpNe;
         case ast::BinaryOp::Lt: return OpCode::CmpLt;
         case ast::BinaryOp::Le: return OpCode::CmpLe;
         case ast::BinaryOp::Gt: return OpCode::CmpGt;
         case ast::BinaryOp::Ge: return OpCode::CmpGe;
            // LogicalAnd/LogicalOr never reach here - emitBinary routes
            // them to emitLogicalAnd/Or for short-circuit lowering first.
         default:
            AssertFatal(false, "binaryOpCode - operator has no direct opcode");
            return OpCode::Nop;
         }
      }

      Reg Emitter::emitExpr(ast::ExprHandle handle)
      {
         const ast::ExprNode& node = mUnit.get(handle);
         U32 line = node.span.line;

         switch (node.kind)
         {
         case ast::ExprKind::IntLiteral:
         {
            Reg r = allocTemp();
            emitRaw(OpCode::LoadInt, r, 0, 0, addIntConst(node.intLiteral.value), line);
            return r;
         }
         case ast::ExprKind::FloatLiteral:
         {
            Reg r = allocTemp();
            emitRaw(OpCode::LoadFloat, r, 0, 0, addFloatConst(node.floatLiteral.value), line);
            return r;
         }
         case ast::ExprKind::StringLiteral:
         {
            Reg r = allocTemp();
            emitRaw(OpCode::LoadString, r, 0, 0, addStringConst(node.stringLiteral.text), line);
            return r;
         }
         case ast::ExprKind::TaggedLiteral:
         {
            Reg r = allocTemp();
            emitRaw(OpCode::LoadTagged, r, 0, 0, addTaggedStringConst(node.taggedLiteral.text), line);
            return r;
         }
         case ast::ExprKind::GlobalVar:
         {
            Reg r = allocTemp();
            emitRaw(OpCode::GetGlobal, r, 0, 0, addStringConst(node.globalVar.name), line);
            return r;
         }
         case ast::ExprKind::LocalVar:
         {
            Reg existing = tryResolveLocal(node.localVar.name);
            if (existing == static_cast<Reg>(~0))
            {
               // Unassigned local reads as empty/null, not an error
               // (see tryResolveLocal).
               Reg r = allocTemp();
               emitRaw(OpCode::LoadNull, r, 0, 0, 0, line);
               return r;
            }
            return existing;
         }
         case ast::ExprKind::Assign:      return emitAssign(node);
         case ast::ExprKind::CompoundAssign: return emitCompoundAssign(node);
         case ast::ExprKind::Binary:
            if (node.binary.op == ast::BinaryOp::LogicalAnd) return emitLogicalAnd(node);
            if (node.binary.op == ast::BinaryOp::LogicalOr)  return emitLogicalOr(node);
            return emitBinary(node);
         case ast::ExprKind::StringConcat:  return emitStringConcat(node);
         case ast::ExprKind::StringCompare: return emitStringCompare(node);
         case ast::ExprKind::Unary:         return emitUnary(node);
         case ast::ExprKind::PreIncDec:     return emitPreIncDec(node);
         case ast::ExprKind::PostIncDec:    return emitPostIncDec(node);
         case ast::ExprKind::Ternary:       return emitTernary(node);
         case ast::ExprKind::FieldAccess:   return emitFieldAccess(node);
         case ast::ExprKind::FieldAssign:   return emitFieldAssign(node);
         case ast::ExprKind::IndexAccess:   return emitIndexAccess(node);
         case ast::ExprKind::IndexAssign:   return emitIndexAssign(node);
         case ast::ExprKind::Call:          return emitCall(node);
         case ast::ExprKind::MethodCall:    return emitMethodCall(node);
         case ast::ExprKind::ObjectDecl:    return emitObjectDecl(node);
         case ast::ExprKind::DocComment:
         {
            // No runtime value - loads null so it still occupies a
            // register if misused as a value.
            Reg r = allocTemp();
            emitRaw(OpCode::LoadNull, r, 0, 0, 0, line);
            return r;
         }
         default:
            reportError(line, "internal error: unhandled expression kind in emitExpr");
            return allocTemp();
         }
      }

      Reg Emitter::emitBinary(const ast::ExprNode& node)
      {
         Reg lhs = emitExpr(node.binary.lhs);
         Reg rhs = emitExpr(node.binary.rhs);
         Reg out = allocTemp();
         emitRaw(binaryOpCode(node.binary.op), out, lhs, rhs, 0, node.span.line);
         return out;
      }

      Reg Emitter::emitLogicalAnd(const ast::ExprNode& node)
      {
         // %a && %b: lowered to a jump (not a plain opcode) so rhs's
         // side effects are skipped when short-circuited.
         Reg out = allocTemp();
         Reg lhs = emitExpr(node.binary.lhs);
         emitRaw(OpCode::MoveReg, out, lhs, 0, 0, node.span.line);
         U32 skipRhs = emitJump(OpCode::JumpIfFalse, out);
         Reg rhs = emitExpr(node.binary.rhs);
         emitRaw(OpCode::MoveReg, out, rhs, 0, 0, node.span.line);
         patchJumpToHere(skipRhs);
         return out;
      }

      Reg Emitter::emitLogicalOr(const ast::ExprNode& node)
      {
         Reg out = allocTemp();
         Reg lhs = emitExpr(node.binary.lhs);
         emitRaw(OpCode::MoveReg, out, lhs, 0, 0, node.span.line);
         U32 skipRhs = emitJump(OpCode::JumpIfTrue, out);
         Reg rhs = emitExpr(node.binary.rhs);
         emitRaw(OpCode::MoveReg, out, rhs, 0, 0, node.span.line);
         patchJumpToHere(skipRhs);
         return out;
      }

      void Emitter::emitStoreTo(ast::ExprHandle targetHandle, Reg valueReg)
      {
         const ast::ExprNode& target = mUnit.get(targetHandle);
         U32 line = target.span.line;

         switch (target.kind)
         {
         case ast::ExprKind::GlobalVar:
            emitRaw(OpCode::SetGlobal, valueReg, 0, 0, addStringConst(target.globalVar.name), line);
            return;
         case ast::ExprKind::LocalVar:
         {
            Reg localReg = resolveOrDeclareLocal(target.localVar.name);
            if (localReg != valueReg)
               emitRaw(OpCode::MoveReg, localReg, valueReg, 0, 0, line);
            return;
         }
         case ast::ExprKind::FieldAccess:
         {
            Reg obj = emitExpr(target.fieldAccess.object);
            OpCode op = target.fieldAccess.isInternal ? OpCode::SetFieldInternal : OpCode::SetField;
            emitRaw(op, valueReg, obj, 0, addStringConst(target.fieldAccess.field), line);
            return;
         }
         case ast::ExprKind::IndexAccess:
         {
            Reg base = emitExpr(target.indexAccess.base);
            Reg index = emitExpr(target.indexAccess.index);
            emitRaw(OpCode::SetIndex, valueReg, base, index, 0, line);
            // SetIndex always writes the possibly-vivified base back to
            // the same register (IInterpreterHost::setIndex contract).
            // Matters only when base was Null and got vivified into a
            // fresh array - recurse into emitStoreTo so that new array
            // lands wherever the base expression came from.
            //
            // Only recurse for base kinds emitStoreTo can actually
            // store to (LocalVar/GlobalVar/FieldAccess/IndexAccess);
            // other kinds (e.g. a call result) have nowhere to write
            // back and never needed one.
            //
            // @note Re-evaluates the base a second time - free for
            //   LocalVar, a real GetGlobal/GetField for others. All
            //   getters here are pure reads, so this is a minor
            //   redundant-read cost, not a correctness issue.
            switch (mUnit.get(target.indexAccess.base).kind)
            {
            case ast::ExprKind::LocalVar:
            case ast::ExprKind::GlobalVar:
            case ast::ExprKind::FieldAccess:
            case ast::ExprKind::IndexAccess:
               emitStoreTo(target.indexAccess.base, base);
               break;
            default:
               break;
            }
            return;
         }
         default:
            reportError(line, "invalid assignment target - not a variable, field, or index expression");
            return;
         }
      }

      Reg Emitter::emitAssign(const ast::ExprNode& node)
      {
         Reg value = emitExpr(node.assign.value);
         emitStoreTo(node.assign.target, value);
         return value;
      }

      Reg Emitter::emitCompoundAssign(const ast::ExprNode& node)
      {
         // lhs OP= rhs desugars to lhs = lhs OP rhs; emitStoreTo handles
         // the actual write regardless of storage kind.
         Reg currentValue = emitExpr(node.compoundAssign.target);
         Reg rhs = emitExpr(node.compoundAssign.value);
         Reg result = allocTemp();
         emitRaw(binaryOpCode(node.compoundAssign.op), result, currentValue, rhs, 0, node.span.line);
         emitStoreTo(node.compoundAssign.target, result);
         return result;
      }

      Reg Emitter::emitStringConcat(const ast::ExprNode& node)
      {
         Reg lhs = emitExpr(node.stringConcat.lhs);
         Reg rhs = emitExpr(node.stringConcat.rhs);
         Reg out = allocTemp();
         // separatorChar packed into bx's low byte - see bytecode.h's
         // Concat opcode comment.
         emitRaw(OpCode::Concat, out, lhs, rhs, static_cast<U32>(static_cast<U8>(node.stringConcat.separatorChar)), node.span.line);
         return out;
      }

      Reg Emitter::emitStringCompare(const ast::ExprNode& node)
      {
         Reg lhs = emitExpr(node.stringCompare.lhs);
         Reg rhs = emitExpr(node.stringCompare.rhs);
         Reg out = allocTemp();
         emitRaw(node.stringCompare.negated ? OpCode::StrNe : OpCode::StrEq, out, lhs, rhs, 0, node.span.line);
         return out;
      }

      Reg Emitter::emitUnary(const ast::ExprNode& node)
      {
         Reg operand = emitExpr(node.unary.operand);
         Reg out = allocTemp();
         OpCode op = (node.unary.op == ast::UnaryOp::Negate) ? OpCode::Negate
            : (node.unary.op == ast::UnaryOp::LogicalNot) ? OpCode::LogicalNot
            : OpCode::BitNot;
         emitRaw(op, out, operand, 0, 0, node.span.line);
         return out;
      }

      Reg Emitter::emitPreIncDec(const ast::ExprNode& node)
      {
         // ++x/--x: bump in place, store back, result is the new value.
         Reg current = emitExpr(node.preIncDec.target);
         emitRaw(node.preIncDec.isIncrement ? OpCode::IncReg : OpCode::DecReg, current, 0, 0, 0, node.span.line);
         emitStoreTo(node.preIncDec.target, current);
         return current;
      }

      Reg Emitter::emitPostIncDec(const ast::ExprNode& node)
      {
         // x++/x--: result is the pre-bump value, so it's copied out
         // before IncReg/DecReg mutates the original.
         Reg current = emitExpr(node.postIncDec.target);
         Reg savedOldValue = allocTemp();
         emitRaw(OpCode::MoveReg, savedOldValue, current, 0, 0, node.span.line);
         emitRaw(node.postIncDec.isIncrement ? OpCode::IncReg : OpCode::DecReg, current, 0, 0, 0, node.span.line);
         emitStoreTo(node.postIncDec.target, current);
         return savedOldValue;
      }

      Reg Emitter::emitTernary(const ast::ExprNode& node)
      {
         Reg out = allocTemp();
         Reg cond = emitExpr(node.ternary.cond);
         U32 jumpToFalse = emitJump(OpCode::JumpIfFalse, cond);

         Reg trueVal = emitExpr(node.ternary.whenTrue);
         emitRaw(OpCode::MoveReg, out, trueVal, 0, 0, node.span.line);
         U32 jumpPastFalse = emitJump(OpCode::Jump);

         patchJumpToHere(jumpToFalse);
         Reg falseVal = emitExpr(node.ternary.whenFalse);
         emitRaw(OpCode::MoveReg, out, falseVal, 0, 0, node.span.line);

         patchJumpToHere(jumpPastFalse);
         return out;
      }

      Reg Emitter::emitFieldAccess(const ast::ExprNode& node)
      {
         Reg obj = emitExpr(node.fieldAccess.object);
         Reg out = allocTemp();
         OpCode op = node.fieldAccess.isInternal ? OpCode::GetFieldInternal : OpCode::GetField;
         emitRaw(op, out, obj, 0, addStringConst(node.fieldAccess.field), node.span.line);
         return out;
      }

      Reg Emitter::emitFieldAssign(const ast::ExprNode& node)
      {
         Reg obj = emitExpr(node.fieldAssign.object);
         Reg value = emitExpr(node.fieldAssign.value);
         OpCode op = node.fieldAssign.isInternal ? OpCode::SetFieldInternal : OpCode::SetField;
         emitRaw(op, value, obj, 0, addStringConst(node.fieldAssign.field), node.span.line);
         return value;
      }

      Reg Emitter::emitIndexAccess(const ast::ExprNode& node)
      {
         Reg base = emitExpr(node.indexAccess.base);
         Reg index = emitExpr(node.indexAccess.index);
         Reg out = allocTemp();
         emitRaw(OpCode::GetIndex, out, base, index, 0, node.span.line);
         return out;
      }

      Reg Emitter::emitIndexAssign(const ast::ExprNode& node)
      {
         Reg base = emitExpr(node.indexAssign.base);
         Reg index = emitExpr(node.indexAssign.index);
         Reg value = emitExpr(node.indexAssign.value);
         emitRaw(OpCode::SetIndex, value, base, index, 0, node.span.line);
         // Same write-back recursion as emitStoreTo's IndexAccess case
         // (not shared via a helper since the two functions differ in
         // what they return).
         switch (mUnit.get(node.indexAssign.base).kind)
         {
         case ast::ExprKind::LocalVar:
         case ast::ExprKind::GlobalVar:
         case ast::ExprKind::FieldAccess:
         case ast::ExprKind::IndexAccess:
            emitStoreTo(node.indexAssign.base, base);
            break;
         default:
            break;
         }
         return value;
      }

      Reg Emitter::emitCall(const ast::ExprNode& node)
      {
         // Call's a operand is the return-value/first-arg register; args
         // fill consecutive registers from there (see bytecode.h).
         //
         // base is allocated OUTSIDE the RegisterScope below - it holds
         // the result and must survive this function's return. Getting
         // this backwards was a real bug: the scope's destructor would
         // reclaim base's register immediately, colliding with the next
         // temp the caller allocated.
         Reg base = allocTemp();

         const ast::ExprHandle* args = CompilationUnit::listData(mUnit.exprList, node.call.args);
         for (U32 i = 0; i < node.call.args.count; ++i)
         {
            Reg argReg = emitExpr(args[i]);
            Reg target = static_cast<Reg>(base + 1 + i);
            if (argReg != target)
               emitRaw(OpCode::MoveReg, target, argReg, 0, 0, node.span.line);

            // target is now spoken for and must not be reused by a
            // later argument's temp allocation - advancing mNextReg
            // here is what guarantees that. Missing this was a real bug
            // (hostAdd(%x, 10) evaluated "10" into the register %x had
            // just been moved into).
            if (mNextReg <= target)
               mNextReg = static_cast<Reg>(target + 1);
         }
         // Reset past the argument registers - sub-expression temporaries
         // above are discarded; Call only reads base..base+argCount.
         mNextReg = static_cast<Reg>(base + 1 + node.call.args.count);

         if (node.call.namespaceName)
         {
            U32 nameConst = addStringConst(node.call.functionName);
            U32 nsConst = addStringConst(node.call.namespaceName);
            emitRaw(OpCode::CallNamespaced, base, static_cast<Reg>(node.call.args.count), 0, (nsConst << 16) | (nameConst & 0xFFFF), node.span.line);
         }
         else
         {
            emitRaw(OpCode::Call, base, static_cast<Reg>(node.call.args.count), 0, addStringConst(node.call.functionName), node.span.line);
         }

         // Result lands back in `base` by convention; returned rather
         // than reused, since RegisterScope above will reclaim the
         // argument registers once this expression's result is consumed.
         return base;
      }

      Reg Emitter::emitMethodCall(const ast::ExprNode& node)
      {
         // Same reasoning as emitCall: objReg and base both need to
         // survive past this function, so neither is inside a
         // RegisterScope that would reclaim them early.
         Reg objReg = emitExpr(node.methodCall.object);
         Reg base = allocTemp();

         const ast::ExprHandle* args = CompilationUnit::listData(mUnit.exprList, node.methodCall.args);
         for (U32 i = 0; i < node.methodCall.args.count; ++i)
         {
            Reg argReg = emitExpr(args[i]);
            Reg target = static_cast<Reg>(base + 1 + i);
            if (argReg != target)
               emitRaw(OpCode::MoveReg, target, argReg, 0, 0, node.span.line);

            // See emitCall's identical fix and comment.
            if (mNextReg <= target)
               mNextReg = static_cast<Reg>(target + 1);
         }
         mNextReg = static_cast<Reg>(base + 1 + node.methodCall.args.count);

         emitRaw(OpCode::MethodCall, base, objReg, static_cast<Reg>(node.methodCall.args.count),
            addStringConst(node.methodCall.methodName), node.span.line);
         return base;
      }

      Reg Emitter::emitObjectDecl(const ast::ExprNode& node)
      {
         const ast::ObjectDeclExpr& decl = node.objectDecl;

         BytecodeUnit::ObjectDeclTemplate tmpl;
         tmpl.parentName = decl.parentName;
         tmpl.isDatablock = decl.isDatablock;
         tmpl.isSingleton = decl.isSingleton;
         tmpl.isArrayElement = decl.isArrayElement;

         const ast::ExprNode& classNameNode = mUnit.get(decl.classNameExpr);
         const ast::ExprNode& objectNameNode = mUnit.get(decl.objectNameExpr);

         // Dynamic name expressions ("new (getClassName())(...)") are
         // evaluated into registers before NewObject emits; NewObject
         // then reads classNameReg/objectNameReg instead of the pool.
         if (classNameNode.kind == ast::ExprKind::StringLiteral)
         {
            tmpl.classNameConstIndex = addStringConst(classNameNode.stringLiteral.text);
         }
         else
         {
            tmpl.classNameIsDynamic = true;
            tmpl.classNameReg = emitExpr(decl.classNameExpr);
         }

         if (objectNameNode.kind == ast::ExprKind::StringLiteral)
         {
            tmpl.objectNameConstIndex = addStringConst(objectNameNode.stringLiteral.text);
         }
         else
         {
            tmpl.objectNameIsDynamic = true;
            tmpl.objectNameReg = emitExpr(decl.objectNameExpr);
         }

         mOut.objectDecls.push_back(tmpl);
         U32 templateIndex = static_cast<U32>(mOut.objectDecls.size() - 1);

         Reg out = allocTemp();
         emitRaw(OpCode::NewObject, out, 0, 0, templateIndex, node.span.line);

         // Slot assignments and child decls execute against the new
         // object in source order - each is a SetField against `out`,
         // reusing emitStoreTo's FieldAccess path.
         const ast::SlotAssignment* slots = CompilationUnit::listData(mUnit.slotList, decl.slotAssignments);
         for (U32 i = 0; i < decl.slotAssignments.count; ++i)
         {
            Reg value = emitExpr(slots[i].value);
            emitRaw(OpCode::SetField, value, out, 0, addStringConst(slots[i].slotName), node.span.line);
         }

         const ast::ExprHandle* children = CompilationUnit::listData(mUnit.exprList, decl.childDecls);
         for (U32 i = 0; i < decl.childDecls.count; ++i)
         {
            // Child's construction result is discarded - it attaches to
            // `out` as part of construction; only its side effect matters.
            emitExpr(children[i]);
         }

         return out;
      }

      // =============================================================================
      // Statements
      // =============================================================================

      void Emitter::emitStmt(ast::StmtHandle handle)
      {
         const ast::StmtNode& node = mUnit.get(handle);
         switch (node.kind)
         {
         case ast::StmtKind::ExprStmt:
         {
            RegisterScope scope(*this); // result value unused at statement position
            emitExpr(node.exprStmt.expr);
            return;
         }
         case ast::StmtKind::Block:      emitBlock(node); return;
         case ast::StmtKind::If:         emitIf(node); return;
         case ast::StmtKind::While:      emitWhile(node); return;
         case ast::StmtKind::DoWhile:    emitDoWhile(node); return;
         case ast::StmtKind::For:        emitFor(node); return;
         case ast::StmtKind::Foreach:    emitForeach(node); return;
         case ast::StmtKind::Switch:     emitSwitch(node); return;
         case ast::StmtKind::Break:      emitBreak(node); return;
         case ast::StmtKind::Continue:   emitContinue(node); return;
         case ast::StmtKind::Return:     emitReturn(node); return;
         case ast::StmtKind::FunctionDecl:
         case ast::StmtKind::PackageDecl:
            // Not valid nested in a function body; the parser doesn't
            // currently reject this at parse time, so this is a
            // defensive check against a future parser change.
            reportError(node.span.line, "function/package declarations are only valid at file scope");
            return;
         default:
            reportError(node.span.line, "internal error: unhandled statement kind in emitStmt");
            return;
         }
      }

      void Emitter::emitBlock(const ast::StmtNode& node)
      {
         pushLexicalScope();
         RegisterScope regScope(*this);

         const ast::StmtHandle* stmts = CompilationUnit::listData(mUnit.stmtList, node.block.statements);
         for (U32 i = 0; i < node.block.statements.count; ++i)
            emitStmt(stmts[i]);

         popLexicalScope();
      }

      void Emitter::emitIf(const ast::StmtNode& node)
      {
         Reg cond;
         {
            RegisterScope condScope(*this);
            cond = emitExpr(node.ifStmt.condition);
            U32 jumpToElse = emitJump(OpCode::JumpIfFalse, cond);

            emitStmt(node.ifStmt.thenBranch);

            if (node.ifStmt.elseBranch.isValid())
            {
               U32 jumpPastElse = emitJump(OpCode::Jump);
               patchJumpToHere(jumpToElse);
               emitStmt(node.ifStmt.elseBranch);
               patchJumpToHere(jumpPastElse);
            }
            else
            {
               patchJumpToHere(jumpToElse);
            }
         }
      }

      void Emitter::emitWhile(const ast::StmtNode& node)
      {
         U32 loopStart = static_cast<U32>(mOut.code.size());
         mLoopStack.push_back(LoopContext());

         Reg cond;
         {
            RegisterScope condScope(*this);
            cond = emitExpr(node.whileStmt.condition);
            U32 exitJump = emitJump(OpCode::JumpIfFalse, cond);

            emitStmt(node.whileStmt.body);

            // continue re-checks the condition, i.e. targets loopStart -
            // patched directly since the destination is already known.
            LoopContext& ctx = mLoopStack.back();
            for (U32 i = 0; i < ctx.continueJumps.size(); ++i)
               mOut.code[ctx.continueJumps[i]].bx = loopStart;

            emitRaw(OpCode::Jump, 0, 0, 0, loopStart, node.span.line);
            patchJumpToHere(exitJump);

            for (U32 i = 0; i < ctx.breakJumps.size(); ++i)
               mOut.code[ctx.breakJumps[i]].bx = static_cast<U32>(mOut.code.size());
         }

         mLoopStack.pop_back();
      }

      void Emitter::emitDoWhile(const ast::StmtNode& node)
      {
         U32 loopStart = static_cast<U32>(mOut.code.size());
         mLoopStack.push_back(LoopContext());

         emitStmt(node.doWhileStmt.body);

         U32 conditionCheckStart = static_cast<U32>(mOut.code.size());
         {
            RegisterScope condScope(*this);
            Reg cond = emitExpr(node.doWhileStmt.condition);
            emitRaw(OpCode::JumpIfTrue, cond, 0, 0, loopStart, node.span.line);
         }

         LoopContext& ctx = mLoopStack.back();
         // continue re-checks the condition rather than jumping straight
         // to loopStart, which would skip the check and loop forever.
         for (U32 i = 0; i < ctx.continueJumps.size(); ++i)
            mOut.code[ctx.continueJumps[i]].bx = conditionCheckStart;
         for (U32 i = 0; i < ctx.breakJumps.size(); ++i)
            mOut.code[ctx.breakJumps[i]].bx = static_cast<U32>(mOut.code.size());

         mLoopStack.pop_back();
      }

      void Emitter::emitFor(const ast::StmtNode& node)
      {
         pushLexicalScope();
         RegisterScope outerScope(*this);

         if (node.forStmt.init.isValid())
            emitStmt(node.forStmt.init);

         U32 conditionStart = static_cast<U32>(mOut.code.size());
         U32 exitJump = static_cast<U32>(~0);
         if (node.forStmt.condition.isValid())
         {
            RegisterScope condScope(*this);
            Reg cond = emitExpr(node.forStmt.condition);
            exitJump = emitJump(OpCode::JumpIfFalse, cond);
         }

         mLoopStack.push_back(LoopContext());
         emitStmt(node.forStmt.body);

         U32 incrementStart = static_cast<U32>(mOut.code.size());
         if (node.forStmt.increment.isValid())
         {
            RegisterScope incScope(*this);
            emitExpr(node.forStmt.increment);
         }
         emitRaw(OpCode::Jump, 0, 0, 0, conditionStart, node.span.line);

         U32 loopEnd = static_cast<U32>(mOut.code.size());
         if (exitJump != static_cast<U32>(~0))
            mOut.code[exitJump].bx = loopEnd;

         LoopContext& ctx = mLoopStack.back();
         for (U32 i = 0; i < ctx.continueJumps.size(); ++i)
            mOut.code[ctx.continueJumps[i]].bx = incrementStart;
         for (U32 i = 0; i < ctx.breakJumps.size(); ++i)
            mOut.code[ctx.breakJumps[i]].bx = loopEnd;
         mLoopStack.pop_back();

         popLexicalScope();
      }

      void Emitter::emitForeach(const ast::StmtNode& node)
      {
         pushLexicalScope();
         RegisterScope outerScope(*this);

         Reg collection = emitExpr(node.foreachStmt.collection);
         Reg iterator = allocTemp();
         U32 iterFormBit = node.foreachStmt.isStringForm ? 1u : 0u;
         emitRaw(OpCode::IterBegin, iterator, collection, 0, iterFormBit, node.span.line);

         // Loop variable gets one register, bound in the body's scope;
         // overwritten each IterNext rather than re-declared per iteration.
         Reg loopVarReg = resolveOrDeclareLocal(node.foreachStmt.loopVar);

         U32 loopStart = static_cast<U32>(mOut.code.size());
         U32 exitJump = emitRaw(OpCode::IterNext, loopVarReg, iterator, 0, 0, node.span.line);
         // IterNext's own bx IS the exit target (see bytecode.h) - patched below.

         mLoopStack.push_back(LoopContext());
         emitStmt(node.foreachStmt.body);

         LoopContext& ctx = mLoopStack.back();
         for (U32 i = 0; i < ctx.continueJumps.size(); ++i)
            mOut.code[ctx.continueJumps[i]].bx = loopStart;

         emitRaw(OpCode::Jump, 0, 0, 0, loopStart, node.span.line);
         U32 loopEnd = static_cast<U32>(mOut.code.size());
         mOut.code[exitJump].bx = loopEnd;

         // break must reach IterEnd, not skip past it, or the loop leaks
         // whatever IterBegin allocated. Both natural exit and every
         // break land here, before IterEnd runs.
         for (U32 i = 0; i < ctx.breakJumps.size(); ++i)
            mOut.code[ctx.breakJumps[i]].bx = loopEnd;

         emitRaw(OpCode::IterEnd, iterator, 0, 0, 0, node.span.line);
         mLoopStack.pop_back();

         popLexicalScope();
      }

      void Emitter::emitSwitch(const ast::StmtNode& node)
      {
         RegisterScope outerScope(*this);
         Reg subject = emitExpr(node.switchStmt.subject);

         mLoopStack.push_back(LoopContext()); // switch supports 'break' the same way a loop does

         const ast::SwitchCase* cases = CompilationUnit::listData(mUnit.caseList, node.switchStmt.cases);
         Vector<U32> fallthroughToNextTestJumps; // unused placeholder kept empty - see note below
         Vector<U32> caseBodyStarts;
         Vector<U32> testFailJumps;

         // Comparison chain first: for each case, test subject against
         // each OR'd value, jump to the body on any match, else fall
         // through to the next case's test.
         for (U32 i = 0; i < node.switchStmt.cases.count; ++i)
         {
            const ast::ExprHandle* values = CompilationUnit::listData(mUnit.exprList, cases[i].valueList);
            Vector<U32> matchJumps;
            for (U32 v = 0; v < cases[i].valueList.count; ++v)
            {
               RegisterScope testScope(*this);
               Reg valueReg = emitExpr(values[v]);
               Reg cmp = allocTemp();
               OpCode cmpOp = node.switchStmt.isStringForm ? OpCode::StrEq : OpCode::CmpEq;
               emitRaw(cmpOp, cmp, subject, valueReg, 0, node.span.line);
               matchJumps.push_back(emitJump(OpCode::JumpIfTrue, cmp));
            }
            U32 fallToNextCase = emitJump(OpCode::Jump);

            U32 bodyStart = static_cast<U32>(mOut.code.size());
            for (U32 j = 0; j < matchJumps.size(); ++j)
               patchJumpToHere(matchJumps[j]);

            const ast::StmtHandle* body = CompilationUnit::listData(mUnit.stmtList, cases[i].body);
            for (U32 s = 0; s < cases[i].body.count; ++s)
               emitStmt(body[s]);
            // No fallthrough between cases - each body jumps to the end
            // of the switch (case/break semantics, not C fallthrough).
            U32 jumpToEnd = emitJump(OpCode::Jump);
            caseBodyStarts.push_back(bodyStart);
            testFailJumps.push_back(jumpToEnd);

            patchJumpToHere(fallToNextCase);
         }

         const ast::StmtHandle* defaultBody = CompilationUnit::listData(mUnit.stmtList, node.switchStmt.defaultBody);
         for (U32 s = 0; s < node.switchStmt.defaultBody.count; ++s)
            emitStmt(defaultBody[s]);

         U32 switchEnd = static_cast<U32>(mOut.code.size());
         for (U32 i = 0; i < testFailJumps.size(); ++i)
            mOut.code[testFailJumps[i]].bx = switchEnd;

         LoopContext& ctx = mLoopStack.back();
         for (U32 i = 0; i < ctx.breakJumps.size(); ++i)
            mOut.code[ctx.breakJumps[i]].bx = switchEnd;
         AssertFatal(ctx.continueJumps.empty(), "Emitter::emitSwitch - continue inside switch should have been rejected by emitContinue");
         mLoopStack.pop_back();
      }

      void Emitter::emitBreak(const ast::StmtNode& node)
      {
         if (mLoopStack.empty())
         {
            reportError(node.span.line, "'break' used outside of any loop or switch");
            return;
         }
         U32 jump = emitJump(OpCode::Jump);
         mLoopStack.back().breakJumps.push_back(jump);
      }

      void Emitter::emitContinue(const ast::StmtNode& node)
      {
         if (mLoopStack.empty())
         {
            reportError(node.span.line, "'continue' used outside of any loop");
            return;
         }
         // continue inside a switch's case body targets the nearest
         // enclosing *loop*, not the switch - a switch's own LoopContext
         // never has continueJumps patched (see emitSwitch's assert), so
         // continue with no enclosing loop hits that assert rather than
         // silently doing nothing. Worth a dedicated test.
         U32 jump = emitJump(OpCode::Jump);
         mLoopStack.back().continueJumps.push_back(jump);
      }

      void Emitter::emitReturn(const ast::StmtNode& node)
      {
         if (node.returnStmt.value.isValid())
         {
            RegisterScope scope(*this);
            Reg value = emitExpr(node.returnStmt.value);
            emitRaw(OpCode::Return, value, 0, 0, 0, node.span.line);
         }
         else
         {
            emitRaw(OpCode::ReturnNull, 0, 0, 0, 0, node.span.line);
         }
      }

      // =============================================================================
      // Top-level compile entry points
      // =============================================================================

      BytecodeUnit Emitter::compileFunction(const ast::FunctionDeclStmt& fn)
      {
         mOut = BytecodeUnit();
         mOut.name = fn.functionName;
         mNextReg = 0;
         mScopeStack.clear();
         mLoopStack.clear();

         pushLexicalScope();

         const ast::Param* params = CompilationUnit::listData(mUnit.paramList, fn.params);
         Vector<Reg> paramRegs;
         for (U32 i = 0; i < fn.params.count; ++i)
         {
            Reg paramReg = allocTemp();
            mScopeStack.back().push_back(LocalBinding{ params[i].name, paramReg, 0 });
            paramRegs.push_back(paramReg);
         }
         mOut.paramCount = static_cast<U16>(fn.params.count);

         // Default parameter values: for each defaulted param, compare
         // actual arg count against its position; if not supplied,
         // evaluate and assign the default - as if the body opened with
         // `if (%argCount <= i) %param = default;`. Emitted after all
         // param registers are allocated (so later defaults can
         // reference earlier params), before the real body.
         for (U32 i = 0; i < fn.params.count; ++i)
         {
            if (!params[i].hasDefault)
               continue;

            RegisterScope defaultScope(*this);
            Reg argCount = allocTemp();
            emitRaw(OpCode::GetArgCount, argCount, 0, 0, 0, 0);
            Reg threshold = allocTemp();
            emitRaw(OpCode::LoadInt, threshold, 0, 0, addIntConst(static_cast<S64>(i + 1)), 0);
            Reg hasArg = allocTemp();
            emitRaw(OpCode::CmpGe, hasArg, argCount, threshold, 0, 0);
            U32 skipDefault = emitJump(OpCode::JumpIfTrue, hasArg);

            Reg defaultValue = emitExpr(params[i].defaultValue);
            if (defaultValue != paramRegs[i])
               emitRaw(OpCode::MoveReg, paramRegs[i], defaultValue, 0, 0, 0);

            patchJumpToHere(skipDefault);
         }

         const ast::StmtHandle* body = CompilationUnit::listData(mUnit.stmtList, fn.body);
         for (U32 i = 0; i < fn.body.count; ++i)
            emitStmt(body[i]);

         // Implicit `return null;` if the body falls off the end.
         emitRaw(OpCode::ReturnNull, 0, 0, 0, 0, 0);

         popLexicalScope();
         mOut.origin = mUnit.originName;
         return std::move(mOut);
      }

      BytecodeUnit Emitter::compileTopLevel()
      {
         mOut = BytecodeUnit();
         mOut.name = mUnit.originName;
         mNextReg = 0;
         mScopeStack.clear();
         mLoopStack.clear();

         pushLexicalScope();

         const ast::StmtHandle* top = CompilationUnit::listData(mUnit.stmtList, mUnit.topLevel);
         for (U32 i = 0; i < mUnit.topLevel.count; ++i)
         {
            const ast::StmtNode& stmt = mUnit.get(top[i]);
            if (stmt.kind == ast::StmtKind::FunctionDecl || stmt.kind == ast::StmtKind::PackageDecl)
               continue; // compiled separately, one BytecodeUnit per function - see compileFunction
            emitStmt(top[i]);
         }

         emitRaw(OpCode::ReturnNull, 0, 0, 0, 0, 0);

         popLexicalScope();
         mOut.origin = mUnit.originName;
         return std::move(mOut);
      }

   } // namespace ts2
} // namespace newConsole
