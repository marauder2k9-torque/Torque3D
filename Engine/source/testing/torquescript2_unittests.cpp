#include <gtest/gtest.h>

// VM-core suites below test Lexer/Parser/Emitter/Interpreter directly
// against a FakeHost, independent of any engine object model.
#include "newConsole/torquescript2/parser.h"
#include "newConsole/torquescript2/emitter.h"
#include "newConsole/torquescript2/interpreter.h"

// newConsole.h provides ScriptObject, SCRIPT_CLASS/SCRIPT_FIELDS/
// SCRIPT_METHOD, registerObject/resolveObject, getIndex/setIndex.
#include "newConsole/newConsole.h"
#include "newConsole/host/enumRegistry.h"

// Object construction tests still construct a TorqueScript2Runtime
// directly - something has to.
#include "newConsole/torquescript2/torquescript2Runtime.h"

// DebugAdapterTests.
#include "platform/threads/thread.h"

// Compiled-module (.tsc) tests.
#include "newConsole/torquescript2/bytecodeSerialize.h"
#include "newConsole/torquescript2/scriptCompiler.h"
#include "newConsole/torquescript2/sourceFile.h"
#include "newConsole/host/hostBinding.h"

// LogTests.
#include "newConsole/log.h"

#ifndef _FILESTREAM_H_
#include "core/stream/fileStream.h"
#endif
#ifndef _VOLUME_H_
#include "core/volume.h"
#endif
#ifndef _PLATFORMVOLUME_H_
#include "platform/platformVolume.h"
#endif

#include <cstring>
#include <limits>
#include <unordered_map>
#include <cmath>
#include <atomic>
#include <vector>
#include <string>

#ifndef _STRINGFUNCTIONS_H_
#include "core/strings/stringFunctions.h"
#endif

using namespace newConsole;
using namespace newConsole::ts2;
using namespace newConsole::ts2::ast;

namespace newConsole_test
{

   namespace
   {

      /// Minimal fake host: globals in a map, functions resolved from a
      /// pre-populated table, plus real array indexing (delegates to
      /// newConsole::getIndex/setIndex, which is universal - see
      /// host/scriptArray.h) and real foreach$ (string-tokenized) iteration.
      /// Object-collection foreach and object fields/methods are still stubbed
      /// out - tests that need those go through TorqueScript2Runtime instead.
      class FakeHost : public IInterpreterHost
      {
      public:
         std::unordered_map<std::string, ScriptValue> globals;
         std::unordered_map<std::string, std::shared_ptr<const BytecodeUnit>> functions;

         ScriptValue getGlobal(StringTableEntry name) override
         {
            auto it = globals.find(name);
            return it != globals.end() ? it->second : ScriptValue::makeNull();
         }
         void setGlobal(StringTableEntry name, const ScriptValue& value) override { globals[name] = value; }

         ScriptValue getField(const ScriptValue&, StringTableEntry, bool) override { return ScriptValue::makeNull(); }
         bool setField(const ScriptValue&, StringTableEntry, bool, const ScriptValue&) override { return false; }
         ScriptValue getIndex(const ScriptValue& base, const ScriptValue& index) override { return newConsole::getIndex(base, index); }
         ScriptValue setIndex(const ScriptValue& base, const ScriptValue& index, const ScriptValue& value) override { return scriptArraySet(base, index, value); }

         ScriptValue callFunction(StringTableEntry, StringTableEntry, ScriptValueSpan) override { return ScriptValue::makeError("unresolved"); }
         ScriptValue callMethod(const ScriptValue&, StringTableEntry, ScriptValueSpan) override { return ScriptValue::makeError("unresolved"); }
         ScriptValue newObject(const BytecodeUnit::ObjectDeclTemplate&, const ScriptValue*, const ScriptValue*) override { return ScriptValue::makeNull(); }

         struct TokenIterState { std::vector<std::string> tokens; size_t index = 0; };

         IteratorHandle iterBegin(const ScriptValue& collection, bool isStringForm) override
         {
            if (!isStringForm)
               return IteratorHandle{};

            auto* state = new TokenIterState();
            String full = collection.toDisplayString();
            const char* s = full.c_str();
            while (*s)
            {
               while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') ++s;
               if (!*s) break;
               const char* wordStart = s;
               while (*s && *s != ' ' && *s != '\t' && *s != '\n' && *s != '\r') ++s;
               state->tokens.push_back(std::string(wordStart, s - wordStart));
            }
            return IteratorHandle{ state };
         }
         bool iterNext(IteratorHandle handle, ScriptValue& outValue) override
         {
            auto* state = static_cast<TokenIterState*>(handle.opaque);
            if (!state || state->index >= state->tokens.size())
               return false;
            outValue = ScriptValue::makeString(state->tokens[state->index].c_str());
            ++state->index;
            return true;
         }
         void iterEnd(IteratorHandle handle) override
         {
            delete static_cast<TokenIterState*>(handle.opaque);
         }

         std::shared_ptr<const BytecodeUnit> resolveFunctionUnit(StringTableEntry, StringTableEntry name) override
         {
            auto it = functions.find(name);
            return it != functions.end() ? it->second : nullptr;
         }
         std::shared_ptr<const BytecodeUnit> resolveMethodUnit(const ScriptValue&, StringTableEntry) override { return nullptr; }
      };

      /// Compiles the named function out of @a src. Returns nullptr (and lets
      /// gtest's ASSERT_NE at the call site fail with useful context) rather
      /// than aborting itself.
      std::shared_ptr<const BytecodeUnit> compileFn(const char* src, const char* funcName, CompilationUnit& outUnit)
      {
         Lexer lexer(src, "test", nullptr);
         Parser parser(lexer, StringTable->insert("test"));
         outUnit = parser.parse();
         if (parser.hasErrors())
            return nullptr;

         // StringTable->insert() is case-insensitive by default (see
         // core/stringTable.h): "foo"/"Foo"/"FOO" all intern to the SAME
         // StringTableEntry pointer, whose actual byte content is whichever
         // casing happened to be inserted first anywhere in the process - this
         // is purely about string/symbol *identity*, separate from
         // FunctionTable's own "last publish wins" redefinition semantics
         // (matching the legacy scripting language's behavior: if a function
         // is declared more than once under case-variant spellings, the last
         // one compiled is the one that runs - see FunctionTable::publish,
         // which gets this for free from a plain map assignment once both
         // spellings resolve to the same interned key). Comparing a
         // StringTableEntry's *content* via strcmp against a raw, never-
         // interned funcName literal is fragile against the first part
         // (canonical-casing-is-whichever-came-first): intern funcName through
         // the identical path first, then compare interned pointers, so this
         // lookup does not depend on test/process insertion order. Caught by
         // testing: a Debug build's different test ordering surfaced this.
         StringTableEntry internedFuncName = StringTable->insert(funcName);

         const StmtHandle* top = CompilationUnit::listData(outUnit.stmtList, outUnit.topLevel);
         for (U32 i = 0; i < outUnit.topLevel.count; ++i)
         {
            const StmtNode& s = outUnit.get(top[i]);
            if (s.kind == StmtKind::FunctionDecl && s.functionDecl.functionName == internedFuncName)
            {
               Emitter emitter(outUnit);
               BytecodeUnit u = emitter.compileFunction(s.functionDecl);
               if (emitter.hasErrors())
                  return nullptr;
               return std::make_shared<BytecodeUnit>(std::move(u));
            }
         }
         return nullptr;
      }

      S64 runAndGetInt(Interpreter& interp, std::shared_ptr<const BytecodeUnit> unit, ScriptValueSpan args = ScriptValueSpan())
      {
         ScriptValue result = interp.run(unit, args);
         S64 out = std::numeric_limits<S64>::min();
         result.tryGet<S64>(out);
         return out;
      }

      /// Shared fixture base for every VM-core-level suite below - each test
      /// gets its own CompilationUnit/FakeHost, so tests never share compiled
      /// state or global bindings. Suites below are organized by language
      /// feature (what can torquescript2 do), not by implementation layer.
      class ScriptExecTest : public ::testing::Test
      {
      protected:
         CompilationUnit mUnit;
         FakeHost mHost;
      };

      using ArithmeticTests = ScriptExecTest;
      using BitwiseTests = ScriptExecTest;
      using ComparisonTests = ScriptExecTest;
      using StringTests = ScriptExecTest;
      using ControlFlowTests = ScriptExecTest;
      using SwitchTests = ScriptExecTest;
      using FunctionTests = ScriptExecTest;
      using VariableTests = ScriptExecTest;
      using ForeachTests = ScriptExecTest;

   } // namespace

   // =============================================================================
   // ArithmeticTests
   // =============================================================================

