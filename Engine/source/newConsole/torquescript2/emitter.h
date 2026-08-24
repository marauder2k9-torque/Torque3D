#ifndef _NEWCONSOLE_TS2_EMITTER_H_
#define _NEWCONSOLE_TS2_EMITTER_H_

#ifndef _NEWCONSOLE_TS2_BYTECODE_H_
#include "newConsole/torquescript2/bytecode.h"
#endif
#ifndef _NEWCONSOLE_TS2_COMPILATIONUNIT_H_
#include "newConsole/torquescript2/compilationUnit.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      /// One emit-time diagnostic - a construct the parser accepted but
      /// the emitter can't compile (e.g. break/continue outside a loop).
      struct EmitDiagnostic
      {
         U32 line = 0;
         String message;
      };

      /// Walks one function body (ListHandle over StmtHandle + params)
      /// into a BytecodeUnit. One Emitter per function; top-level
      /// statements compile as an implicit no-arg function too (see
      /// compileTopLevel), so there's one code path for "compile a body".
      class Emitter
      {
      public:
         explicit Emitter(const CompilationUnit& unit);

         /// Compiles one named function's body into a BytecodeUnit.
         BytecodeUnit compileFunction(const ast::FunctionDeclStmt& fn);

         /// Compiles a file's top-level statements as a single implicit
         /// zero-parameter BytecodeUnit, run once at load time.
         BytecodeUnit compileTopLevel();

         const Vector<EmitDiagnostic>& diagnostics() const { return mDiagnostics; }
         bool hasErrors() const { return !mDiagnostics.empty(); }

      private:
         // ---- register allocation ----
         //
         // Linear: mNextReg only increases while walking deeper into an
         // expression/block, restored (not decremented) when a block or
         // temporary's lifetime ends - see RegisterScope.

         Reg allocTemp();

         /// RAII: saves mNextReg on construction, restores on
         /// destruction - but never below the highest register bound to
         /// a named local in mScopeStack. This clamp is load-bearing: a
         /// local's first assignment can allocate a register mid-scope
         /// and outlive the RegisterScope active at that point; without
         /// the clamp a plain restore would hand that register to the
         /// next temporary while the local is still live. Used for both
         /// temp-expression lifetimes and block scopes (clamp relaxes
         /// once popLexicalScope() removes the block's bindings).
         class RegisterScope
         {
         public:
            explicit RegisterScope(Emitter& emitter) : mEmitter(emitter), mSaved(emitter.mNextReg) {}
            ~RegisterScope() { mEmitter.mNextReg = mEmitter.clampRestore(mSaved); }
            RegisterScope(const RegisterScope&) = delete;
         private:
            Emitter& mEmitter;
            Reg mSaved;
         };

         /// @return max(candidate, one-past the highest register bound
         ///   to a named local) - see RegisterScope's comment.
         Reg clampRestore(Reg candidate) const;

         // ---- named-local scoping ----
         //
         // Each block is a lexical scope; a stack of name->register maps
         // pushed/popped around Block/loop bodies/etc.

         struct LocalBinding { StringTableEntry name; Reg reg; U32 firstInstruction; };

         void pushLexicalScope();
         void popLexicalScope();
         /// Finds @a name in the current scope chain, or allocates a
         /// fresh register in the innermost scope if none exists -
         /// matches "first assignment declares the local" semantics.
         Reg resolveOrDeclareLocal(StringTableEntry name);
         /// Like resolveOrDeclareLocal but never declares - for reading
         /// a local that may not exist yet (reads as empty/zero, not an
         /// error). Returns Reg(~0) if unresolved; caller emits LoadNull.
         Reg tryResolveLocal(StringTableEntry name) const;

         Vector<Vector<LocalBinding>> mScopeStack;

         // ---- jump patching ----
         //
         // emitJump returns the emitted instruction's index so its bx
         // target can be patched once known. patchJumpToHere rewrites a
         // jump's bx to the current end of the instruction stream.

         U32 emitJump(OpCode op, Reg conditionReg = 0);
         void patchJumpToHere(U32 jumpInstructionIndex);

         /// Per-loop break/continue target lists, pushed/popped around a
         /// loop body. break/continue append to the innermost active
         /// list; the loop's emit function patches once it knows its
         /// exit/increment points. A stack, not a single pair, so nested
         /// loops target only their own innermost enclosing loop.
         struct LoopContext
         {
            Vector<U32> breakJumps;
            Vector<U32> continueJumps;
         };
         Vector<LoopContext> mLoopStack;

         // ---- constant pool interning ----
         //
         // Each add*Const reuses an existing entry if the value is
         // already interned, avoiding duplicate pool entries for a
         // repeated literal.
         U32 addIntConst(S64 value);
         U32 addFloatConst(F64 value);
         U32 addStringConst(StringTableEntry value);
         U32 addTaggedStringConst(StringTableEntry value);

         // ---- expression emission ----
         //
         // Contract: emit needed instructions, leave the result in the
         // returned register. Callers don't need instruction counts.

         Reg emitExpr(ast::ExprHandle handle);
         Reg emitBinary(const ast::ExprNode& node);
         Reg emitLogicalAnd(const ast::ExprNode& node);  // short-circuit - see .cpp
         Reg emitLogicalOr(const ast::ExprNode& node);   // short-circuit - see .cpp
         Reg emitAssign(const ast::ExprNode& node);
         Reg emitCompoundAssign(const ast::ExprNode& node);
         Reg emitStringConcat(const ast::ExprNode& node);
         Reg emitStringCompare(const ast::ExprNode& node);
         Reg emitUnary(const ast::ExprNode& node);
         Reg emitPreIncDec(const ast::ExprNode& node);
         Reg emitPostIncDec(const ast::ExprNode& node);
         Reg emitTernary(const ast::ExprNode& node);
         Reg emitFieldAccess(const ast::ExprNode& node);
         Reg emitFieldAssign(const ast::ExprNode& node);
         Reg emitIndexAccess(const ast::ExprNode& node);
         Reg emitIndexAssign(const ast::ExprNode& node);
         Reg emitCall(const ast::ExprNode& node);
         Reg emitMethodCall(const ast::ExprNode& node);
         Reg emitObjectDecl(const ast::ExprNode& node);

         /// Writes an already-computed value (in @a valueReg) to the
         /// storage @a targetHandle names (GlobalVar/LocalVar/
         /// FieldAccess/IndexAccess). Shared by emitAssign and
         /// emitCompoundAssign.
         void emitStoreTo(ast::ExprHandle targetHandle, Reg valueReg);

         // ---- statement emission ----

         void emitStmt(ast::StmtHandle handle);
         void emitBlock(const ast::StmtNode& node);
         void emitIf(const ast::StmtNode& node);
         void emitWhile(const ast::StmtNode& node);
         void emitDoWhile(const ast::StmtNode& node);
         void emitFor(const ast::StmtNode& node);
         void emitForeach(const ast::StmtNode& node);
         void emitSwitch(const ast::StmtNode& node);
         void emitBreak(const ast::StmtNode& node);
         void emitContinue(const ast::StmtNode& node);
         void emitReturn(const ast::StmtNode& node);

         U32 emitRaw(OpCode op, Reg a = 0, Reg b = 0, Reg c = 0, U32 bx = 0, U32 line = 0);
         void reportError(U32 line, const char* message);

         const CompilationUnit& mUnit;
         BytecodeUnit mOut;
         Reg mNextReg = 0;
         Vector<EmitDiagnostic> mDiagnostics;
      };

   } // namespace ts2
} // namespace newConsole

#endif // !_NEWCONSOLE_TS2_EMITTER_H_
