#ifndef _NEWCONSOLE_TS2_BYTECODE_H_
#define _NEWCONSOLE_TS2_BYTECODE_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif
#ifndef _STRINGTABLE_H_
#include "core/stringTable.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _NEWCONSOLE_SCRIPTVALUE_H_
#include "newConsole/host/scriptValue.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      /// @file
      /// torquescript2's bytecode format.
      ///
      /// Register-based, not stack-based: hot-reload can swap a function's
      /// body out from under an in-flight call frame, and a register
      /// machine only needs slot-for-slot compatibility (fixed register
      /// count per function) to stay correct across a swap - far easier
      /// to verify than exact per-instruction stack depth.
      ///
      /// Each function has a flat register array, sized at compile time
      /// (BytecodeUnit::registerCount). [0, paramCount) are parameters;
      /// the rest are compiler-allocated temporaries/locals, assigned
      /// linearly during compilation (same as Lua's reference compiler).

      /// Register index into the current frame's register file.
      typedef U16 Reg;

      enum class OpCode : U16
      {
         // ---- constants ----
         LoadInt,       // R[a] = intConst[bx]
         LoadFloat,     // R[a] = floatConst[bx]
         LoadString,    // R[a] = stringConst[bx]
         LoadTagged,    // R[a] = taggedStringConst[bx]  (see ScriptValue's tagged-string handling)
         LoadNull,      // R[a] = null

         // ---- variable access ----
         MoveReg,       // R[a] = R[b]                          (register-to-register copy)
         GetGlobal,     // R[a] = globals[stringConst[bx]]       (bx names a $Global::Var)
         SetGlobal,     // globals[stringConst[bx]] = R[a]
         GetArgCount,   // R[a] = arg count of current call - see default-param lowering in emitter.cpp's compileFunction
         // No GetLocal/SetLocal - a local read/write is a plain register
         // read/write (see emitter.h's Scope for name-to-register mapping).

         // ---- object field / index access ----
         GetField,      // R[a] = R[b].field(stringConst[bx])          - not internal
         SetField,      // R[b].field(stringConst[bx]) = R[a]
         GetFieldInternal, // R[a] = R[b]->field(stringConst[bx])       - internal-name form
         SetFieldInternal, // R[b]->field(stringConst[bx]) = R[a]
         GetIndex,      // R[a] = R[b][R[c]]
         SetIndex,      // R[b][R[c]] = R[a]

         // ---- arithmetic / bitwise / comparison (R[a] = R[b] OP R[c]) ----
         Add, Sub, Mul, Div, Mod,
         Shl, Shr,
         BitAnd, BitOr, BitXor,
         CmpEq, CmpNe, CmpLt, CmpLe, CmpGt, CmpGe,

         // ---- && / || are lowered to jumps by the emitter, not opcodes -
         // see emitter.h's short-circuit lowering note ----

         // ---- string ops ----
         Concat,        // R[a] = R[b] CONCAT R[c] with separator char in low byte of bx
         StrEq,         // R[a] = (R[b] streq R[c])
         StrNe,         // R[a] = (R[b] strne R[c])

         // ---- unary ----
         Negate,        // R[a] = -R[b]
         LogicalNot,    // R[a] = !R[b]
         BitNot,        // R[a] = ~R[b]

         // ---- in-place inc/dec; field/index/global cases use a Get/Set
         // pair around this - see emitter.h's IncDec lowering note ----
         IncReg,        // R[a] = R[a] + 1
         DecReg,        // R[a] = R[a] - 1

         // ---- control flow ----
         Jump,          // ip = bx  (absolute instruction index, not relative - see BytecodeUnit's note on why)
         JumpIfFalse,   // if (!truthy(R[a])) ip = bx
         JumpIfTrue,    // if (truthy(R[a])) ip = bx

         // ---- calls ----
         Call,          // R[a..a+bx-1] = call stringConst[c] with args R[a+1..a+bx-1] (a itself holds the callee's return slot on entry, args follow)
         CallNamespaced,// same as Call, but c names a second constant (the namespace) rather than an argument count
         MethodCall,    // R[a..] = R[b].method(stringConst[c])(args in R[a+1..])
         NewObject,     // R[a] = construct object per ObjectDecl described by objectDecls[bx]

         // ---- iteration (foreach / foreach$) ----
         //
         // Three opcodes, not one, so emitForeach reuses the same
         // break/continue jump-list machinery every other loop uses
         // (IterNext's failure branch is an ordinary conditional jump).
         IterBegin,     // R[a] = new iterator state over R[b]; bx low bit: 0 = object-collection form (foreach), 1 = string-tokenized form (foreach$)
         IterNext,      // R[a] = next element from iterator R[b]; if exhausted, do not write R[a] and instead jump to bx (the loop's exit)
         IterEnd,       // release iterator state held in R[a] (frees whatever IterBegin allocated - a SimSet iterator handle, a tokenizer cursor, etc.)

         // ---- function exit ----
         Return,        // return R[a]
         ReturnNull,    // return null   (bare `return;`)

         // ---- misc ----
         Nop,
      };

      /// One instruction. Fixed-width, not variable-length - simpler and
      /// easier to disassemble; script bytecode density isn't a concern.
      struct Instruction
      {
         OpCode op;
         Reg a = 0;
         Reg b = 0;
         Reg c = 0;
         U32 bx = 0; // wide immediate/constant-index operand, used instead of b+c (e.g. LoadInt's constant index, Jump's target)
      };

      /// One compiled function or top-level chunk. Content-addressed via
      /// hash (computed by the compiler, not stored here) for hot-reload's
      /// FunctionCell mechanism.
      struct BytecodeUnit
      {
         StringTableEntry name = nullptr;

         Vector<Instruction> code;

         // Constant pools, indexed by Instruction::bx (or a+c together for
         // two-constant opcodes like CallNamespaced). Separate typed pools
         // rather than one ScriptValue pool so a constant's type is known
         // at compile time - no runtime tag check needed.
         Vector<S64> intConsts;
         Vector<F64> floatConsts;
         Vector<StringTableEntry> stringConsts;
         Vector<StringTableEntry> taggedStringConsts;

         /// Object-declaration templates (see ast::ObjectDeclExpr)
         /// referenced by NewObject. Kept out-of-line since the nested
         /// slot/child structure doesn't fit a fixed-width Instruction;
         /// NewObject's bx indexes into this array.
         struct ObjectDeclTemplate
         {
            // Static case: name from the constant pool. Dynamic case
            // ("new (expr)(...)"): classNameIsDynamic/objectNameIsDynamic
            // is set and the matching register holds the already-
            // evaluated name string when NewObject executes.
            U32 classNameConstIndex = 0;
            U32 objectNameConstIndex = 0;
            bool classNameIsDynamic = false;
            bool objectNameIsDynamic = false;
            Reg classNameReg = 0;
            Reg objectNameReg = 0;
            StringTableEntry parentName = nullptr;
            bool isDatablock = false;
            bool isSingleton = false;
            bool isArrayElement = false;
         };
         Vector<ObjectDeclTemplate> objectDecls;

         /// Registers this unit's frame needs. Params occupy
         /// [0, paramCount); rest are locals/temporaries. Checked for
         /// calling-convention compatibility before a hot-reload swap.
         U16 registerCount = 0;
         U16 paramCount = 0;

         /// One entry per Instruction in code - maps instruction to
         /// source line. AST is discarded after compilation, so this is
         /// what a debugger's "what's executing" query uses. Populated
         /// per-instruction, not just per-statement, for expression-level
         /// resolution.
         Vector<U32> lineTable;

         /// Source file this unit was compiled from - null unless
         /// explicitly populated for debugging, same strip-by-default
         /// rule as lineTable (see scriptCompiler.cpp's stripDebugInfo).
         /// A breakpoint can never match a unit with a null origin - see
         /// Interpreter::shouldBreak.
         StringTableEntry origin = nullptr;

         /// Name->register debug info for locals/params, keyed by an
         /// instruction range since a register slot is reused across
         /// sibling scopes (see Emitter::RegisterScope) - a name is only
         /// a valid lookup while ip falls within [firstValidInstruction,
         /// lastValidInstruction] for that entry. Empty unless explicitly
         /// populated for debugging, same strip-by-default rule as
         /// lineTable/origin - a stripped .tsc has no way to recover
         /// local names, by construction, not by convention alone. This
         /// is a deliberate security property: a compiled .tsc is not
         /// meant to be debuggable past "which function is running" (see
         /// Interpreter::CallFrame-level inspection), specifically so a
         /// shipped build doesn't leak source-level identifiers to
         /// anyone with a text/hex editor and no source.
         struct LocalDebugInfo
         {
            StringTableEntry name = nullptr;
            Reg reg = 0;
            U32 firstValidInstruction = 0;
            U32 lastValidInstruction = 0;
         };
         Vector<LocalDebugInfo> localDebugInfo;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_BYTECODE_H_