   TEST_F(ArithmeticTests, OperatorPrecedence_MultiplyBeforeAdd)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a + %b * 2; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeInt(3), ScriptValue::makeInt(4) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(args, 2)), 11) << "3 + 4*2 should respect * over +";
      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(ArithmeticTests, AllFourOperators)
   {
      auto unit = compileFn("function foo(%a, %b) { return (%a + %b) * (%a - %b) - %a / %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeInt(10), ScriptValue::makeInt(4) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(args, 2)), 82) << "(14*6)-(10/4=2) = 84-2 = 82";
   }

   TEST_F(ArithmeticTests, Modulo)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a % %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeInt(17), ScriptValue::makeInt(5) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(args, 2)), 2);
   }

   TEST_F(ArithmeticTests, IntFloatWidening)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a + %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeInt(1), ScriptValue::makeFloat(1.5) };
      ScriptValue result = interp.run(unit, ScriptValueSpan(args, 2));
      F64 f = -1.0;
      ASSERT_TRUE(result.tryGet<F64>(f));
      EXPECT_DOUBLE_EQ(f, 2.5) << "int + float must widen to float, not truncate";
   }

   TEST_F(ArithmeticTests, UnaryNegate)
   {
      auto unit = compileFn("function foo(%a) { return -%a; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(5) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), -5);
   }

   TEST_F(ArithmeticTests, DivisionByZero_ReportsFatalError)
   {
      auto unit = compileFn("function foo(%a) { return %a / 0; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(5) };
      interp.run(unit, ScriptValueSpan(arg, 1));
      EXPECT_TRUE(interp.hadFatalError());
   }

   // =============================================================================
   // BitwiseTests
   // =============================================================================

   TEST_F(BitwiseTests, AndOrXorNot_ComputeWithoutError)
   {
      auto unit = compileFn("function foo(%a, %b) { return (%a & %b) | (%a ^ %b) & ~%b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeInt(0xF0), ScriptValue::makeInt(0x0F) };
      ScriptValue result = interp.run(unit, ScriptValueSpan(args, 2));
      EXPECT_FALSE(interp.hadFatalError());
      EXPECT_EQ(result.kind(), ScriptValue::Kind::Int);
   }

   TEST_F(BitwiseTests, ShiftLeftAndRight)
   {
      auto unit = compileFn("function foo(%a) { return (%a << 2) >> 1; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(4) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), 8) << "(4<<2)=16, 16>>1=8";
   }

   // =============================================================================
   // ComparisonTests
   // =============================================================================

   TEST_F(ComparisonTests, NumericComparisons)
   {
      auto unit = compileFn(
         "function foo(%a, %b) { return (%a < %b) + (%a <= %a) + (%b > %a) + (%b >= %b) + (%a == %a) + (%a != %b); }",
         "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeInt(1), ScriptValue::makeInt(2) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(args, 2)), 6) << "all six comparisons should be true";
   }

   TEST_F(ComparisonTests, StringEquality_StrEqOperator)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a $= %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue sameArgs[2] = { ScriptValue::makeString("hello"), ScriptValue::makeString("hello") };
      ScriptValue diffArgs[2] = { ScriptValue::makeString("hello"), ScriptValue::makeString("world") };

      ScriptValue r1 = interp.run(unit, ScriptValueSpan(sameArgs, 2));
      bool b1 = false; r1.tryGet<bool>(b1);
      EXPECT_TRUE(b1) << "$= on equal strings";

      ScriptValue r2 = interp.run(unit, ScriptValueSpan(diffArgs, 2));
      bool b2 = true; r2.tryGet<bool>(b2);
      EXPECT_FALSE(b2) << "$= on different strings";
   }

   TEST_F(ComparisonTests, StringInequality_StrNeOperator)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a !$= %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue diffArgs[2] = { ScriptValue::makeString("hello"), ScriptValue::makeString("world") };
      ScriptValue r = interp.run(unit, ScriptValueSpan(diffArgs, 2));
      bool b = false; r.tryGet<bool>(b);
      EXPECT_TRUE(b) << "!$= on different strings should be true";
   }

   // =============================================================================
   // StringTests
   // =============================================================================

   TEST_F(StringTests, PlainConcat)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a @ %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeString("hello"), ScriptValue::makeString("world") };
      ScriptValue result = interp.run(unit, ScriptValueSpan(args, 2));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "helloworld");
   }

   TEST_F(StringTests, ConcatWithNLSigil)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a NL %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeString("a"), ScriptValue::makeString("b") };
      ScriptValue result = interp.run(unit, ScriptValueSpan(args, 2));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "a\nb");
   }

   TEST_F(StringTests, ConcatWithTABSigil)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a TAB %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeString("a"), ScriptValue::makeString("b") };
      ScriptValue result = interp.run(unit, ScriptValueSpan(args, 2));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "a\tb");
   }

   TEST_F(StringTests, ConcatWithSPCSigil)
   {
      auto unit = compileFn("function foo(%a, %b) { return %a SPC %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue args[2] = { ScriptValue::makeString("a"), ScriptValue::makeString("b") };
      ScriptValue result = interp.run(unit, ScriptValueSpan(args, 2));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "a b");
   }

   TEST_F(StringTests, TaggedStringLiteral_CompilesAndReturnsText)
   {
      // 'text' is a tagged string - ordinary literal syntax; the "tagged"
      // behavior (transmitted once over the network, referenced afterward
      // by an integer tag) is a network-layer concern, not something the
      // interpreter itself needs to do anything special for beyond reading
      // it back as text - see astNodes.h's TaggedLiteralExpr comment.
      auto unit = compileFn("function foo() { return 'a tagged string'; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue result = interp.run(unit, ScriptValueSpan());
      EXPECT_FALSE(interp.hadFatalError());
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "a tagged string");
   }

   TEST_F(StringTests, TaggedStringLiteral_UsableInConcat)
   {
      auto unit = compileFn("function foo() { return 'tag' @ \"plain\"; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue result = interp.run(unit, ScriptValueSpan());
      EXPECT_FALSE(interp.hadFatalError());
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "tagplain");
   }

   // =============================================================================
   // ControlFlowTests
   // =============================================================================

   TEST_F(ControlFlowTests, IfElse_BothBranches)
   {
      auto unit = compileFn("function foo(%x) { if (%x > 0) { return 1; } else { return -1; } }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue posArg[1] = { ScriptValue::makeInt(5) };
      ScriptValue negArg[1] = { ScriptValue::makeInt(-5) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(posArg, 1)), 1);
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(negArg, 1)), -1);
   }

   TEST_F(ControlFlowTests, WhileLoop_Accumulates)
   {
      auto unit = compileFn(
         "function foo() { %i = 0; %sum = 0; while (%i < 10) { %sum += %i; %i++; } return %sum; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 45) << "sum of 0..9";
      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(ControlFlowTests, WhileLoop_BreakStopsAtCorrectIteration)
   {
      auto unit = compileFn(
         "function foo() { %i = 0; while (%i < 100) { if (%i == 5) break; %i++; } return %i; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 5);
   }

   TEST_F(ControlFlowTests, DoWhileLoop_RunsBodyAtLeastOnce)
   {
      auto unit = compileFn("function foo() { %i = 0; do { %i++; } while (%i < 0); return %i; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 1) << "do-while runs the body once even though the condition is false immediately";
   }

   TEST_F(ControlFlowTests, ForLoop_ContinueSkipsEvens)
   {
      auto unit = compileFn(
         "function foo() { %sum = 0; for (%i = 0; %i < 10; %i++) { if (%i % 2 == 0) continue; %sum += %i; } return %sum; }",
         "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 25) << "1+3+5+7+9";
   }

   TEST_F(ControlFlowTests, Ternary_TrueBranch)
   {
      auto unit = compileFn("function foo(%x) { return %x > 0 ? 1 : -1; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue posArg[1] = { ScriptValue::makeInt(1) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(posArg, 1)), 1);
   }

   TEST_F(ControlFlowTests, LogicalAnd_ShortCircuitsAndSkipsRhsSideEffect)
   {
      auto unit = compileFn("function foo() { %x = 0; return %x != 0 && (1 / %x > 0); }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue result = interp.run(unit, ScriptValueSpan());
      ASSERT_FALSE(interp.hadFatalError()) << "short-circuit should have prevented the division by zero";

      bool b = true;
      ASSERT_TRUE(result.tryGet<bool>(b));
      EXPECT_FALSE(b);
   }

   TEST_F(ControlFlowTests, LogicalOr_ShortCircuitsAndSkipsRhsSideEffect)
   {
      auto unit = compileFn("function foo() { %x = 1; return %x != 0 || (1 / 0 > 0); }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue result = interp.run(unit, ScriptValueSpan());
      ASSERT_FALSE(interp.hadFatalError()) << "short-circuit should have prevented the division by zero";

      bool b = false;
      ASSERT_TRUE(result.tryGet<bool>(b));
      EXPECT_TRUE(b);
   }

   // =============================================================================
   // SwitchTests
   // =============================================================================

   TEST_F(SwitchTests, DispatchesIncludingOrValues)
   {
      auto unit = compileFn(
         "function foo(%x) { switch (%x) { case 1: return 100; case 2 or 3: return 200; default: return 0; } }",
         "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      auto runWith = [&](S64 x) {
         ScriptValue arg[1] = { ScriptValue::makeInt(x) };
         return runAndGetInt(interp, unit, ScriptValueSpan(arg, 1));
      };
      EXPECT_EQ(runWith(1), 100);
      EXPECT_EQ(runWith(2), 200) << "case 2 or 3 - value 2";
      EXPECT_EQ(runWith(3), 200) << "case 2 or 3 - value 3";
      EXPECT_EQ(runWith(99), 0) << "default";
   }

   TEST_F(SwitchTests, StringForm_DispatchesByStringEquality)
   {
      auto unit = compileFn(
         "function foo(%x) { switch$ (%x) { case \"a\": return 1; case \"b\": return 2; default: return 0; } }",
         "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue argA[1] = { ScriptValue::makeString("a") };
      ScriptValue argB[1] = { ScriptValue::makeString("b") };
      ScriptValue argC[1] = { ScriptValue::makeString("c") };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(argA, 1)), 1);
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(argB, 1)), 2);
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(argC, 1)), 0);
   }

   // =============================================================================
   // FunctionTests
   // =============================================================================

   TEST_F(FunctionTests, Call_ResolvesThroughInterpreterFastPath)
   {
      CompilationUnit helperUnit, callerUnit;
      auto helper = compileFn("function helper(%x) { return %x * 10; }", "helper", helperUnit);
      auto caller = compileFn("function caller(%x) { return helper(%x) + 1; }", "caller", callerUnit);
      ASSERT_NE(helper, nullptr);
      ASSERT_NE(caller, nullptr);

      mHost.functions["helper"] = helper;
      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(5) };
      EXPECT_EQ(runAndGetInt(interp, caller, ScriptValueSpan(arg, 1)), 51) << "helper(5)*... + 1 == 5*10+1";
      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(FunctionTests, DefaultParameters_OneDefaultedAppliedOnlyWhenArgMissing)
   {
      auto unit = compileFn("function foo(%a, %b = 5) { return %a + %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue oneArg[1] = { ScriptValue::makeInt(10) };
      ScriptValue twoArgs[2] = { ScriptValue::makeInt(10), ScriptValue::makeInt(1) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(oneArg, 1)), 15) << "default applied";
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(twoArgs, 2)), 11) << "explicit arg overrides default";
   }

   TEST_F(FunctionTests, DefaultParameters_AllDefaulted_CalledWithZeroOneOrTwoArgs)
   {
      // function foo(%input1 = 10, %input2 = 1) - every parameter has a
      // default, exercised with zero, one, and two arguments. Genuinely
      // different from the one-defaulted-parameter test above: it requires
      // the argument-count check to correctly apply (or skip) a default
      // independently for *every* parameter position, not just the last one.
      auto unit = compileFn("function foo(%input1 = 10, %input2 = 1) { return %input1 + %input2; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);

      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan()), 11) << "both defaults applied: 10 + 1";

      ScriptValue oneArg[1] = { ScriptValue::makeInt(99) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(oneArg, 1)), 100) << "first arg explicit, second defaulted: 99 + 1";

      ScriptValue twoArgs[2] = { ScriptValue::makeInt(99), ScriptValue::makeInt(2) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(twoArgs, 2)), 101) << "both args explicit: 99 + 2";

      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(FunctionTests, NamespacedCall_ResolvesCorrectly)
   {
      CompilationUnit helperUnit, callerUnit;
      auto helper = compileFn("function MyNamespace::helper(%x) { return %x + 1; }", "helper", helperUnit);
      auto caller = compileFn("function caller(%x) { return MyNamespace::helper(%x); }", "caller", callerUnit);
      ASSERT_NE(helper, nullptr);
      ASSERT_NE(caller, nullptr);

      mHost.functions["helper"] = helper;
      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(4) };
      EXPECT_EQ(runAndGetInt(interp, caller, ScriptValueSpan(arg, 1)), 5);
      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(FunctionTests, Recursion_Fibonacci)
   {
      auto unit = compileFn(
         "function fib(%n) { if (%n < 2) { return %n; } return fib(%n - 1) + fib(%n - 2); }", "fib", mUnit);
      ASSERT_NE(unit, nullptr);

      mHost.functions["fib"] = unit;
      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(10) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), 55);
      EXPECT_FALSE(interp.hadFatalError());
   }

   // =============================================================================
   // VariableTests
   // =============================================================================

   TEST_F(VariableTests, GlobalReadWrite)
   {
      auto unit = compileFn("function foo(%x) { $g = %x; return $g + 1; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeInt(9) };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), 10);
   }

   TEST_F(VariableTests, CompoundAssign_ArithmeticOperators)
   {
      auto unit = compileFn(
         "function foo() { %x = 10; %x += 5; %x -= 3; %x *= 2; %x /= 4; %x %= 4; return %x; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 2) << "((((10+5)-3)*2)/4)%4 = 2";
      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(VariableTests, CompoundAssign_BitwiseOperators)
   {
      auto unit = compileFn(
         "function foo() { %x = 0xF0; %x &= 0xFF; %x |= 0x0F; %x ^= 0x01; %x <<= 1; %x >>= 1; return %x; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue result = interp.run(unit, ScriptValueSpan());
      EXPECT_FALSE(interp.hadFatalError());
      EXPECT_EQ(result.kind(), ScriptValue::Kind::Int) << "all six compound bitwise assigns should compile and run without error";
   }

   TEST_F(VariableTests, PreIncrement_ReturnsNewValue)
   {
      auto unit = compileFn("function foo() { %x = 5; return ++%x; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 6) << "pre-increment evaluates to the new value";
   }

   TEST_F(VariableTests, PostIncrement_ReturnsOldValueButMutates)
   {
      auto unit = compileFn("function foo() { %x = 5; %y = %x++; return %y * 10 + %x; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 56) << "%y (old value)=5, %x (mutated)=6 -> 5*10+6=56";
   }

   TEST_F(VariableTests, PreDecrementAndPostDecrement)
   {
      auto unit = compileFn("function foo() { %x = 5; %a = --%x; %b = %x--; return %a * 10 + %b; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      EXPECT_EQ(runAndGetInt(interp, unit), 44) << "--%x: x=4,%a=4. %x--: %b=4(old), x becomes 3. 4*10+4=44";
   }

   // =============================================================================
   // ForeachTests
   // =============================================================================

   TEST_F(ForeachTests, StringForm_IteratesWhitespaceSeparatedTokens)
   {
      auto unit = compileFn(
         "function foo(%words) { %count = 0; foreach$(%w in %words) { %count++; } return %count; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeString("one two three four") };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), 4);
      EXPECT_FALSE(interp.hadFatalError());
   }

   TEST_F(ForeachTests, StringForm_LoopVariableHoldsEachToken)
   {
      auto unit = compileFn(
         "function foo(%words) { %result = \"\"; foreach$(%w in %words) { %result = %result @ %w; } return %result; }",
         "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeString("a b c") };
      ScriptValue result = interp.run(unit, ScriptValueSpan(arg, 1));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "abc");
   }

   TEST_F(ForeachTests, StringForm_BreakStopsEarly)
   {
      auto unit = compileFn(
         "function foo(%words) { %count = 0; foreach$(%w in %words) { if (%w $= \"stop\") break; %count++; } return %count; }",
         "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeString("a b stop c d") };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), 2) << "should stop counting at 'stop'";
   }

   TEST_F(ForeachTests, ObjectCollectionForm_RunsZeroIterations_KnownGap)
   {
      // Object-collection foreach (not foreach$) needs a SimSet-equivalent
      // that does not exist in host/ yet (see TorqueScript2Runtime::
      // iterBegin's own comment on this gap). This test documents the
      // current, correct-but-incomplete behavior - zero iterations, no
      // crash, no fatal error - rather than leaving this path with no
      // coverage at all. Update this test once object-collection iteration
      // is implemented for real.
      auto unit = compileFn(
         "function foo(%set) { %count = 0; foreach(%obj in %set) { %count++; } return %count; }", "foo", mUnit);
      ASSERT_NE(unit, nullptr);

      Interpreter interp(mHost);
      ScriptValue arg[1] = { ScriptValue::makeNull() };
      EXPECT_EQ(runAndGetInt(interp, unit, ScriptValueSpan(arg, 1)), 0);
      EXPECT_FALSE(interp.hadFatalError()) << "an unimplemented iteration form should run zero iterations, not crash";
   }

   // =============================================================================
   // ScriptClassMacrosTest / ObjectRegistryTest: SCRIPT_CLASS-declared
   // classes, exercised through the real macro-generated reflection - this
   // is testing the host/ reflection layer's own correctness, independent
   // of torquescript2 specifically.
   // =============================================================================

   namespace
   {

      class TestWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(TestWidget, ScriptObject);
         SCRIPT_FIELDS(mHealth, mName);

         S32 mHealth = 100;
         StringTableEntry mName = nullptr;

         SCRIPT_METHOD(S32, doubleHealth, ());
      };

      /// Used only by ObjectConstructionTests below.
      class ConstructWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(ConstructWidget, ScriptObject);
         SCRIPT_FIELDS(mValue);
         S32 mValue = 0;

         SCRIPT_METHOD(S32, doubleValue, ());
         SCRIPT_METHOD(S32, addToValue, (S32 amount, const char* label));
         SCRIPT_METHOD(S32, addWithDefaultLabel, (S32 amount, const char* label), (0, "default"));
      };

   } // namespace

   SCRIPT_CLASS_BEGIN(ConstructWidget)
      SCRIPT_CLASS_END(ConstructWidget)

      S32 ConstructWidget::doubleValue()
   {
      mValue *= 2;
      return mValue;
   }

   S32 ConstructWidget::addToValue(S32 amount, const char* label)
   {
      mValue += amount;
      mValue += static_cast<S32>(dStrlen(label));
      return mValue;
   }

   S32 ConstructWidget::addWithDefaultLabel(S32 amount, const char* label)
   {
      mValue += amount;
      mValue += static_cast<S32>(dStrlen(label));
      return mValue;
   }

   SCRIPT_CLASS_BEGIN(TestWidget)
      SCRIPT_CLASS_END(TestWidget)

      S32 TestWidget::doubleHealth()
   {
      mHealth *= 2;
      return mHealth;
   }

   TEST(ScriptClassMacrosTest, GetScriptClassRep_ReportsCorrectNameAndFieldCount)
   {
      const ScriptClassRep* rep = TestWidget::getScriptClassRep();
      ASSERT_NE(rep, nullptr);
      EXPECT_STREQ(rep->getName(), "TestWidget");
      EXPECT_EQ(rep->getFields().size(), 2u) << "mHealth, mName";
      EXPECT_EQ(rep->getMethods().size(), 1u) << "doubleHealth";
   }

   TEST(ScriptClassMacrosTest, GetRuntimeClassRep_ResolvesThroughBasePointer)
   {
      TestWidget* w = new TestWidget();
      ScriptObject* base = w;

      const ScriptClassRep* rep = base->getRuntimeClassRep();
      ASSERT_NE(rep, nullptr);
      EXPECT_STREQ(rep->getName(), "TestWidget");

      w->decRefCount();
   }

   TEST(ScriptClassMacrosTest, FieldGetSet_RoundTripsThroughRealMemberPointer)
   {
      TestWidget* w = new TestWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      ASSERT_NE(rep, nullptr);

      const ScriptFieldRep* healthField = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(healthField, nullptr);

      ScriptValue got = healthField->get(w);
      S64 health = -1;
      ASSERT_TRUE(got.tryGet<S64>(health));
      EXPECT_EQ(health, 100);

      bool setOk = healthField->set(w, ScriptValue::makeInt(42));
      EXPECT_TRUE(setOk);
      EXPECT_EQ(w->mHealth, 42) << "set() must write through to the real member, not a copy";

      w->decRefCount();
   }

   TEST(ScriptClassMacrosTest, MethodInvoke_MutatesRealObjectAndReturnsResult)
   {
      TestWidget* w = new TestWidget();
      w->mHealth = 50;
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      ASSERT_NE(rep, nullptr);

      const ScriptMethodRep* method = rep->findMethod(StringTable->insert("doubleHealth"));
      ASSERT_NE(method, nullptr);

      ScriptValue result = method->invoke(w, ScriptValueSpan());
      S64 returned = -1;
      ASSERT_TRUE(result.tryGet<S64>(returned));
      EXPECT_EQ(returned, 100);
      EXPECT_EQ(w->mHealth, 100) << "invoke() must mutate the real object, not a copy";

      w->decRefCount();
   }

   TEST(ObjectRegistryTest, RegisterResolveUnregister_RoundTrip)
   {
      TestWidget* w = new TestWidget();
      ObjectHandle handle = newConsole::registerObject(w);
      EXPECT_FALSE(handle.isNull());

      ScriptObject* resolved = newConsole::resolveObject(handle);
      EXPECT_EQ(resolved, w);

      ObjectHandle handleAgain = newConsole::registerObject(w);
      EXPECT_EQ(handle.id, handleAgain.id) << "registering the same object twice must be idempotent";

      newConsole::unregisterObject(handle);
      EXPECT_EQ(newConsole::resolveObject(handle), nullptr) << "stale handle must not resolve after unregister";
   }

   // =============================================================================
   // ObjectConstructionTests: new/singleton/datablock, run through the real
   // TorqueScript2Runtime - these need ClassFactory/ObjectRegistry wiring
   // that FakeHost deliberately stubs out.
   // =============================================================================

   namespace
   {

      class ObjectConstructionTests : public ::testing::Test
      {
      protected:
         void SetUp() override
         {
            runtime = new TorqueScript2Runtime();
            ASSERT_TRUE(newConsole::registerRuntime(runtime));
         }

         void TearDown() override
         {
            newConsole::shutdownAll();
         }

         TorqueScript2Runtime* runtime = nullptr;
      };

   } // namespace

   TEST_F(ObjectConstructionTests, New_AlwaysConstructsFreshInstance)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function makeTwo() { %a = new ConstructWidget(); %b = new ConstructWidget(); return %a; }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("makeTwo"), ScriptValueSpan());
      EXPECT_EQ(result.kind(), ScriptValue::Kind::Object);
   }

   TEST_F(ObjectConstructionTests, Singleton_FindsExistingByName)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function makeSingleton() { return singleton ConstructWidget(MySingleton); }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue first = runtime->callFunction(StringTable->insert("makeSingleton"), ScriptValueSpan());
      ScriptValue second = runtime->callFunction(StringTable->insert("makeSingleton"), ScriptValueSpan());

      ObjectHandle h1, h2;
      ASSERT_TRUE(first.tryGet<ObjectHandle>(h1));
      ASSERT_TRUE(second.tryGet<ObjectHandle>(h2));
      EXPECT_EQ(h1.id, h2.id) << "singleton should return the same object both times";
   }

   TEST_F(ObjectConstructionTests, Datablock_FindsExistingByNameLikeSingleton)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function makeDatablock() { return datablock ConstructWidget(MyDatablock); }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue first = runtime->callFunction(StringTable->insert("makeDatablock"), ScriptValueSpan());
      ScriptValue second = runtime->callFunction(StringTable->insert("makeDatablock"), ScriptValueSpan());

      ObjectHandle h1, h2;
      ASSERT_TRUE(first.tryGet<ObjectHandle>(h1));
      ASSERT_TRUE(second.tryGet<ObjectHandle>(h2));
      EXPECT_EQ(h1.id, h2.id) << "datablock should find-or-create by name, same as singleton";
   }

   TEST_F(ObjectConstructionTests, New_WithSameNameStillProducesDistinctInstances)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function makeNamed() { return new ConstructWidget(SameName); }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue first = runtime->callFunction(StringTable->insert("makeNamed"), ScriptValueSpan());
      ScriptValue second = runtime->callFunction(StringTable->insert("makeNamed"), ScriptValueSpan());

      ObjectHandle h1, h2;
      ASSERT_TRUE(first.tryGet<ObjectHandle>(h1));
      ASSERT_TRUE(second.tryGet<ObjectHandle>(h2));
      EXPECT_NE(h1.id, h2.id) << "'new' must always construct fresh, even when reusing a name";
   }

   TEST_F(ObjectConstructionTests, MethodCall_OnScriptConstructedObject)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function makeAndDouble() { %obj = new ConstructWidget(){}; %obj.mValue = 5; return %obj.doubleValue(); }",
         diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("makeAndDouble"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 10) << "method call on a new-constructed object";
   }

   TEST_F(ObjectConstructionTests, MethodCall_WithMultipleArguments)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function makeAndAdd() { %obj = new ConstructWidget(){}; %obj.mValue = 5; return %obj.addToValue(10, \"hi\"); }",
         diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("makeAndAdd"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 17) << "5 + 10 + strlen(\"hi\")=2 = 17, proves int+string args both marshal correctly";
   }

   TEST_F(ObjectConstructionTests, MethodCall_WithDefaultArgument)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function addBoth() { %obj = new ConstructWidget(){}; %obj.mValue = 0; return %obj.addWithDefaultLabel(10, \"hi\"); }\n"
         "function addOnlyRequired() { %obj = new ConstructWidget(){}; %obj.mValue = 0; return %obj.addWithDefaultLabel(10); }\n",
         diags);
      ASSERT_TRUE(lr.success);

      ScriptValue bothResult = runtime->callFunction(StringTable->insert("addBoth"), ScriptValueSpan());
      S64 bothVal = -1;
      ASSERT_TRUE(bothResult.tryGet<S64>(bothVal));
      EXPECT_EQ(bothVal, 12) << "explicit label 'hi' (2 chars): 10 + 2 = 12";

      ScriptValue onlyRequiredResult = runtime->callFunction(StringTable->insert("addOnlyRequired"), ScriptValueSpan());
      S64 onlyRequiredVal = -1;
      ASSERT_TRUE(onlyRequiredResult.tryGet<S64>(onlyRequiredVal));
      EXPECT_EQ(onlyRequiredVal, 17) << "default label 'default' (7 chars): 10 + 7 = 17";
   }

   TEST_F(ObjectConstructionTests, ScriptDefinedMethod_OverridesCppMethod)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function ConstructWidget::doubleValue(%this) { return %this.mValue * 100; }", diags);
      ASSERT_TRUE(lr.success);

      LoadResult lr2 = runtime->loadSource("t.ts2",
         "function makeAndDouble2() { %obj = new ConstructWidget(){}; %obj.mValue = 5; return %obj.doubleValue(); }",
         diags);
      ASSERT_TRUE(lr2.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("makeAndDouble2"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 500) << "script override must win over C++ SCRIPT_METHOD (5*100, not 5*2)";
   }

   TEST_F(ObjectConstructionTests, Redefinition_CaseInsensitive_LastDefinitionWins)
   {
      // Matches the legacy scripting language's behavior: function names
      // are case-insensitive, and if the same name (in any casing) is
      // declared more than once, the LAST one compiled is the one that
      // runs - not the first. FunctionTable::publish gets this for free
      // from StringTable's case-insensitive interning (both spellings
      // resolve to the same key) plus a plain map assignment (last write
      // wins) - this test exists to pin that behavior down explicitly
      // rather than leave it as an unverified emergent property.
      DiagnosticSink diags;

      LoadResult lr1 = runtime->loadSource("t.ts2", "function greet() { return 1; }", diags);
      ASSERT_TRUE(lr1.success);
      S64 firstResult = -1;
      runtime->callFunction(StringTable->insert("greet"), ScriptValueSpan()).tryGet<S64>(firstResult);
      EXPECT_EQ(firstResult, 1);

      // Redeclare under a different casing - same symbol, new body.
      LoadResult lr2 = runtime->loadSource("t.ts2", "function GREET() { return 2; }", diags);
      ASSERT_TRUE(lr2.success);

      S64 afterRedefine = -1;
      runtime->callFunction(StringTable->insert("greet"), ScriptValueSpan()).tryGet<S64>(afterRedefine);
      EXPECT_EQ(afterRedefine, 2) << "calling by the original casing should now reach the redefined body";

      S64 viaNewCasing = -1;
      runtime->callFunction(StringTable->insert("Greet"), ScriptValueSpan()).tryGet<S64>(viaNewCasing);
      EXPECT_EQ(viaNewCasing, 2) << "calling by yet another casing should reach the same (redefined) body";
   }

   // =============================================================================
   // HotReloadTests: reloadSource's actual contract - a compile error must
   // not disturb the previously-published good version of the function(s)
   // it targets, and reloading one function must not disturb its siblings
   // in the same file. Same registerRuntime/shutdownAll fixture pattern as
   // ObjectConstructionTests above.
   //
   // NOTE ON SCOPE: only these two behaviors are implemented in the
   // codebase today. Live debugging / breakpoints are NOT implemented -
   // IDebugAdapter (setBreakpoint/removeBreakpoint/currentStack/evaluate)
   // is designed in newConsole_multilang_host_design.md section 5 and
   // explicitly called out for torquescript2 to implement against its own
   // VM, but no such adapter, no IScriptRuntime::getDebugAdapter()
   // override, and no breakpoint/pause hook exists anywhere in
   // Interpreter::executeFrame today. A test can't exercise code that
   // doesn't exist, so instead of faking coverage, there's one DISABLED_
   // test at the bottom of this section that documents the exact contract
   // the future IDebugAdapter needs to satisfy. Remove the DISABLED_
   // prefix and fill in the real calls once that adapter exists - until
   // then it's a spec, not a test.
   // =============================================================================

   namespace
   {

      class HotReloadTests : public ::testing::Test
      {
      protected:
         void SetUp() override
         {
            runtime = new TorqueScript2Runtime();
            ASSERT_TRUE(newConsole::registerRuntime(runtime));
         }

         void TearDown() override
         {
            newConsole::shutdownAll();
         }

         TorqueScript2Runtime* runtime = nullptr;
      };

   } // namespace

   TEST_F(HotReloadTests, ReloadWithSyntaxErrorKeepsPreviousGoodVersion)
   {
      DiagnosticSink loadDiags;
      LoadResult lr = runtime->loadSource("hotreload_syntax.ts2",
         "function greet(%name) { return \"hello \" @ %name; }", loadDiags);
      ASSERT_TRUE(lr.success);

      ScriptValue arg[1] = { ScriptValue::makeString("world") };
      ScriptValue before = runtime->callFunction(StringTable->insert("greet"), ScriptValueSpan(arg, 1));
      const char* beforeStr = "";
      ASSERT_TRUE(before.tryGet<const char*>(beforeStr));
      EXPECT_STREQ(beforeStr, "hello world");

      // Deliberately broken: unterminated string literal - a lexer error, so
      // compileAndPublish returns false before its per-function emit/publish
      // loop ever runs (see compileAndPublish's early return on
      // lexer.hasErrors()/parser.hasErrors()), meaning nothing in this file
      // gets touched at all.
      DiagnosticSink reloadDiags;
      ReloadResult reloadResult = runtime->reloadSource("hotreload_syntax.ts2",
         "function greet(%name) { return \"hello ; }", reloadDiags);

      EXPECT_FALSE(reloadResult.anyApplied()) << "a syntax error must not apply any symbol from this reload";
      EXPECT_TRUE(reloadDiags.hasErrors()) << "the syntax error should be reported";

      // The previously-published version must still be the one that runs -
      // this is the actual guarantee under test.
      ScriptValue after = runtime->callFunction(StringTable->insert("greet"), ScriptValueSpan(arg, 1));
      const char* afterStr = "";
      ASSERT_TRUE(after.tryGet<const char*>(afterStr));
      EXPECT_STREQ(afterStr, "hello world") << "greet() must still be the pre-reload version after a failed reload";
   }

   TEST_F(HotReloadTests, ReloadWithEmitErrorInOneFunctionKeepsThatFunctionsPreviousVersion)
   {
      // Distinct from the syntax-error case above: exercises
      // compileAndPublish's per-function emit loop, where one function's
      // emit error (break outside a loop) is reported but does not prevent
      // a DIFFERENT function in the same reload from successfully
      // publishing - anyEmitError only gates the overall return value, not
      // already-applied publishes earlier in the same loop.
      DiagnosticSink loadDiags;
      LoadResult lr = runtime->loadSource("hotreload_partial.ts2",
         "function willBreak() { return 1; }"
         "function staysGood() { return 2; }", loadDiags);
      ASSERT_TRUE(lr.success);

      ScriptValue before = runtime->callFunction(StringTable->insert("willBreak"), ScriptValueSpan());
      S64 beforeVal = -1;
      ASSERT_TRUE(before.tryGet<S64>(beforeVal));
      EXPECT_EQ(beforeVal, 1);

      // willBreak now has a genuine emit-time error (break outside a loop);
      // staysGood is a valid, different implementation.
      DiagnosticSink reloadDiags;
      ReloadResult reloadResult = runtime->reloadSource("hotreload_partial.ts2",
         "function willBreak() { break; return 99; }"
         "function staysGood() { return 20; }", reloadDiags);

      ASSERT_EQ(reloadResult.symbols.size(), 2u);

      bool foundWillBreak = false, foundStaysGood = false;
      for (const ReloadedSymbol& sym : reloadResult.symbols)
      {
         if (sym.name == StringTable->insert("willBreak"))
         {
            foundWillBreak = true;
            EXPECT_FALSE(sym.applied) << "willBreak has an emit error and must not apply";
            EXPECT_NE(sym.reason.c_str()[0], '\0') << "a failed symbol should carry a non-empty reason";
         }
         else if (sym.name == StringTable->insert("staysGood"))
         {
            foundStaysGood = true;
            EXPECT_TRUE(sym.applied) << "staysGood compiles cleanly and should still apply despite willBreak's failure";
         }
      }
      EXPECT_TRUE(foundWillBreak);
      EXPECT_TRUE(foundStaysGood);

      // willBreak keeps running its previous (pre-reload) body...
      ScriptValue afterBreak = runtime->callFunction(StringTable->insert("willBreak"), ScriptValueSpan());
      S64 afterBreakVal = -1;
      ASSERT_TRUE(afterBreak.tryGet<S64>(afterBreakVal));
      EXPECT_EQ(afterBreakVal, 1) << "willBreak must still be the pre-reload version";

      // ...while staysGood picks up its new body, from the SAME reload call.
      ScriptValue afterStays = runtime->callFunction(StringTable->insert("staysGood"), ScriptValueSpan());
      S64 afterStaysVal = -1;
      ASSERT_TRUE(afterStays.tryGet<S64>(afterStaysVal));
      EXPECT_EQ(afterStaysVal, 20) << "staysGood should have picked up its new implementation";
   }

   TEST_F(HotReloadTests, ReloadingOneFunctionDoesNotAffectSiblingFunctionsInSameFile)
   {
      DiagnosticSink loadDiags;
      LoadResult lr = runtime->loadSource("hotreload_granularity.ts2",
         "function alpha() { return 1; }"
         "function beta() { return 2; }"
         "function gamma() { return 3; }", loadDiags);
      ASSERT_TRUE(lr.success);

      auto callInt = [&](const char* name) -> S64
      {
         ScriptValue v = runtime->callFunction(StringTable->insert(name), ScriptValueSpan());
         S64 out = -1;
         v.tryGet<S64>(out);
         return out;
      };

      ASSERT_EQ(callInt("alpha"), 1);
      ASSERT_EQ(callInt("beta"), 2);
      ASSERT_EQ(callInt("gamma"), 3);

      // Only beta's body actually changes; alpha/gamma are re-declared
      // identically, as a real editor round-trip of "the whole file" would
      // produce - reloadSource operates per source file, not per function,
      // so the assertion is that only beta's *behavior* changes, not that
      // alpha/gamma were somehow excluded from compilation.
      DiagnosticSink reloadDiags;
      ReloadResult reloadResult = runtime->reloadSource("hotreload_granularity.ts2",
         "function alpha() { return 1; }"
         "function beta() { return 200; }"
         "function gamma() { return 3; }", reloadDiags);

      EXPECT_TRUE(reloadResult.anyApplied());
      EXPECT_FALSE(reloadDiags.hasErrors());

      EXPECT_EQ(callInt("alpha"), 1) << "alpha unaffected by beta's change";
      EXPECT_EQ(callInt("beta"), 200) << "beta picked up its new body";
      EXPECT_EQ(callInt("gamma"), 3) << "gamma unaffected by beta's change";
   }

   TEST_F(HotReloadTests, DISABLED_ScriptExecutionSuspendsAtBreakpoint)
   {
      // Left as GTEST_SKIP-only: IDebugAdapter, IScriptRuntime::getDebugAdapter(),
      // and StackFrameInfo don't exist in the codebase yet (no such override
      // or type is declared anywhere in scriptHost.h or
      // torquescript2Runtime.h/.cpp), so this body can't reference them
      // without breaking the build even with the test disabled (gtest still
      // compiles a DISABLED_ test's body, it only skips running it). The
      // commented block below is the intended shape once those exist -
      // uncomment and adjust once TorqueScript2Runtime::getDebugAdapter()
      // is real.
      //
      // DiagnosticSink loadDiags;
      // LoadResult lr = runtime->loadSource("breakpoint_test.ts2",
      //    "function withLocal(%x) {\n"
      //    "   %doubled = %x * 2;\n"    // line 2 - intended breakpoint target
      //    "   return %doubled;\n"
      //    "}", loadDiags);
      // ASSERT_TRUE(lr.success);
      //
      // IDebugAdapter* debugAdapter = runtime->getDebugAdapter();
      // ASSERT_NE(debugAdapter, nullptr) << "torquescript2 should expose a debug adapter once implemented";
      //
      // ASSERT_TRUE(debugAdapter->setBreakpoint("breakpoint_test.ts2", 2));
      //
      // // Real implementation would need to run the call on another thread
      // // (or via a resumable/async run() variant) so this thread can observe
      // // the suspended state - the current Interpreter::run() is fully
      // // synchronous/blocking with no suspend point, which is itself part of
      // // what needs to change to support this.
      //
      // ScriptValue arg = ScriptValue::makeInt(21);
      // auto future = runtime->callFunctionAsync(StringTable->insert("withLocal"), ScriptValueSpan(&arg, 1));
      // ASSERT_TRUE(debugAdapter->waitForSuspend(/*timeoutMs*/ 1000));
      //
      // std::vector<StackFrameInfo> stack = debugAdapter->currentStack();
      // ASSERT_FALSE(stack.empty());
      // EXPECT_EQ(stack[0].line, 2u);
      //
      // ScriptValue xValue = debugAdapter->evaluate("%x", /*frameIndex*/ 0);
      // S64 xInt = 0; xValue.tryGet<S64>(xInt);
      // EXPECT_EQ(xInt, 21);
      //
      // debugAdapter->removeBreakpoint("breakpoint_test.ts2", 2);
      // ScriptValue result = future.resumeAndWait();
      // S64 resultInt = 0; result.tryGet<S64>(resultInt);
      // EXPECT_EQ(resultInt, 42);

      GTEST_SKIP() << "IDebugAdapter is not implemented yet - see newConsole_multilang_host_design.md section 5";
   }

   // =============================================================================
   // LogTests: newConsole::LogSink dispatch (newConsole/log.h).
   // =============================================================================

   namespace
   {

      class RecordingSink : public newConsole::LogSink
      {
      public:
         struct Entry { newConsole::LogLevel level; String line; };
         Vector<Entry> entries;

         void write(newConsole::LogLevel level, const char* line) override
         {
            entries.push_back({ level, String(line) });
         }
      };

   } // namespace

   TEST(LogTests, RegisteredSink_ReceivesLoggedLine)
   {
      RecordingSink sink;
      newConsole::addLogSink(&sink);

      newConsole::logInfo("hello %s", "world");

      newConsole::removeLogSink(&sink);

      ASSERT_EQ(sink.entries.size(), 1u);
      EXPECT_EQ(sink.entries[0].level, newConsole::LogLevel::Info);
      EXPECT_STREQ(sink.entries[0].line.c_str(), "hello world");
   }

   TEST(LogTests, LogWarnAndLogError_ReportCorrectLevel)
   {
      RecordingSink sink;
      newConsole::addLogSink(&sink);

      newConsole::logWarn("warning %d", 1);
      newConsole::logError("error %d", 2);

      newConsole::removeLogSink(&sink);

      ASSERT_EQ(sink.entries.size(), 2u);
      EXPECT_EQ(sink.entries[0].level, newConsole::LogLevel::Warn);
      EXPECT_STREQ(sink.entries[0].line.c_str(), "warning 1");
      EXPECT_EQ(sink.entries[1].level, newConsole::LogLevel::Error);
      EXPECT_STREQ(sink.entries[1].line.c_str(), "error 2");
   }

   TEST(LogTests, RemovedSink_NoLongerReceivesLines)
   {
      RecordingSink sink;
      newConsole::addLogSink(&sink);
      newConsole::removeLogSink(&sink);

      newConsole::logInfo("should not arrive");

      EXPECT_TRUE(sink.entries.empty());
   }

   TEST(LogTests, MultipleSinks_AllReceiveSameLine)
   {
      RecordingSink sinkA, sinkB;
      newConsole::addLogSink(&sinkA);
      newConsole::addLogSink(&sinkB);

      newConsole::logInfo("broadcast");

      newConsole::removeLogSink(&sinkA);
      newConsole::removeLogSink(&sinkB);

      ASSERT_EQ(sinkA.entries.size(), 1u);
      ASSERT_EQ(sinkB.entries.size(), 1u);
      EXPECT_STREQ(sinkA.entries[0].line.c_str(), "broadcast");
      EXPECT_STREQ(sinkB.entries[0].line.c_str(), "broadcast");
   }

   TEST(LogTests, NoRegisteredSinks_DoesNotCrash)
   {
      newConsole::logInfo("nobody is listening");
      SUCCEED();
   }

   TEST(LogTests, LevelParameterizedLog_DispatchesToCorrectLevel)
   {
      RecordingSink sink;
      newConsole::addLogSink(&sink);

      newConsole::log(newConsole::LogLevel::Warn, "via level param %d", 7);

      newConsole::removeLogSink(&sink);

      ASSERT_EQ(sink.entries.size(), 1u);
      EXPECT_EQ(sink.entries[0].level, newConsole::LogLevel::Warn);
      EXPECT_STREQ(sink.entries[0].line.c_str(), "via level param 7");
   }

   // =============================================================================
   // DebugAdapterTests
   // =============================================================================

   namespace
   {

      class CallOnThread : public Thread
      {
      public:
         TorqueScript2Runtime* runtime;
         StringTableEntry funcName;
         ScriptValue arg;
         ScriptValue result;

         CallOnThread(TorqueScript2Runtime* rt, StringTableEntry name, ScriptValue a)
            : Thread(0, NULL, false), runtime(rt), funcName(name), arg(a)
         {
         }

         void run(void*) override
         {
            ScriptValue args[1] = { arg };
            result = runtime->callFunction(funcName, ScriptValueSpan(args, 1));
         }
      };

      class DebugAdapterTests : public ::testing::Test
      {
      protected:
         void SetUp() override
         {
            runtime = new TorqueScript2Runtime();
            ASSERT_TRUE(newConsole::registerRuntime(runtime));
            adapter = runtime->getDebugAdapter();
            ASSERT_NE(adapter, nullptr);
         }

         void TearDown() override
         {
            newConsole::shutdownAll();
         }

         TorqueScript2Runtime* runtime = nullptr;
         IDebugAdapter* adapter = nullptr;
      };

   } // namespace

   TEST_F(DebugAdapterTests, BreakpointSuspendsExecutionOnBackgroundThread)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("bp_basic.ts2",
         "function withLocal(%x) {\n"
         "   %doubled = %x * 2;\n"
         "   return %doubled;\n"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ASSERT_TRUE(adapter->setBreakpoint("bp_basic.ts2", 2));

      CallOnThread thread(runtime, StringTable->insert("withLocal"), ScriptValue::makeInt(21));
      thread.start();

      ASSERT_TRUE(adapter->waitForSuspend(2000));
      EXPECT_TRUE(adapter->isPaused());

      adapter->resume();
      thread.join();

      S64 v = -1;
      ASSERT_TRUE(thread.result.tryGet<S64>(v));
      EXPECT_EQ(v, 42);
   }

   TEST_F(DebugAdapterTests, CurrentStack_ReportsFunctionAndLine)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("bp_stack.ts2",
         "function withLocal(%x) {\n"
         "   %doubled = %x * 2;\n"
         "   return %doubled;\n"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ASSERT_TRUE(adapter->setBreakpoint("bp_stack.ts2", 2));

      CallOnThread thread(runtime, StringTable->insert("withLocal"), ScriptValue::makeInt(5));
      thread.start();
      ASSERT_TRUE(adapter->waitForSuspend(2000));

      Vector<StackFrameInfo> stack = adapter->currentStack();
      ASSERT_FALSE(stack.empty());
      EXPECT_STREQ(stack[0].origin, "bp_stack.ts2");
      EXPECT_STREQ(stack[0].functionName, "withLocal");
      EXPECT_EQ(stack[0].line, 2u);

      adapter->resume();
      thread.join();
   }

   TEST_F(DebugAdapterTests, GetLocal_ReadsParameterValueWhilePaused)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("bp_local.ts2",
         "function withLocal(%x) {\n"
         "   %doubled = %x * 2;\n"
         "   return %doubled;\n"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ASSERT_TRUE(adapter->setBreakpoint("bp_local.ts2", 2));

      CallOnThread thread(runtime, StringTable->insert("withLocal"), ScriptValue::makeInt(21));
      thread.start();
      ASSERT_TRUE(adapter->waitForSuspend(2000));

      ScriptValue x = adapter->getLocal(0, StringTable->insert("%x"));
      S64 xVal = -1;
      ASSERT_TRUE(x.tryGet<S64>(xVal));
      EXPECT_EQ(xVal, 21);

      adapter->resume();
      thread.join();
   }

   TEST_F(DebugAdapterTests, GetLocal_UnknownName_ReturnsError)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("bp_unknown.ts2",
         "function withLocal(%x) {\n"
         "   %doubled = %x * 2;\n"
         "   return %doubled;\n"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ASSERT_TRUE(adapter->setBreakpoint("bp_unknown.ts2", 2));

      CallOnThread thread(runtime, StringTable->insert("withLocal"), ScriptValue::makeInt(1));
      thread.start();
      ASSERT_TRUE(adapter->waitForSuspend(2000));

      ScriptValue v = adapter->getLocal(0, StringTable->insert("%doesNotExist"));
      EXPECT_EQ(v.kind(), ScriptValue::Kind::Error);

      adapter->resume();
      thread.join();
   }

   TEST_F(DebugAdapterTests, RemoveBreakpoint_StopsFutureHits)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("bp_remove.ts2",
         "function plain(%x) { return %x + 1; }", diags);
      ASSERT_TRUE(lr.success);

      ASSERT_TRUE(adapter->setBreakpoint("bp_remove.ts2", 1));
      ASSERT_TRUE(adapter->removeBreakpoint("bp_remove.ts2", 1));

      ScriptValue arg[1] = { ScriptValue::makeInt(4) };
      ScriptValue result = runtime->callFunction(StringTable->insert("plain"), ScriptValueSpan(arg, 1));

      EXPECT_FALSE(adapter->isPaused());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 5);
   }

   // =============================================================================
   // ArrayTests: indexing on ScriptValue::Kind::Array via newConsole::
   // getIndex/setIndex directly - universal, not runtime-specific (see
   // host/scriptArray.h), so no runtime construction is needed here.
   // =============================================================================

   TEST(ArrayTests, GetSet_RoundTrips)
   {
      ScriptValue arr = ScriptValue::makeArray();
      bool ok = newConsole::setIndex(arr, ScriptValue::makeInt(2), ScriptValue::makeInt(99));
      EXPECT_TRUE(ok);

      ScriptValue readBack = newConsole::getIndex(arr, ScriptValue::makeInt(2));
      S64 v = -1;
      ASSERT_TRUE(readBack.tryGet<S64>(v));
      EXPECT_EQ(v, 99);
   }

   TEST(ArrayTests, GetSet_WriteVisibleThroughOriginalHandle)
   {
      ScriptValue arr = ScriptValue::makeArray();
      bool ok = newConsole::setIndex(arr, ScriptValue::makeInt(0), ScriptValue::makeInt(7));
      EXPECT_TRUE(ok);

      ScriptValue readBack = newConsole::getIndex(arr, ScriptValue::makeInt(0));
      S64 v = -1;
      ASSERT_TRUE(readBack.tryGet<S64>(v));
      EXPECT_EQ(v, 7) << "a write through a copied handle must be visible via the original array (shared Vector)";
   }

   TEST(ArrayTests, GetSet_OutOfRangeReadReturnsNullNotError)
   {
      ScriptValue arr = ScriptValue::makeArray();
      newConsole::setIndex(arr, ScriptValue::makeInt(0), ScriptValue::makeInt(1));

      ScriptValue readBack = newConsole::getIndex(arr, ScriptValue::makeInt(50));
      EXPECT_EQ(readBack.kind(), ScriptValue::Kind::Null) << "reading past the end should read as empty, not error";
   }

   TEST(ArrayTests, GetSet_NonArrayBaseReturnsError)
   {
      ScriptValue notAnArray = ScriptValue::makeInt(5);
      ScriptValue readBack = newConsole::getIndex(notAnArray, ScriptValue::makeInt(0));
      EXPECT_EQ(readBack.kind(), ScriptValue::Kind::Error);

      bool ok = newConsole::setIndex(notAnArray, ScriptValue::makeInt(0), ScriptValue::makeInt(1));
      EXPECT_FALSE(ok);
   }

   // ArrayVivificationTests: a Null base vivifies into a fresh array on
   // first indexed write, instead of failing.

   TEST(ArrayVivificationTests, NullBase_VivifiesIntoNewArray)
   {
      ScriptValue unset; // Kind::Null
      ASSERT_EQ(unset.kind(), ScriptValue::Kind::Null);

      ScriptValue result = scriptArraySet(unset, ScriptValue::makeInt(2), ScriptValue::makeInt(99));
      ASSERT_EQ(result.kind(), ScriptValue::Kind::Array);

      ScriptValue readBack = newConsole::getIndex(result, ScriptValue::makeInt(2));
      S64 v = -1;
      ASSERT_TRUE(readBack.tryGet<S64>(v));
      EXPECT_EQ(v, 99);
   }

   TEST(ArrayVivificationTests, NonNullNonArrayBase_StillReturnsErrorNotVivified)
   {
      // vivification is Null-specific, never a silent overwrite.
      ScriptValue notAnArray = ScriptValue::makeInt(5);
      ScriptValue result = scriptArraySet(notAnArray, ScriptValue::makeInt(0), ScriptValue::makeInt(1));
      EXPECT_EQ(result.kind(), ScriptValue::Kind::Error);
   }

   TEST(ArrayVivificationTests, ExistingArrayBase_IdentityPreservedNotReplaced)
   {
      ScriptValue arr = ScriptValue::makeArray();
      arr.arrayRef().push_back(ScriptValue::makeInt(1));

      ScriptValue result = scriptArraySet(arr, ScriptValue::makeInt(0), ScriptValue::makeInt(42));
      ASSERT_EQ(result.kind(), ScriptValue::Kind::Array);

      S64 v = -1;
      ASSERT_TRUE(arr.arrayRef()[0].tryGet<S64>(v));
      EXPECT_EQ(v, 42) << "write must be visible through the original handle - shared storage";
   }

   TEST(ArrayTests, ScriptSide_IndexAssignAndReadBack)
   {
      CompilationUnit cu;
      Lexer lexer("function useArray(%arr) { %arr[2] = 99; return %arr[2]; }", "test", nullptr);
      Parser parser(lexer, StringTable->insert("test"));
      cu = parser.parse();
      ASSERT_FALSE(parser.hasErrors());

      const StmtHandle* top = CompilationUnit::listData(cu.stmtList, cu.topLevel);
      std::shared_ptr<const BytecodeUnit> unit;
      for (U32 i = 0; i < cu.topLevel.count; ++i)
      {
         const StmtNode& s = cu.get(top[i]);
         if (s.kind == StmtKind::FunctionDecl && s.functionDecl.functionName == StringTable->insert("useArray"))
         {
            Emitter emitter(cu);
            BytecodeUnit u = emitter.compileFunction(s.functionDecl);
            ASSERT_FALSE(emitter.hasErrors());
            unit = std::make_shared<BytecodeUnit>(std::move(u));
         }
      }
      ASSERT_NE(unit, nullptr);

      FakeHost host;
      Interpreter interp(host);
      ScriptValue arr = ScriptValue::makeArray();
      ScriptValue arg[1] = { arr };
      ScriptValue result = interp.run(unit, ScriptValueSpan(arg, 1));

      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 99);
   }

   // =============================================================================
   // EnumTests: SCRIPT_ENUM-registered enum class types, marshalled through
   // ScriptTypeTraits<T>'s generic std::is_enum_v specialization - no
   // per-enum boilerplate anywhere in the field/method binding machinery,
   // replacing the legacy EngineEnumTable pattern (which had no lookup
   // method at all and required a hand-written linear scan/string compare
   // at every consumption site).
   // =============================================================================

   namespace
   {

      enum class EnumTestHealth : S32 { Dead = 0, Wounded = 1, Healthy = 2 };

      class EnumWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(EnumWidget, ScriptObject);
         SCRIPT_FIELDS(mHealth);
         EnumTestHealth mHealth = EnumTestHealth::Dead;
      };

   } // namespace

   SCRIPT_ENUM(EnumTestHealth, Dead, Wounded, Healthy)

      SCRIPT_CLASS_BEGIN(EnumWidget)
      SCRIPT_CLASS_END(EnumWidget)

      TEST(EnumTests, DefaultValue_ReadsBackAsRegisteredName)
   {
      EnumWidget* w = new EnumWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* healthField = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(healthField, nullptr);

      ScriptValue got = healthField->get(w);
      EXPECT_EQ(got.kind(), ScriptValue::Kind::String);
      const char* s = "";
      ASSERT_TRUE(got.tryGet<const char*>(s));
      EXPECT_STREQ(s, "Dead");

      w->decRefCount();
   }

   TEST(EnumTests, Set_AcceptsNameStringOrRawInteger)
   {
      EnumWidget* w = new EnumWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* healthField = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(healthField, nullptr);

      bool setByName = healthField->set(w, ScriptValue::makeString("Healthy"));
      EXPECT_TRUE(setByName);
      EXPECT_EQ(w->mHealth, EnumTestHealth::Healthy);

      bool setByInt = healthField->set(w, ScriptValue::makeInt(1));
      EXPECT_TRUE(setByInt);
      EXPECT_EQ(w->mHealth, EnumTestHealth::Wounded);

      w->decRefCount();
   }

   TEST(EnumTests, Set_CaseInsensitiveNameMatch)
   {
      EnumWidget* w = new EnumWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* healthField = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(healthField, nullptr);

      bool ok = healthField->set(w, ScriptValue::makeString("healthy"));
      EXPECT_TRUE(ok);
      EXPECT_EQ(w->mHealth, EnumTestHealth::Healthy);

      w->decRefCount();
   }

   TEST(EnumTests, Set_UnmatchedNameFails)
   {
      EnumWidget* w = new EnumWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* healthField = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(healthField, nullptr);

      bool ok = healthField->set(w, ScriptValue::makeString("NotARealValue"));
      EXPECT_FALSE(ok);
      EXPECT_EQ(w->mHealth, EnumTestHealth::Dead) << "a failed set must not mutate the field";

      w->decRefCount();
   }

   namespace
   {

      class EnumScriptTest : public ::testing::Test
      {
      protected:
         void SetUp() override
         {
            runtime = new TorqueScript2Runtime();
            ASSERT_TRUE(newConsole::registerRuntime(runtime));
         }
         void TearDown() override
         {
            newConsole::shutdownAll();
         }
         TorqueScript2Runtime* runtime = nullptr;
      };

   } // namespace

   TEST_F(EnumScriptTest, ScriptRead_ReturnsRegisteredName)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function checkHealth(%obj) { return %obj.mHealth; }", diags);
      ASSERT_TRUE(lr.success);

      EnumWidget* w = new EnumWidget();
      w->mHealth = EnumTestHealth::Wounded;
      ObjectHandle handle = newConsole::registerObject(w);
      ScriptValue arg[1] = { ScriptValue::makeObject(handle) };

      ScriptValue result = runtime->callFunction(StringTable->insert("checkHealth"), ScriptValueSpan(arg, 1));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "Wounded");
   }

   TEST_F(EnumScriptTest, ScriptWrite_ByNameString_MutatesRealObject)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function setHealthy(%obj) { %obj.mHealth = \"Healthy\"; return %obj.mHealth; }", diags);
      ASSERT_TRUE(lr.success);

      EnumWidget* w = new EnumWidget();
      ObjectHandle handle = newConsole::registerObject(w);
      ScriptValue arg[1] = { ScriptValue::makeObject(handle) };

      ScriptValue result = runtime->callFunction(StringTable->insert("setHealthy"), ScriptValueSpan(arg, 1));
      const char* s = "";
      ASSERT_TRUE(result.tryGet<const char*>(s));
      EXPECT_STREQ(s, "Healthy");
      EXPECT_EQ(w->mHealth, EnumTestHealth::Healthy) << "must mutate the real object, not a copy";
   }

   // =============================================================================
   // NotifyFieldTests: ADD_FIELD/ADD_FIELD_NET fields fire onFieldChanged
   // on every write, C++ or script, both via NotifyField<T>::operator= -
   // closes a gap the legacy engine had (onStaticModified fired only for
   // script writes).
   // =============================================================================

   namespace
   {

      class NotifyWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(NotifyWidget, ScriptObject);
         ADD_FIELD(S32, mHealth, "current health", 100);
         ADD_FIELD_NET(S32, mScore, "score value", 0, NetFieldAttribute::fixed(0x02, 16));

         int notifyCount = 0;
         StringTableEntry lastChangedField = nullptr;
         U32 lastMask = 0;

         void onFieldChanged(StringTableEntry name, U32 mask) override
         {
            notifyCount++;
            lastChangedField = name;
            lastMask = mask;
         }
      };

   } // namespace

   SCRIPT_CLASS_BEGIN(NotifyWidget)
      SCRIPT_CLASS_END(NotifyWidget)

      TEST(NotifyFieldTests, PlainCppWrite_FiresOnFieldChanged)
   {
      NotifyWidget* w = new NotifyWidget();
      w->mHealth = 50;

      EXPECT_EQ(w->notifyCount, 1);
      EXPECT_EQ(w->lastChangedField, StringTable->insert("mHealth"));
      EXPECT_EQ((S32)w->mHealth, 50);

      w->decRefCount();
   }

   TEST(NotifyFieldTests, ScriptDrivenWrite_FiresSameHookAsCppWrite)
   {
      NotifyWidget* w = new NotifyWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* f = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(f, nullptr);

      bool ok = f->set(w, ScriptValue::makeInt(75));
      EXPECT_TRUE(ok);
      EXPECT_EQ(w->notifyCount, 1) << "script-driven set must fire onFieldChanged the same as a C++ write";
      EXPECT_EQ((S32)w->mHealth, 75);

      ScriptValue got = f->get(w);
      S64 readBack = -1;
      ASSERT_TRUE(got.tryGet<S64>(readBack));
      EXPECT_EQ(readBack, 75);

      w->decRefCount();
   }

   TEST(NotifyFieldTests, DefaultDirtyMask_IsAlwaysDirty)
   {
      NotifyWidget* w = new NotifyWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* f = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(f, nullptr);
      EXPECT_EQ(f->network.dirtyMask, NetFieldAttribute::kAlwaysDirty)
         << "ADD_FIELD with no explicit NetFieldAttribute should default to alwaysDirty(), not 'never sent'";

      w->decRefCount();
   }

   TEST(NotifyFieldTests, ExplicitDirtyMask_UsesGivenMask)
   {
      NotifyWidget* w = new NotifyWidget();
      w->mScore = 10;
      EXPECT_EQ(w->lastMask, 0x02u) << "ADD_FIELD_NET's explicit mask must be what onFieldChanged receives";

      w->decRefCount();
   }

   TEST(NotifyFieldTests, InitialValue_MatchesAddFieldArgument)
   {
      NotifyWidget* w = new NotifyWidget();
      EXPECT_EQ((S32)w->mHealth, 100) << "ADD_FIELD's initial-value argument";
      EXPECT_EQ((S32)w->mScore, 0);

      w->decRefCount();
   }

   namespace
   {

      class CircleWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(CircleWidget, ScriptObject);
         ADD_FIELD(F32, mRadius, "radius", 1.0f);
         ADD_FIELD(F32, mArea, "computed area", 3.14159f);

         void onFieldChanged(StringTableEntry name, U32) override
         {
            // Cascading update: changing radius recomputes area. Guarded by
            // field name so mArea's own write below does not recurse.
            if (name == StringTable->insert("mRadius"))
            {
               F32 r = mRadius.get();
               mArea = 3.14159f * r * r;
            }
         }
      };

   } // namespace

   SCRIPT_CLASS_BEGIN(CircleWidget)
      SCRIPT_CLASS_END(CircleWidget)

      TEST(NotifyFieldTests, CascadingUpdate_OneFieldRecomputesAnother)
   {
      CircleWidget* w = new CircleWidget();
      w->mRadius = 2.0f;

      F32 area = w->mArea.get();
      EXPECT_NEAR(area, 12.566f, 0.01f) << "changing mRadius should recompute mArea via onFieldChanged";

      w->decRefCount();
   }

   // =============================================================================
   // CustomFieldTests: SCRIPT_FIELD_CUSTOM - getter/setter-backed field,
   // not a data member. Setter can validate/clamp before storing, unlike
   // NotifyFieldTests' after-the-fact onFieldChanged.
   // =============================================================================

   namespace
   {

      class ClampedHealthWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(ClampedHealthWidget, ScriptObject);
         SCRIPT_FIELD_CUSTOM(mHealth, "health, clamped 0-100", S32, getHealth, setHealth);

         S32 mRawHealth = 100;

         ScriptValue getHealth() const { return ScriptValue::makeInt(mRawHealth); }
         bool setHealth(S32 v)
         {
            if (v < 0 || v > 100)
               return false;
            mRawHealth = v;
            return true;
         }
      };

   } // namespace

   SCRIPT_CLASS_BEGIN(ClampedHealthWidget)
      SCRIPT_CLASS_END(ClampedHealthWidget)

      TEST(CustomFieldTests, Get_CallsCustomGetter)
   {
      ClampedHealthWidget* w = new ClampedHealthWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* f = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(f, nullptr);

      ScriptValue got = f->get(w);
      S64 v = -1;
      ASSERT_TRUE(got.tryGet<S64>(v));
      EXPECT_EQ(v, 100);

      w->decRefCount();
   }

   TEST(CustomFieldTests, Set_ValidValue_CallsCustomSetterAndStores)
   {
      ClampedHealthWidget* w = new ClampedHealthWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* f = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(f, nullptr);

      bool ok = f->set(w, ScriptValue::makeInt(50));
      EXPECT_TRUE(ok);
      EXPECT_EQ(w->mRawHealth, 50);

      w->decRefCount();
   }

   TEST(CustomFieldTests, Set_InvalidValue_RejectedAndUnchanged)
   {
      ClampedHealthWidget* w = new ClampedHealthWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      const ScriptFieldRep* f = rep->findField(StringTable->insert("mHealth"));
      ASSERT_NE(f, nullptr);

      bool ok = f->set(w, ScriptValue::makeInt(999));
      EXPECT_FALSE(ok) << "custom setter validation should reject an out-of-range value";
      EXPECT_EQ(w->mRawHealth, 100) << "a rejected set must leave the stored value untouched";

      w->decRefCount();
   }

   TEST(CustomFieldTests, ScriptSide_SetAndGetGoThroughCustomFunctions)
   {
      TorqueScript2Runtime* runtime = new TorqueScript2Runtime();
      ASSERT_TRUE(newConsole::registerRuntime(runtime));

      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function trySetHealth(%obj, %v) { %obj.mHealth = %v; return %obj.mHealth; }", diags);
      ASSERT_TRUE(lr.success);

      ClampedHealthWidget* w = new ClampedHealthWidget();
      ObjectHandle handle = newConsole::registerObject(w);
      ScriptValue args[2] = { ScriptValue::makeObject(handle), ScriptValue::makeInt(200) };

      ScriptValue result = runtime->callFunction(StringTable->insert("trySetHealth"), ScriptValueSpan(args, 2));
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 100) << "an out-of-range script-side set should be rejected, reading back the unchanged value";

      newConsole::shutdownAll();
   }

   // StructValueTests: SCRIPT_STRUCT struct types (%v.x, %v.method(),
   // static/free functions), both directly and through a real runtime.

} // namespace newConsole_test

