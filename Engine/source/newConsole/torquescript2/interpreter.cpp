#include "newConsole/torquescript2/interpreter.h"

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

      Interpreter::Interpreter(IInterpreterHost& host)
         : mHost(host)
      {
      }

      void Interpreter::reportFatal(U32 line, const char* message)
      {
         mHadFatalError = true;
         mLastError.line = line;
         mLastError.message = message;
      }

      ScriptValue Interpreter::run(std::shared_ptr<const BytecodeUnit> unit, ScriptValueSpan args)
      {
         mHadFatalError = false;
         mCallDepth = 0;

         if (!unit)
         {
            reportFatal(0, "Interpreter::run - null BytecodeUnit");
            return ScriptValue::makeError("null function");
         }

         CallFrame frame(unit);
         frame.argCount = args.size();
         for (U32 i = 0; i < unit->paramCount && i < args.size(); ++i)
            frame.registers[i] = args[i];

         return executeFrame(frame);
      }

      static bool truthy(const ScriptValue& v)
      {
         switch (v.kind())
         {
         case ScriptValue::Kind::Null:  return false;
         case ScriptValue::Kind::Bool: { bool b = false; v.tryGet<bool>(b); return b; }
         case ScriptValue::Kind::Int: { S64 i = 0; v.tryGet<S64>(i); return i != 0; }
         case ScriptValue::Kind::Float: { F64 f = 0; v.tryGet<F64>(f); return f != 0.0; }
         case ScriptValue::Kind::String:
         {
            const char* s = "";
            v.tryGet<const char*>(s);
            return s && s[0] != '\0';
         }
         case ScriptValue::Kind::Object: return true; // a null handle is Kind::Null, not a zeroed Object
         case ScriptValue::Kind::Array:  return true;
         case ScriptValue::Kind::Struct: return true; // same as Array - always exists once constructed
         case ScriptValue::Kind::Error:  return false;
         default: return false;
         }
      }

      /// Numeric OP dispatch shared by Add/Sub/Mul - widens to F64 if
      /// either operand is float, else stays S64. Never falls back to
      /// string concat/parsing on mismatched kinds; a non-numeric
      /// operand reads as 0, same as an unset local elsewhere in this VM.
      template<typename IntOp, typename FloatOp>
      static ScriptValue numericBinaryOp(const ScriptValue& a, const ScriptValue& b, IntOp intOp, FloatOp floatOp)
      {
         bool aIsFloat = a.kind() == ScriptValue::Kind::Float;
         bool bIsFloat = b.kind() == ScriptValue::Kind::Float;

         if (aIsFloat || bIsFloat)
         {
            F64 va = 0.0, vb = 0.0;
            a.tryGet<F64>(va);
            b.tryGet<F64>(vb);
            return ScriptValue::makeFloat(floatOp(va, vb));
         }

         S64 ia = 0, ib = 0;
         a.tryGet<S64>(ia);
         b.tryGet<S64>(ib);
         return ScriptValue::makeInt(intOp(ia, ib));
      }

      static ScriptValue compareOp(const ScriptValue& a, const ScriptValue& b, OpCode op)
      {
         bool useFloat = (a.kind() == ScriptValue::Kind::Float || b.kind() == ScriptValue::Kind::Float);

         bool result = false;
         if (useFloat)
         {
            F64 fa = 0.0, fb = 0.0;
            a.tryGet<F64>(fa);
            b.tryGet<F64>(fb);
            switch (op)
            {
            case OpCode::CmpEq: result = fa == fb; break;
            case OpCode::CmpNe: result = fa != fb; break;
            case OpCode::CmpLt: result = fa < fb; break;
            case OpCode::CmpLe: result = fa <= fb; break;
            case OpCode::CmpGt: result = fa > fb; break;
            case OpCode::CmpGe: result = fa >= fb; break;
            default: break;
            }
         }
         else
         {
            S64 ia = 0, ib = 0;
            a.tryGet<S64>(ia);
            b.tryGet<S64>(ib);
            switch (op)
            {
            case OpCode::CmpEq: result = ia == ib; break;
            case OpCode::CmpNe: result = ia != ib; break;
            case OpCode::CmpLt: result = ia < ib; break;
            case OpCode::CmpLe: result = ia <= ib; break;
            case OpCode::CmpGt: result = ia > ib; break;
            case OpCode::CmpGe: result = ia >= ib; break;
            default: break;
            }
         }
         return ScriptValue::makeBool(result);
      }

      ScriptValue Interpreter::executeFrame(CallFrame& frame)
      {
         const BytecodeUnit& unit = *frame.unit;
         Vector<ScriptValue>& R = frame.registers;

         mCallStack.push_back(&frame);
         struct StackGuard
         {
            Vector<CallFrame*>& stack;
            ~StackGuard() { stack.pop_back(); }
         } stackGuard{ mCallStack };

         for (;;)
         {
            if (frame.ip >= unit.code.size())
            {
               reportFatal(0, "Interpreter::executeFrame - fell off the end of the instruction stream without a Return");
               return ScriptValue::makeError("malformed bytecode: missing return");
            }

            const Instruction& instr = unit.code[frame.ip];
            U32 line = (frame.ip < unit.lineTable.size()) ? unit.lineTable[frame.ip] : 0;
            U32 nextIp = frame.ip + 1;

            if (line != frame.lastBreakLine)
            {
               if (mDebugHook && unit.origin && mDebugHook->shouldBreak(unit.origin, line))
               {
                  mDebugHook->suspend(mCallStack);
                  frame.lastBreakLine = line;
               }
            }

            switch (instr.op)
            {
            case OpCode::LoadInt:    R[instr.a] = ScriptValue::makeInt(unit.intConsts[instr.bx]); break;
            case OpCode::LoadFloat:  R[instr.a] = ScriptValue::makeFloat(unit.floatConsts[instr.bx]); break;
            case OpCode::LoadString: R[instr.a] = ScriptValue::makeString(unit.stringConsts[instr.bx]); break;
            case OpCode::LoadTagged: R[instr.a] = ScriptValue::makeString(unit.taggedStringConsts[instr.bx]); break;
            case OpCode::LoadNull:   R[instr.a] = ScriptValue::makeNull(); break;

            case OpCode::MoveReg:    R[instr.a] = R[instr.b]; break;
            case OpCode::GetGlobal:  R[instr.a] = mHost.getGlobal(unit.stringConsts[instr.bx]); break;
            case OpCode::SetGlobal:  mHost.setGlobal(unit.stringConsts[instr.bx], R[instr.a]); break;
            case OpCode::GetArgCount:
               // Actual supplied-argument count, not unit.paramCount -
               // see CallFrame::argCount.
               R[instr.a] = ScriptValue::makeInt(static_cast<S64>(frame.argCount));
               break;

            case OpCode::GetField:
               R[instr.a] = mHost.getField(R[instr.b], unit.stringConsts[instr.bx], false);
               break;
            case OpCode::SetField:
               mHost.setField(R[instr.b], unit.stringConsts[instr.bx], false, R[instr.a]);
               break;
            case OpCode::GetFieldInternal:
               R[instr.a] = mHost.getField(R[instr.b], unit.stringConsts[instr.bx], true);
               break;
            case OpCode::SetFieldInternal:
               mHost.setField(R[instr.b], unit.stringConsts[instr.bx], true, R[instr.a]);
               break;
            case OpCode::GetIndex:
               R[instr.a] = mHost.getIndex(R[instr.b], R[instr.c]);
               break;
            case OpCode::SetIndex:
               // Writes the (possibly vivified) base back to R[instr.b] -
               // see IInterpreterHost::setIndex. Only the register-level
               // half of vivification; the Emitter recursively stores it
               // back further (emitStoreTo's IndexAccess case).
               R[instr.b] = mHost.setIndex(R[instr.b], R[instr.c], R[instr.a]);
               break;

            case OpCode::Add: R[instr.a] = numericBinaryOp(R[instr.b], R[instr.c], [](S64 x, S64 y) { return x + y; }, [](F64 x, F64 y) { return x + y; }); break;
            case OpCode::Sub: R[instr.a] = numericBinaryOp(R[instr.b], R[instr.c], [](S64 x, S64 y) { return x - y; }, [](F64 x, F64 y) { return x - y; }); break;
            case OpCode::Mul: R[instr.a] = numericBinaryOp(R[instr.b], R[instr.c], [](S64 x, S64 y) { return x * y; }, [](F64 x, F64 y) { return x * y; }); break;
            case OpCode::Div:
            {
               bool divByZero = false;
               if (R[instr.c].kind() == ScriptValue::Kind::Float)
               {
                  F64 fb = 0.0; R[instr.c].tryGet<F64>(fb);
                  divByZero = (fb == 0.0);
               }
               else
               {
                  S64 ib = 0; R[instr.c].tryGet<S64>(ib);
                  divByZero = (ib == 0);
               }
               if (divByZero)
               {
                  reportFatal(line, "division by zero");
                  return ScriptValue::makeError("division by zero");
               }
               R[instr.a] = numericBinaryOp(R[instr.b], R[instr.c], [](S64 x, S64 y) { return x / y; }, [](F64 x, F64 y) { return x / y; });
               break;
            }
            case OpCode::Mod:
            {
               S64 ib = 0; R[instr.c].tryGet<S64>(ib);
               if (ib == 0) { reportFatal(line, "modulo by zero"); return ScriptValue::makeError("modulo by zero"); }
               S64 ia = 0; R[instr.b].tryGet<S64>(ia);
               R[instr.a] = ScriptValue::makeInt(ia % ib);
               break;
            }
            case OpCode::Shl: { S64 ia = 0, ib = 0; R[instr.b].tryGet<S64>(ia); R[instr.c].tryGet<S64>(ib); R[instr.a] = ScriptValue::makeInt(ia << ib); break; }
            case OpCode::Shr: { S64 ia = 0, ib = 0; R[instr.b].tryGet<S64>(ia); R[instr.c].tryGet<S64>(ib); R[instr.a] = ScriptValue::makeInt(ia >> ib); break; }
            case OpCode::BitAnd: { S64 ia = 0, ib = 0; R[instr.b].tryGet<S64>(ia); R[instr.c].tryGet<S64>(ib); R[instr.a] = ScriptValue::makeInt(ia & ib); break; }
            case OpCode::BitOr: { S64 ia = 0, ib = 0; R[instr.b].tryGet<S64>(ia); R[instr.c].tryGet<S64>(ib); R[instr.a] = ScriptValue::makeInt(ia | ib); break; }
            case OpCode::BitXor: { S64 ia = 0, ib = 0; R[instr.b].tryGet<S64>(ia); R[instr.c].tryGet<S64>(ib); R[instr.a] = ScriptValue::makeInt(ia ^ ib); break; }

            case OpCode::CmpEq: case OpCode::CmpNe:
            case OpCode::CmpLt: case OpCode::CmpLe:
            case OpCode::CmpGt: case OpCode::CmpGe:
               R[instr.a] = compareOp(R[instr.b], R[instr.c], instr.op);
               break;

            case OpCode::Concat:
            {
               char sep = static_cast<char>(instr.bx & 0xFF);
               String result = R[instr.b].toDisplayString();
               if (sep != '\0')
                  result += sep;
               result += R[instr.c].toDisplayString();
               R[instr.a] = ScriptValue::makeString(result.c_str());
               break;
            }
            case OpCode::StrEq:
            {
               String sa = R[instr.b].toDisplayString();
               String sb = R[instr.c].toDisplayString();
               R[instr.a] = ScriptValue::makeBool(sa.equal(sb));
               break;
            }
            case OpCode::StrNe:
            {
               String sa = R[instr.b].toDisplayString();
               String sb = R[instr.c].toDisplayString();
               R[instr.a] = ScriptValue::makeBool(!sa.equal(sb));
               break;
            }

            case OpCode::Negate:
            {
               if (R[instr.b].kind() == ScriptValue::Kind::Float)
               {
                  F64 f = 0.0; R[instr.b].tryGet<F64>(f);
                  R[instr.a] = ScriptValue::makeFloat(-f);
               }
               else
               {
                  S64 i = 0; R[instr.b].tryGet<S64>(i);
                  R[instr.a] = ScriptValue::makeInt(-i);
               }
               break;
            }
            case OpCode::LogicalNot: R[instr.a] = ScriptValue::makeBool(!truthy(R[instr.b])); break;
            case OpCode::BitNot: { S64 i = 0; R[instr.b].tryGet<S64>(i); R[instr.a] = ScriptValue::makeInt(~i); break; }

            case OpCode::IncReg: { S64 i = 0; R[instr.a].tryGet<S64>(i); R[instr.a] = ScriptValue::makeInt(i + 1); break; }
            case OpCode::DecReg: { S64 i = 0; R[instr.a].tryGet<S64>(i); R[instr.a] = ScriptValue::makeInt(i - 1); break; }

            case OpCode::Jump:        nextIp = instr.bx; break;
            case OpCode::JumpIfFalse: if (!truthy(R[instr.a])) nextIp = instr.bx; break;
            case OpCode::JumpIfTrue:  if (truthy(R[instr.a]))  nextIp = instr.bx; break;

            case OpCode::Call:
            {
               StringTableEntry fnName = unit.stringConsts[instr.bx];
               ScriptValueSpan args(R.address() + instr.a + 1, instr.b);
               ScriptValue result = callInternal(nullptr, fnName, args);
               if (mHadFatalError) return ScriptValue::makeError("call failed");
               R[instr.a] = result;
               break;
            }
            case OpCode::CallNamespaced:
            {
               U32 nsIndex = instr.bx >> 16;
               U32 nameIndex = instr.bx & 0xFFFF;
               StringTableEntry ns = unit.stringConsts[nsIndex];
               StringTableEntry fnName = unit.stringConsts[nameIndex];
               ScriptValueSpan args(R.address() + instr.a + 1, instr.b);
               ScriptValue result = callInternal(ns, fnName, args);
               if (mHadFatalError) return ScriptValue::makeError("call failed");
               R[instr.a] = result;
               break;
            }
            case OpCode::MethodCall:
            {
               StringTableEntry methodName = unit.stringConsts[instr.bx];
               ScriptValueSpan args(R.address() + instr.a + 1, instr.c);
               ScriptValue result = methodCallInternal(R[instr.b], methodName, args);
               if (mHadFatalError) return ScriptValue::makeError("method call failed");
               R[instr.a] = result;
               break;
            }
            case OpCode::NewObject:
            {
               const BytecodeUnit::ObjectDeclTemplate& tmpl = unit.objectDecls[instr.bx];

               // Name is always resolved to a real ScriptValue string
               // here regardless of static/dynamic form - newObject
               // never needs to know which the source used.
               ScriptValue classNameVal = tmpl.classNameIsDynamic
                  ? R[tmpl.classNameReg]
                  : ScriptValue::makeString(unit.stringConsts[tmpl.classNameConstIndex]);
               ScriptValue objectNameVal = tmpl.objectNameIsDynamic
                  ? R[tmpl.objectNameReg]
                  : ScriptValue::makeString(unit.stringConsts[tmpl.objectNameConstIndex]);

               R[instr.a] = mHost.newObject(tmpl, &classNameVal, &objectNameVal);
               break;
            }

            case OpCode::IterBegin:
            {
               IInterpreterHost::IteratorHandle handle = mHost.iterBegin(R[instr.b], (instr.bx & 1) != 0);
               // Opaque void* packed into an int ScriptValue rather than
               // a new Kind - only this VM consumes the encoding
               // (IterNext/IterEnd below), never leaks to script values.
               R[instr.a] = ScriptValue::makeInt(reinterpret_cast<S64>(handle.opaque));
               break;
            }
            case OpCode::IterNext:
            {
               S64 packed = 0; R[instr.b].tryGet<S64>(packed);
               IInterpreterHost::IteratorHandle handle{ reinterpret_cast<void*>(packed) };
               ScriptValue value;
               if (mHost.iterNext(handle, value))
               {
                  R[instr.a] = value;
               }
               else
               {
                  nextIp = instr.bx;
               }
               break;
            }
            case OpCode::IterEnd:
            {
               S64 packed = 0; R[instr.a].tryGet<S64>(packed);
               mHost.iterEnd(IInterpreterHost::IteratorHandle{ reinterpret_cast<void*>(packed) });
               break;
            }

            case OpCode::Return:     return R[instr.a];
            case OpCode::ReturnNull: return ScriptValue::makeNull();

            case OpCode::Nop: break;

            default:
               reportFatal(line, "internal error: unhandled opcode in executeFrame");
               return ScriptValue::makeError("unhandled opcode");
            }

            frame.ip = nextIp;
         }
      }

      ScriptValue Interpreter::callInternal(StringTableEntry namespaceNameOrNull, StringTableEntry functionName, ScriptValueSpan args)
      {
         if (mCallDepth >= kMaxCallDepth)
         {
            reportFatal(0, "call depth exceeded (possible unbounded recursion)");
            return ScriptValue::makeError("stack overflow");
         }

         std::shared_ptr<const BytecodeUnit> unit = mHost.resolveFunctionUnit(namespaceNameOrNull, functionName);
         if (unit)
         {
            ++mCallDepth;
            CallFrame frame(unit);
            frame.argCount = args.size();
            for (U32 i = 0; i < unit->paramCount && i < args.size(); ++i)
               frame.registers[i] = args[i];
            ScriptValue result = executeFrame(frame);
            --mCallDepth;
            return result;
         }

         return mHost.callFunction(namespaceNameOrNull, functionName, args);
      }

      ScriptValue Interpreter::methodCallInternal(const ScriptValue& object, StringTableEntry methodName, ScriptValueSpan args)
      {
         if (mCallDepth >= kMaxCallDepth)
         {
            reportFatal(0, "call depth exceeded (possible unbounded recursion)");
            return ScriptValue::makeError("stack overflow");
         }

         std::shared_ptr<const BytecodeUnit> unit = mHost.resolveMethodUnit(object, methodName);
         if (unit)
         {
            // No implicit "self in register 0" - the emitter has no
            // reserved receiver register, a ts2 method compiles exactly
            // like a plain function. Receiver is passed as a genuine
            // leading argument instead.
            ++mCallDepth;
            CallFrame frame(unit);
            frame.argCount = args.size() + 1;
            if (unit->paramCount > 0)
               frame.registers[0] = object;
            for (U32 i = 0; i < args.size() && (i + 1) < unit->paramCount; ++i)
               frame.registers[i + 1] = args[i];
            ScriptValue result = executeFrame(frame);
            --mCallDepth;
            return result;
         }

         return mHost.callMethod(object, methodName, args);
      }

   } // namespace ts2
} // namespace newConsole