// VectorF must be at global scope, not anonymous-namespaced - SCRIPT_STRUCT's
// ScriptTypeTraits<VectorF> specialization needs unqualified lookup to find it.
struct VectorF
{
   F32 x, y, z;
   VectorF() : x(0.0f), y(0.0f), z(0.0f) {}
   VectorF(F32 X, F32 Y, F32 Z) : x(X), y(Y), z(Z) {}

   F32 length() const { return static_cast<F32>(std::sqrt(static_cast<double>(x * x + y * y + z * z))); }

   VectorF normalized() const
   {
      F32 len = length();
      if (len <= 0.0f)
         return VectorF();
      return VectorF(x / len, y / len, z / len);
   }

   static F32 dot(VectorF a, VectorF b) { return a.x * b.x + a.y * b.y + a.z * b.z; }

   static VectorF cross(VectorF a, VectorF b)
   {
      return VectorF(a.y * b.z - a.z * b.y,
         a.z * b.x - a.x * b.z,
         a.x * b.y - a.y * b.x);
   }
};

SCRIPT_STRUCT(VectorF,
   SCRIPT_STRUCT_FIELDS(x, y, z),
   SCRIPT_STRUCT_METHODS(length, normalized),
   SCRIPT_STRUCT_STATIC_METHODS(dot, cross));

namespace newConsole_test
{

   namespace
   {

      // Ordinary SCRIPT_CLASS with a VectorF field - no special-casing
      // needed anywhere in FieldAccessor/SCRIPT_FIELDS.
      class VectorWidget : public ScriptObject
      {
      public:
         SCRIPT_CLASS(VectorWidget, ScriptObject);
         SCRIPT_FIELDS(mVelocity);

         VectorF mVelocity{ 1.0f, 2.0f, 3.0f };
      };

   } // namespace

} // namespace newConsole_test

SCRIPT_CLASS_BEGIN(newConsole_test::VectorWidget)
SCRIPT_CLASS_END(newConsole_test::VectorWidget)

namespace newConsole_test
{

   TEST(StructValueTests, ToScript_ProducesStructKindWithRegisteredType)
   {
      VectorF v(3.0f, 4.0f, 0.0f);
      ScriptValue sv = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(v);

      ASSERT_EQ(sv.kind(), ScriptValue::Kind::Struct);
      ASSERT_NE(sv.structType(), nullptr);
      EXPECT_STREQ(sv.structType()->getName(), "VectorF");
      EXPECT_EQ(sv.structType()->componentCount(), 3u);
   }

   TEST(StructValueTests, FromScript_RoundTripsThroughRealComponents)
   {
      VectorF v(3.0f, 4.0f, 5.0f);
      ScriptValue sv = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(v);

      VectorF back;
      ASSERT_TRUE(newConsole::detail::ScriptTypeTraits<VectorF>::fromScript(back, sv));
      EXPECT_FLOAT_EQ(back.x, 3.0f);
      EXPECT_FLOAT_EQ(back.y, 4.0f);
      EXPECT_FLOAT_EQ(back.z, 5.0f);
   }

   TEST(StructValueTests, FromScript_AcceptsPlainArrayLiteralPositionally)
   {
      // [1, 2, 3] positionally, not via VectorF's own constructor syntax.
      ScriptValue arr = ScriptValue::makeArray();
      arr.arrayRef().push_back(ScriptValue::makeFloat(7.0));
      arr.arrayRef().push_back(ScriptValue::makeFloat(8.0));
      arr.arrayRef().push_back(ScriptValue::makeFloat(9.0));

      VectorF v;
      ASSERT_TRUE(newConsole::detail::ScriptTypeTraits<VectorF>::fromScript(v, arr));
      EXPECT_FLOAT_EQ(v.x, 7.0f);
      EXPECT_FLOAT_EQ(v.y, 8.0f);
      EXPECT_FLOAT_EQ(v.z, 9.0f);
   }

   TEST(StructValueTests, NamedFieldLookup_GetAndSetRoundTripThroughComponentStorage)
   {
      VectorF v(1.0f, 2.0f, 3.0f);
      ScriptValue sv = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(v);
      const StructTypeRep* type = sv.structType();
      ASSERT_NE(type, nullptr);

      const ScriptStructFieldRep* yField = type->findField(StringTable->insert("y"));
      ASSERT_NE(yField, nullptr);
      EXPECT_EQ(yField->componentIndex, 1u);

      ScriptValue got = sv.arrayRef()[yField->componentIndex];
      F64 y = -1;
      ASSERT_TRUE(got.tryGet<F64>(y));
      EXPECT_EQ(y, 2.0);

      // shared storage - visible through every copy of sv.
      sv.arrayRef()[yField->componentIndex] = ScriptValue::makeFloat(99.0);
      ScriptValue gotAfter = sv.arrayRef()[yField->componentIndex];
      F64 yAfter = -1;
      ASSERT_TRUE(gotAfter.tryGet<F64>(yAfter));
      EXPECT_EQ(yAfter, 99.0);
   }

   TEST(StructValueTests, InstanceMethod_InvokeComputesFromRealComponents)
   {
      VectorF v(3.0f, 4.0f, 0.0f);
      ScriptValue sv = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(v);
      const StructTypeRep* type = sv.structType();
      ASSERT_NE(type, nullptr);

      const ScriptStructMethodRep* lengthMethod = type->findMethod(StringTable->insert("length"));
      ASSERT_NE(lengthMethod, nullptr);

      ScriptValue result = lengthMethod->invoke(sv, ScriptValueSpan());
      F64 len = -1;
      ASSERT_TRUE(result.tryGet<F64>(len));
      EXPECT_DOUBLE_EQ(len, 5.0) << "3-4-5 triangle - length() must read the real x/y/z, not defaults";
   }

   TEST(StructValueTests, StaticMethod_RegisteredAsGlobalScriptFunction)
   {
      bool found = false;
      for (const HostFunctionDecl& decl : HostBindingRegistry::instance().globalFunctions())
      {
         if (decl.name == StringTable->insert("cross"))
            found = true;
      }
      EXPECT_TRUE(found) << "SCRIPT_STRUCT_STATIC_METHODS entries must be reachable as ordinary global functions";
   }

   TEST(StructValueTests, FieldOnOrdinaryScriptClass_RoundTripsThroughFieldAccessor)
   {
      // Plain field type, no special-casing needed in FieldAccessor.
      VectorWidget* w = new VectorWidget();
      const ScriptClassRep* rep = w->getRuntimeClassRep();
      ASSERT_NE(rep, nullptr);

      const ScriptFieldRep* velocityField = rep->findField(StringTable->insert("mVelocity"));
      ASSERT_NE(velocityField, nullptr);

      ScriptValue got = velocityField->get(w);
      ASSERT_EQ(got.kind(), ScriptValue::Kind::Struct);

      bool setOk = velocityField->set(w, newConsole::detail::ScriptTypeTraits<VectorF>::toScript(VectorF(10.0f, 20.0f, 30.0f)));
      EXPECT_TRUE(setOk);
      EXPECT_FLOAT_EQ(w->mVelocity.x, 10.0f) << "set() must write through to the real VectorF member, not a copy";
      EXPECT_FLOAT_EQ(w->mVelocity.y, 20.0f);
      EXPECT_FLOAT_EQ(w->mVelocity.z, 30.0f);

      w->decRefCount();
   }

   namespace
   {

      class StructScriptTests : public ::testing::Test
      {
      protected:
         void SetUp() override
         {
            runtime = new TorqueScript2Runtime();
            ASSERT_TRUE(newConsole::registerRuntime(runtime));
         }

         void TearDown() override
         {
            newConsole::shutdownAll();
         }

         TorqueScript2Runtime* runtime = nullptr;
      };

   } // namespace

   TEST_F(StructScriptTests, DotAccess_ReadsNamedComponent)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function getX(%v) { return %v.x; }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue vArg = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(VectorF(5.0f, 6.0f, 7.0f));
      ScriptValue result = runtime->callFunction(StringTable->insert("getX"), ScriptValueSpan(&vArg, 1));

      F64 x = -1;
      ASSERT_TRUE(result.tryGet<F64>(x));
      EXPECT_EQ(x, 5.0) << "%v.x must resolve through the struct's own StructTypeRep, not ObjectRegistry";
   }

   TEST_F(StructScriptTests, DotAssign_WritesNamedComponent)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function setXAndReturnWhole(%v) { %v.x = 42; return %v; }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue vArg = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(VectorF(1.0f, 2.0f, 3.0f));
      ScriptValue result = runtime->callFunction(StringTable->insert("setXAndReturnWhole"), ScriptValueSpan(&vArg, 1));

      ASSERT_EQ(result.kind(), ScriptValue::Kind::Struct)
         << "%v alone (the whole value) must remain just as usable as %v.x - returning the local still yields a struct value";

      VectorF back;
      ASSERT_TRUE(newConsole::detail::ScriptTypeTraits<VectorF>::fromScript(back, result));
      EXPECT_FLOAT_EQ(back.x, 42.0f) << "%v.x = 42 must have mutated the component";
      EXPECT_FLOAT_EQ(back.y, 2.0f) << "unassigned components must be left untouched";
      EXPECT_FLOAT_EQ(back.z, 3.0f);
   }

   TEST_F(StructScriptTests, MethodCall_InvokesRealCppMethodFromScript)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function getLength(%v) { return %v.length(); }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue vArg = newConsole::detail::ScriptTypeTraits<VectorF>::toScript(VectorF(3.0f, 4.0f, 0.0f));
      ScriptValue result = runtime->callFunction(StringTable->insert("getLength"), ScriptValueSpan(&vArg, 1));

      F64 len = -1;
      ASSERT_TRUE(result.tryGet<F64>(len));
      EXPECT_DOUBLE_EQ(len, 5.0);
   }

   TEST_F(StructScriptTests, StaticFunctionCall_CrossCallableAsGlobalFunction)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function perpDot(%a, %b) { %c = cross(%a, %b); return %c.z; }", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue args[2] = {
         newConsole::detail::ScriptTypeTraits<VectorF>::toScript(VectorF(1.0f, 0.0f, 0.0f)),
         newConsole::detail::ScriptTypeTraits<VectorF>::toScript(VectorF(0.0f, 1.0f, 0.0f))
      };
      ScriptValue result = runtime->callFunction(StringTable->insert("perpDot"), ScriptValueSpan(args, 2));

      F64 z = -1;
      ASSERT_TRUE(result.tryGet<F64>(z));
      EXPECT_DOUBLE_EQ(z, 1.0) << "cross((1,0,0),(0,1,0)) = (0,0,1) - registered SCRIPT_STRUCT_STATIC_METHODS entry callable as an ordinary global function";
   }

   TEST_F(StructScriptTests, FieldOnScriptClass_ScriptSideDotAccessReadsRealMember)
   {
      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function getVelocityX(%obj) { return %obj.mVelocity.x; }", diags);
      ASSERT_TRUE(lr.success);

      VectorWidget* w = new VectorWidget();
      w->mVelocity = VectorF(11.0f, 22.0f, 33.0f);
      ObjectHandle handle = newConsole::registerObject(w);
      ScriptValue arg = ScriptValue::makeObject(handle);

      ScriptValue result = runtime->callFunction(StringTable->insert("getVelocityX"), ScriptValueSpan(&arg, 1));
      F64 x = -1;
      ASSERT_TRUE(result.tryGet<F64>(x));
      EXPECT_EQ(x, 11.0) << "chained access: %obj.mVelocity (Object->Struct field) then .x (Struct->component)";
   }

} // namespace newConsole_test

// MultiDimArrayTests: %grid[i][j] chains two GetIndex ops. Building
// from nothing needed vivification - see scriptArraySet/emitStoreTo.

namespace newConsole_test
{

   TEST(MultiDimArrayTests, ScriptSide_AssignFromNothing_VivifiesBothLevels)
   {
      TorqueScript2Runtime* runtime = new TorqueScript2Runtime();
      ASSERT_TRUE(newConsole::registerRuntime(runtime));

      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function buildFromNothing() {"
         "   %multi_dimen[10][5] = 42;"
         "   return %multi_dimen[10][5];"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("buildFromNothing"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v)) << "must auto-vivify both levels from Null";
      EXPECT_EQ(v, 42);

      newConsole::shutdownAll();
   }

   TEST(MultiDimArrayTests, ScriptSide_AssignFromNothing_OtherCellsReadAsNull)
   {
      TorqueScript2Runtime* runtime = new TorqueScript2Runtime();
      ASSERT_TRUE(newConsole::registerRuntime(runtime));

      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function checkUntouchedCell() {"
         "   %grid[10][5] = 42;"
         "   return %grid[0][0];"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("checkUntouchedCell"), ScriptValueSpan());
      EXPECT_EQ(result.kind(), ScriptValue::Kind::Null) << "vivifying [10][5] must not fabricate [0][0]";

      newConsole::shutdownAll();
   }

   TEST(MultiDimArrayTests, ScriptSide_ThreeLevelsDeep_VivifiesAllLevels)
   {
      // confirms the recursion actually recurses, not hardcoded to 2 levels.
      TorqueScript2Runtime* runtime = new TorqueScript2Runtime();
      ASSERT_TRUE(newConsole::registerRuntime(runtime));

      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function buildCube() {"
         "   %cube[1][2][3] = 7;"
         "   return %cube[1][2][3];"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("buildCube"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v)) << "three levels of [] must all vivify, not just two";
      EXPECT_EQ(v, 7);

      newConsole::shutdownAll();
   }

   TEST(MultiDimArrayTests, ScriptSide_NestedIndexRead)
   {
      TorqueScript2Runtime* runtime = new TorqueScript2Runtime();
      ASSERT_TRUE(newConsole::registerRuntime(runtime));

      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function buildAndRead() {"
         "   %row0[0] = 1; %row0[1] = 2;"
         "   %row1[0] = 3; %row1[1] = 4;"
         "   %grid[0] = %row0; %grid[1] = %row1;"
         "   return %grid[1][0];"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("buildAndRead"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 3) << "%grid[1][0] must resolve as two chained GetIndex ops - row 1, column 0";

      newConsole::shutdownAll();
   }

   TEST(MultiDimArrayTests, ScriptSide_NestedIndexWriteMutatesSharedStorage)
   {
      TorqueScript2Runtime* runtime = new TorqueScript2Runtime();
      ASSERT_TRUE(newConsole::registerRuntime(runtime));

      DiagnosticSink diags;
      LoadResult lr = runtime->loadSource("t.ts2",
         "function buildAndMutate() {"
         "   %row[0] = 10; %row[1] = 20;"
         "   %grid[0] = %row;"
         "   %grid[0][1] = 99;"
         "   return %grid[0][1];"
         "}", diags);
      ASSERT_TRUE(lr.success);

      ScriptValue result = runtime->callFunction(StringTable->insert("buildAndMutate"), ScriptValueSpan());
      S64 v = -1;
      ASSERT_TRUE(result.tryGet<S64>(v));
      EXPECT_EQ(v, 99) << "%grid[0][1] = 99 must write through the shared inner-array storage, same as arrayRef()'s own sharing contract";

      newConsole::shutdownAll();
   }

   TEST(MultiDimArrayTests, HostSide_ArrayOfArraysBuiltDirectlyThroughScriptValue)
   {
      ScriptValue grid = ScriptValue::makeArray();

      ScriptValue row0 = ScriptValue::makeArray();
      row0.arrayRef().push_back(ScriptValue::makeInt(1));
      row0.arrayRef().push_back(ScriptValue::makeInt(2));

      ScriptValue row1 = ScriptValue::makeArray();
      row1.arrayRef().push_back(ScriptValue::makeInt(3));
      row1.arrayRef().push_back(ScriptValue::makeInt(4));

      grid.arrayRef().push_back(row0);
      grid.arrayRef().push_back(row1);

      ASSERT_EQ(grid.kind(), ScriptValue::Kind::Array);
      ASSERT_EQ(grid.arrayRef()[1].kind(), ScriptValue::Kind::Array);

      S64 v = -1;
      ASSERT_TRUE(grid.arrayRef()[1].arrayRef()[0].tryGet<S64>(v));
      EXPECT_EQ(v, 3);
   }

   // =============================================================================
   // BytecodeSerializeTests / ScriptCompilerTests: the .tsc format,
   // round-tripped through real disk I/O against a scratch directory.
   // =============================================================================

   namespace
   {

      /// gtest doesn't run engine startup, so no Torque::FS mount exists
      /// yet - mount one real native OS directory once, under its own
      /// root, so ScratchDir's paths actually resolve to disk.
      StringTableEntry ensureScratchMount()
      {
         static bool sMounted = false;
         static const char* kRoot = "tsc_scratch";
         if (!sMounted)
         {
            String base = Torque::Path(StringTable->insert(Platform::getExecutablePath())).getPath();
            Torque::FS::Mount(kRoot, Platform::FS::createNativeFS(base));
            sMounted = true;
         }
         return StringTable->insert(kRoot);
      }

      /// Creates a scratch directory for one test, removed on destruction.
      /// Per-process timestamp salt - if a previous run's cleanup silently
      /// failed, leftover files can't collide with this run's paths.
      class ScratchDir
      {
      public:
         ScratchDir()
         {
            static std::atomic<U32> sCounter{ 0 };
            static U32 sRunSalt = Platform::getTime();
            StringTableEntry root = ensureScratchMount();
            mPath = String(root) + ":/tsc_scratch_" + String::ToString(sRunSalt)
               + "_" + String::ToString(sCounter.fetch_add(1));
            Torque::FS::CreatePath(Torque::Path(mPath));
         }

         ~ScratchDir()
         {
            // Path::isDirectory() requires an empty file/extension, which
            // only a trailing '/' produces (see Path::_split) - required
            // for OpenDirectory to resolve mPath as a directory, not a
            // mis-parsed "file" with mPath's last segment as its name.
            removeRecursive(Torque::Path(mPath + "/"));
         }

         const String& path() const { return mPath; }

         String join(const char* relative) const
         {
            return mPath + "/" + relative;
         }

      private:
         static void removeRecursive(const Torque::Path& dir)
         {
            Torque::FS::DirectoryRef dirRef = Torque::FS::OpenDirectory(dir);
            if (!dirRef)
               return;

            Vector<Torque::Path> files;
            dirRef->dumpFiles(files);
            Vector<Torque::Path> subDirs;
            dirRef->dumpDirectories(subDirs);
            dirRef->close();

            for (U32 i = 0; i < files.size(); ++i)
               Torque::FS::Remove(files[i]);
            for (U32 i = 0; i < subDirs.size(); ++i)
               removeRecursive(Torque::Path(subDirs[i].getFullPath() + "/"));
            Torque::FS::Remove(dir);
         }

         String mPath;
      };

      bool writeTextFile(const char* path, const char* text)
      {
         FileStream* stream = FileStream::createAndOpen(path, Torque::FS::File::Write);
         if (!stream)
            return false;
         bool ok = stream->write(static_cast<U32>(dStrlen(text)), text);
         delete stream;
         return ok;
      }

   } // namespace

   TEST(DebugAdapterSerializationTests, StrippedUnit_NeverTriggersBreakpoint)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("bp_stripped.ts2");
      String dstPath = scratch.join("bp_stripped.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function plain(%x) { return %x + 1; }"));

      CompileFileResult compileResult = compileScriptFile(srcPath.c_str(), dstPath.c_str(), /*stripDebugInfo*/ true);
      ASSERT_TRUE(compileResult.success);

      CompiledModule module;
      FileStream* in = FileStream::createAndOpen(dstPath.c_str(), Torque::FS::File::Read);
      ASSERT_NE(in, nullptr);
      ASSERT_TRUE(readCompiledModule(*in, module));
      delete in;

      ASSERT_EQ(module.functionUnits.size(), 1u);
      EXPECT_EQ(module.functionUnits[0].origin, nullptr);
      EXPECT_TRUE(module.functionUnits[0].localDebugInfo.empty());
   }

   TEST(ScriptCompilerTreeTests, PathIsDirectory_RequiresTrailingSlash)
   {
      // Pins down the mechanism behind the OpenDirectory/FindByPattern
      // bug above: a path with no trailing slash parses its last segment
      // as a filename (mFile), not as part of the directory (mPath).
      EXPECT_FALSE(Torque::Path("tsc_scratch:/some_dir").isDirectory());
      EXPECT_TRUE(Torque::Path("tsc_scratch:/some_dir/").isDirectory());
   }

   TEST(ScriptCompilerTreeTests, OpenDirectory_DumpFilesFindsWrittenFile)
   {
      // Regression test: Path::isDirectory() requires an empty file/
      // extension, which only a trailing '/' produces (see Path::_split).
      // Without it, OpenDirectory/FindByPattern resolve the path's last
      // segment as a filename instead of descending into it - dumpFiles
      // then deterministically finds nothing, not a timing issue.
      ScratchDir scratch;
      ASSERT_TRUE(writeTextFile(scratch.join("probe.ts2").c_str(), "function probe() { return 1; }"));
      EXPECT_TRUE(hasScriptExtension(scratch.join("probe.ts2").c_str()));

      Torque::Path dirPath(scratch.path() + "/");
      ASSERT_TRUE(dirPath.isDirectory()) << "test setup bug: path must be directory-shaped";

      Torque::FS::DirectoryRef dirRef = Torque::FS::OpenDirectory(dirPath);
      ASSERT_NE(dirRef, nullptr) << "OpenDirectory failed on a freshly-created scratch dir";

      Vector<Torque::Path> files;
      ASSERT_TRUE(dirRef->dumpFiles(files));
      dirRef->close();

      ASSERT_EQ(files.size(), 1u);
      EXPECT_TRUE(hasScriptExtension(files[0].getFullPath().c_str()));
   }

   TEST(BytecodeSerializeTests, RoundTrip_PreservesInstructionsAndConstants)
   {
      ScratchDir scratch;
      String path = scratch.join("unit.bin");

      // Reused for fixture and comparison - StringTable::insert is
      // case-insensitive, so a fresh literal could mismatch on casing.
      StringTableEntry fnName = StringTable->insert("foo");
      StringTableEntry strConst = StringTable->insert("hello");

      CompiledModule module;
      module.sourceHash = hashSource("function foo() {}", 18);

      BytecodeUnit unit;
      unit.name = fnName;
      Instruction instr;
      instr.op = OpCode::LoadInt;
      instr.a = 0;
      instr.bx = 0;
      unit.code.push_back(instr);
      unit.intConsts.push_back(42);
      unit.stringConsts.push_back(strConst);
      unit.registerCount = 1;
      unit.paramCount = 0;
      unit.lineTable.push_back(1);

      module.functionNames.push_back(fnName);
      module.functionUnits.push_back(unit);

      {
         FileStream* out = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Write);
         ASSERT_NE(out, nullptr);
         String err;
         ASSERT_TRUE(writeCompiledModule(*out, module, &err)) << err.c_str();
         delete out;
      }

      CompiledModule readBack;
      {
         FileStream* in = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Read);
         ASSERT_NE(in, nullptr);
         String err;
         ASSERT_TRUE(readCompiledModule(*in, readBack, &err)) << err.c_str();
         delete in;
      }

      EXPECT_EQ(readBack.sourceHash, module.sourceHash);
      ASSERT_EQ(readBack.functionNames.size(), 1u);
      EXPECT_EQ(readBack.functionNames[0], fnName) << "interned pointer must round-trip, not just matching text";
      ASSERT_EQ(readBack.functionUnits.size(), 1u);
      EXPECT_EQ(readBack.functionUnits[0].code.size(), 1u);
      EXPECT_EQ(readBack.functionUnits[0].intConsts[0], 42);
      EXPECT_EQ(readBack.functionUnits[0].stringConsts[0], strConst);
      EXPECT_EQ(readBack.functionUnits[0].registerCount, 1);
   }

   TEST(BytecodeSerializeTests, RoundTrip_EmptyLineTable_StaysEmpty)
   {
      ScratchDir scratch;
      String path = scratch.join("stripped.bin");

      CompiledModule module;
      BytecodeUnit unit;
      unit.name = StringTable->insert("bar");
      module.functionNames.push_back(StringTable->insert("bar"));
      module.functionUnits.push_back(unit); // lineTable left empty - simulates stripDebugInfo

      {
         FileStream* out = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Write);
         ASSERT_NE(out, nullptr);
         ASSERT_TRUE(writeCompiledModule(*out, module));
         delete out;
      }

      CompiledModule readBack;
      FileStream* in = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Read);
      ASSERT_NE(in, nullptr);
      ASSERT_TRUE(readCompiledModule(*in, readBack));
      delete in;

      EXPECT_EQ(readBack.functionUnits[0].lineTable.size(), 0u);
   }

   TEST(BytecodeSerializeTests, Read_RejectsBadMagic)
   {
      ScratchDir scratch;
      String path = scratch.join("bad.bin");
      ASSERT_TRUE(writeTextFile(path.c_str(), "NOT A TSC FILE AT ALL"));

      FileStream* in = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Read);
      ASSERT_NE(in, nullptr);
      CompiledModule readBack;
      String err;
      EXPECT_FALSE(readCompiledModule(*in, readBack, &err));
      EXPECT_FALSE(err.isEmpty());
      delete in;
   }

   TEST(BytecodeSerializeTests, Read_RejectsTruncatedFile)
   {
      ScratchDir scratch;
      String path = scratch.join("truncated.bin");

      // Valid header, no payload - must fail cleanly, not read past end.
      {
         FileStream* out = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Write);
         ASSERT_NE(out, nullptr);
         const U8 magic[4] = { 'T', 'S', 'C', '2' };
         const U8 version = 1;
         ASSERT_TRUE(out->write(sizeof(magic), magic));
         ASSERT_TRUE(out->write(sizeof(version), &version));
         delete out;
      }

      FileStream* in = FileStream::createAndOpen(path.c_str(), Torque::FS::File::Read);
      ASSERT_NE(in, nullptr);
      CompiledModule readBack;
      EXPECT_FALSE(readCompiledModule(*in, readBack));
      delete in;
   }

   TEST(ScriptCompilerFileTests, CompilesValidScriptAndProducesReadableTsc)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("test.ts2");
      String dstPath = scratch.join("test.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function addOne(%x) { return %x + 1; }"));

      CompileFileResult result = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      EXPECT_TRUE(result.success) << result.errorMessage.c_str();
      EXPECT_FALSE(result.skippedUpToDate);

      FileStream* in = FileStream::createAndOpen(dstPath.c_str(), Torque::FS::File::Read);
      ASSERT_NE(in, nullptr);
      CompiledModule module;
      ASSERT_TRUE(readCompiledModule(*in, module));
      delete in;

      ASSERT_EQ(module.functionNames.size(), 1u);
      EXPECT_STREQ(module.functionNames[0], "addOne");
   }

   TEST(ScriptCompilerFileTests, DefaultDestination_SwapsExtensionToTsc)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("noDst.ts2");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f() { return 1; }"));

      CompileFileResult result = compileScriptFile(srcPath.c_str());
      ASSERT_TRUE(result.success) << result.errorMessage.c_str();

      String expectedDst = scratch.join("noDst.tsc");
      EXPECT_TRUE(Torque::FS::IsFile(Torque::Path(expectedDst)));
   }

   TEST(ScriptCompilerFileTests, SecondCompile_SkipsWhenUpToDate)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("cache.ts2");
      String dstPath = scratch.join("cache.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f() { return 1; }"));

      CompileFileResult first = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      ASSERT_TRUE(first.success);
      EXPECT_FALSE(first.skippedUpToDate);

      CompileFileResult second = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      EXPECT_TRUE(second.success);
      EXPECT_TRUE(second.skippedUpToDate) << "unchanged source must not be recompiled";
   }

   TEST(ScriptCompilerFileTests, ChangedSource_RecompilesInsteadOfSkipping)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("changing.ts2");
      String dstPath = scratch.join("changing.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f() { return 1; }"));

      CompileFileResult first = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      ASSERT_TRUE(first.success);

      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f() { return 2; }"));
      CompileFileResult second = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      EXPECT_TRUE(second.success);
      EXPECT_FALSE(second.skippedUpToDate) << "changed source must be recompiled even though a .tsc already existed";
   }

   TEST(ScriptCompilerFileTests, ForceFlag_RecompilesEvenWhenUpToDate)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("forced.ts2");
      String dstPath = scratch.join("forced.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f() { return 1; }"));

      CompileFileResult first = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      ASSERT_TRUE(first.success);

      CompileFileResult forced = compileScriptFile(srcPath.c_str(), dstPath.c_str(), /*stripDebugInfo*/ false, /*force*/ true);
      EXPECT_TRUE(forced.success);
      EXPECT_FALSE(forced.skippedUpToDate) << "force must bypass the up-to-date check";
   }

   TEST(ScriptCompilerFileTests, StripDebugInfo_OmitsLineTable)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("debug.ts2");
      String strippedPath = scratch.join("stripped.tsc");
      String fullPath = scratch.join("full.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f() { return 1 + 2; }"));

      CompileFileResult stripped = compileScriptFile(srcPath.c_str(), strippedPath.c_str(), /*stripDebugInfo*/ true);
      CompileFileResult full = compileScriptFile(srcPath.c_str(), fullPath.c_str(), /*stripDebugInfo*/ false);
      ASSERT_TRUE(stripped.success);
      ASSERT_TRUE(full.success);

      CompiledModule strippedModule, fullModule;
      {
         FileStream* in = FileStream::createAndOpen(strippedPath.c_str(), Torque::FS::File::Read);
         ASSERT_NE(in, nullptr);
         ASSERT_TRUE(readCompiledModule(*in, strippedModule));
         delete in;
      }
      {
         FileStream* in = FileStream::createAndOpen(fullPath.c_str(), Torque::FS::File::Read);
         ASSERT_NE(in, nullptr);
         ASSERT_TRUE(readCompiledModule(*in, fullModule));
         delete in;
      }

      EXPECT_EQ(strippedModule.functionUnits[0].lineTable.size(), 0u);
      EXPECT_GT(fullModule.functionUnits[0].lineTable.size(), 0u);
   }

   TEST(ScriptCompilerFileTests, SyntaxError_FailsWithMessageAndWritesNothing)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("broken.ts2");
      String dstPath = scratch.join("broken.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function f( { return"));

      CompileFileResult result = compileScriptFile(srcPath.c_str(), dstPath.c_str());
      EXPECT_FALSE(result.success);
      EXPECT_FALSE(result.errorMessage.isEmpty());
      EXPECT_FALSE(Torque::FS::IsFile(Torque::Path(dstPath))) << "a failed compile must not leave a partial/corrupt .tsc behind";
   }

   TEST(ScriptCompilerFileTests, MissingSourceFile_FailsCleanly)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("doesNotExist.ts2");

      CompileFileResult result = compileScriptFile(srcPath.c_str());
      EXPECT_FALSE(result.success);
      EXPECT_FALSE(result.errorMessage.isEmpty());
   }

   TEST(ScriptCompilerTreeTests, CompilesEveryTs2FileRecursively)
   {
      ScratchDir srcScratch;
      ScratchDir dstScratch;

      ASSERT_TRUE(writeTextFile(srcScratch.join("top.ts2").c_str(), "function top() { return 1; }"));
      Torque::FS::CreatePath(Torque::Path(srcScratch.join("sub")));
      ASSERT_TRUE(writeTextFile(srcScratch.join("sub/nested.ts2").c_str(), "function nested() { return 2; }"));
      // Non-.ts2 file must be ignored, not error the whole tree.
      ASSERT_TRUE(writeTextFile(srcScratch.join("readme.txt").c_str(), "not a script"));

      CompileTreeResult result = compileScriptTree(srcScratch.path().c_str(), dstScratch.path().c_str());
      ASSERT_EQ(result.filesFound, 2u) << "filesFound now reflects FindByPattern's own *.ts2 match count (readme.txt correctly excluded)";
      EXPECT_TRUE(result.success);
      EXPECT_EQ(result.filesCompiled, 2u);
      EXPECT_EQ(result.failures.size(), 0u);

      EXPECT_TRUE(Torque::FS::IsFile(Torque::Path(dstScratch.join("top.tsc"))));
      EXPECT_TRUE(Torque::FS::IsFile(Torque::Path(dstScratch.join("sub/nested.tsc"))));
      EXPECT_FALSE(Torque::FS::IsFile(Torque::Path(dstScratch.join("readme.tsc"))));
   }

   TEST(ScriptCompilerTreeTests, TrailingSlashesOnEitherDir_StillProduceCorrectLayout)
   {
      ScratchDir srcScratch;
      ScratchDir dstScratch;
      ASSERT_TRUE(writeTextFile(srcScratch.join("f.ts2").c_str(), "function f() { return 1; }"));

      String srcWithSlash = srcScratch.path() + "/";
      String dstWithSlash = dstScratch.path() + "/";

      CompileTreeResult result = compileScriptTree(srcWithSlash.c_str(), dstWithSlash.c_str());
      EXPECT_TRUE(result.success);
      EXPECT_EQ(result.filesCompiled, 1u);
      EXPECT_TRUE(Torque::FS::IsFile(Torque::Path(dstScratch.join("f.tsc"))))
         << "a trailing slash on src/dst must not produce a malformed output path";
   }

   TEST(ScriptCompilerTreeTests, OneFileFails_OthersStillCompile)
   {
      ScratchDir srcScratch;
      ScratchDir dstScratch;
      ASSERT_TRUE(writeTextFile(srcScratch.join("good.ts2").c_str(), "function good() { return 1; }"));
      ASSERT_TRUE(writeTextFile(srcScratch.join("bad.ts2").c_str(), "function bad( { broken"));

      CompileTreeResult result = compileScriptTree(srcScratch.path().c_str(), dstScratch.path().c_str());
      EXPECT_FALSE(result.success);
      EXPECT_EQ(result.filesCompiled, 1u) << "the one good file must still compile despite the other failing";
      ASSERT_EQ(result.failures.size(), 1u);
      EXPECT_NE(result.failures[0].find("bad.ts2"), String::NPos) << "failure entry must identify which file failed";

      EXPECT_TRUE(Torque::FS::IsFile(Torque::Path(dstScratch.join("good.tsc"))));
      EXPECT_FALSE(Torque::FS::IsFile(Torque::Path(dstScratch.join("bad.tsc"))));
   }

   TEST(ScriptCompilerTreeTests, SecondRun_SkipsUnchangedFiles)
   {
      ScratchDir srcScratch;
      ScratchDir dstScratch;
      ASSERT_TRUE(writeTextFile(srcScratch.join("a.ts2").c_str(), "function a() { return 1; }"));
      ASSERT_TRUE(writeTextFile(srcScratch.join("b.ts2").c_str(), "function b() { return 2; }"));

      CompileTreeResult first = compileScriptTree(srcScratch.path().c_str(), dstScratch.path().c_str());
      ASSERT_TRUE(first.success);
      EXPECT_EQ(first.filesCompiled, 2u);
      EXPECT_EQ(first.filesUpToDate, 0u);

      CompileTreeResult second = compileScriptTree(srcScratch.path().c_str(), dstScratch.path().c_str());
      EXPECT_TRUE(second.success);
      EXPECT_EQ(second.filesCompiled, 0u);
      EXPECT_EQ(second.filesUpToDate, 2u);
   }

   // =============================================================================
   // GlobalScriptMethodTests: GLOBAL_SCRIPT_METHOD - classless free
   // function, registered into HostBindingRegistry.
   // =============================================================================

   TEST(GlobalScriptMethodTests, ScriptCompileFile_IsRegisteredAsGlobalFunction)
   {
      bool found = false;
      for (const HostFunctionDecl& decl : HostBindingRegistry::instance().globalFunctions())
      {
         if (dStricmp(decl.name, "scriptCompileFile") == 0)
         {
            found = true;
            break;
         }
      }
      EXPECT_TRUE(found) << "GLOBAL_SCRIPT_METHOD(scriptCompileFile) must self-register at static init";
   }

   TEST(GlobalScriptMethodTests, ScriptCompileFile_CallableThroughRegisteredTrampoline)
   {
      ScratchDir scratch;
      String srcPath = scratch.join("viaGlobal.ts2");
      String dstPath = scratch.join("viaGlobal.tsc");
      ASSERT_TRUE(writeTextFile(srcPath.c_str(), "function g() { return 1; }"));

      const HostFunctionDecl* decl = nullptr;
      for (const HostFunctionDecl& d : HostBindingRegistry::instance().globalFunctions())
      {
         if (dStricmp(d.name, "scriptCompileFile") == 0)
         {
            decl = &d;
            break;
         }
      }
      ASSERT_NE(decl, nullptr);

      ScriptValue args[4] = {
         ScriptValue::makeString(srcPath.c_str()),
         ScriptValue::makeString(dstPath.c_str()),
         ScriptValue::makeBool(false),
         ScriptValue::makeBool(false)
      };
      ScriptValue result = decl->invoke(ScriptValueSpan(args, 4));

      bool success = false;
      ASSERT_TRUE(result.tryGet<bool>(success));
      EXPECT_TRUE(success);
      EXPECT_TRUE(Torque::FS::IsFile(Torque::Path(dstPath)));
   }

} // namespace newConsole_test
