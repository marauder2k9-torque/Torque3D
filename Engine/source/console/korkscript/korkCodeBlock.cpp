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

#include "korkCodeBlock.h"
#include "korkdebug/breakpointTable.h"
#include "ast/opcodes.h"
#include "ast/nodes.h"

#include "console/console.h"
#include "console/consoleInternal.h"    // Namespace, Script::gEvalState
#include "console/simBase.h"
#include "sim/netStringTable.h"
#include "console/stringStack.h"
#include "util/messaging/message.h"
#include "core/frameAllocator.h"

#include "console/returnBuffer.h"
#include "console/consoleValueStack.h"
#include "console/telnetDebugger.h"
#include "korkEvalState.h"
#ifndef TORQUE_TGB_ONLY
#include "materials/materialDefinition.h"
#endif


namespace Con
{
   // Current script file name and root, these are registered as
   // console variables.
   extern StringTableEntry gCurrentFile;
   extern StringTableEntry gCurrentRoot;
   extern S32 gObjectCopyFailures;
}



namespace KorkScript
{

   static inline StringTableEntry readSTE(const U32* code, U32 ip)
   {
#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_ARM64)
      return (StringTableEntry)(*((const U64*)(code + ip)));
#else
      return (StringTableEntry)(*(code + ip));
#endif
   }


   SimObject* getThisObject(ConsoleValue& simObjectLookupValue)
   {
      SimObject* thisObject = NULL;

      // Optimization: If we're an integer, we can lookup the value by SimObjectId
      if (simObjectLookupValue.getType() == ConsoleValueType::cvInteger)
         thisObject = Sim::findObject(static_cast<SimObjectId>(simObjectLookupValue.getFastInt()));
      else
      {
         SimObject* foundObject = Sim::findObject(simObjectLookupValue.getString());

         // Optimization: If we're not an integer, let's make it so that the fast path exists
         // on the first argument of the method call (speeds up future usage of %this, for example)
         if (foundObject != NULL)
            simObjectLookupValue.setInt(static_cast<S64>(foundObject->getId()));

         thisObject = foundObject;
      }

      return thisObject;
   }


   /// Frame data for a foreach/foreach$ loop.
   struct IterStackRecord
   {
      /// If true, this is a foreach$ loop; if not, it's a foreach loop.
      bool mIsStringIter;

      /// True if the variable referenced is a global
      bool mIsGlobalVariable;

      union
      {

         /// The iterator variable if we are a global variable
         Dictionary::Entry* mVariable;

         /// The register variable if we are a local variable
         S32 mRegister;
      } mVar;

      /// Information for an object iterator loop.
      struct ObjectPos
      {
         /// The set being iterated over.
         SimSet* mSet;

         /// Current index in the set.
         U32 mIndex;
      };

      /// Information for a string iterator loop.
      struct StringPos
      {
         /// The raw string data on the string stack.
         const char* mString;

         /// Current parsing position.
         U32 mIndex;
      };
      union
      {
         ObjectPos mObj;
         StringPos mStr;
      } mData;
   };

   static void getFieldComponent(SimObject* object,
                                 StringTableEntry field,
                                 const char* array,
                                 StringTableEntry subField,
                                 char val[],
                                 S32 currentLocalRegister)
   {
      const char* prevVal = NULL;

      if (object && field)
         prevVal = object->getDataField(field, array);
      else if (currentLocalRegister != -1)
         prevVal = KorkScript::gEvalState.getLocalStringVariable(currentLocalRegister);
      else if (KorkScript::gEvalState.currentVariable)
         prevVal = KorkScript::gEvalState.getStringVariable();

      // Make sure we got a value.
      if (prevVal && *prevVal)
      {
         static const StringTableEntry xyzw[] =
         {
            StringTable->insert("x"),
            StringTable->insert("y"),
            StringTable->insert("z"),
            StringTable->insert("w")
         };

         static const StringTableEntry rgba[] =
         {
            StringTable->insert("r"),
            StringTable->insert("g"),
            StringTable->insert("b"),
            StringTable->insert("a")
         };

         // Translate xyzw and rgba into the indexed component
         // of the variable or field.
         if (subField == xyzw[0] || subField == rgba[0])
            dStrcpy(val, StringUnit::getUnit(prevVal, 0, " \t\n"), 128);

         else if (subField == xyzw[1] || subField == rgba[1])
            dStrcpy(val, StringUnit::getUnit(prevVal, 1, " \t\n"), 128);

         else if (subField == xyzw[2] || subField == rgba[2])
            dStrcpy(val, StringUnit::getUnit(prevVal, 2, " \t\n"), 128);

         else if (subField == xyzw[3] || subField == rgba[3])
            dStrcpy(val, StringUnit::getUnit(prevVal, 3, " \t\n"), 128);

         else
            val[0] = 0;
      }
      else
         val[0] = 0;
   }

   static void setFieldComponent(SimObject* object,
                                 StringTableEntry field,
                                 const char* array,
                                 StringTableEntry subField,
                                 StringTableEntry strValue,
                                 S32 currentLocalRegister)
   {
      char val[1024] = "";
      const char* prevVal = NULL;

      if (object && field)
         prevVal = object->getDataField(field, array);
      else if (currentLocalRegister != -1)
         prevVal = KorkScript::gEvalState.getLocalStringVariable(currentLocalRegister);
      // Set the value on a variable.
      else if (KorkScript::gEvalState.currentVariable)
         prevVal = KorkScript::gEvalState.getStringVariable();

      // Ensure that the variable has a value
      if (!prevVal)
         return;

      static const StringTableEntry xyzw[] =
      {
         StringTable->insert("x"),
         StringTable->insert("y"),
         StringTable->insert("z"),
         StringTable->insert("w")
      };

      static const StringTableEntry rgba[] =
      {
         StringTable->insert("r"),
         StringTable->insert("g"),
         StringTable->insert("b"),
         StringTable->insert("a")
      };

      // Insert the value into the specified
      // component of the string.
      if (subField == xyzw[0] || subField == rgba[0])
         dStrcpy(val, StringUnit::setUnit(prevVal, 0, strValue, " \t\n"), 128);

      else if (subField == xyzw[1] || subField == rgba[1])
         dStrcpy(val, StringUnit::setUnit(prevVal, 1, strValue, " \t\n"), 128);

      else if (subField == xyzw[2] || subField == rgba[2])
         dStrcpy(val, StringUnit::setUnit(prevVal, 2, strValue, " \t\n"), 128);

      else if (subField == xyzw[3] || subField == rgba[3])
         dStrcpy(val, StringUnit::setUnit(prevVal, 3, strValue, " \t\n"), 128);

      if (val[0] != 0)
      {
         // Update the field or variable.
         if (object && field)
            object->setDataField(field, 0, val);
         else if (currentLocalRegister != -1)
            KorkScript::gEvalState.setLocalStringVariable(currentLocalRegister, val, dStrlen(val));
         else if (KorkScript::gEvalState.currentVariable)
            KorkScript::gEvalState.setStringVariable(val);
      }
   }

   static const char* tsconcat(const char* a, const char* b, S32& outLen)
   {
      static char sBuffer[4096];
      S32 la = (S32)dStrlen(a);
      S32 lb = (S32)dStrlen(b);
      outLen = la + lb;
      if (outLen >= (S32)sizeof(sBuffer)) outLen = sizeof(sBuffer) - 1;
      dMemcpy(sBuffer, a, la);
      dMemcpy(sBuffer + la, b, lb);
      sBuffer[outLen] = '\0';
      return sBuffer;
   }

   static constexpr U32 kDSOVersion = 0x54533200;
   static constexpr S32 kMaxStackSize = 1024;
   static constexpr S32 kMaxIterDepth = 32;

   ConsoleValueStack<4096> gCallStack;

   KorkCodeBlock* KorkCodeBlock::smList = nullptr;

   KorkCodeBlock::KorkCodeBlock() { addToList(); }
   KorkCodeBlock::~KorkCodeBlock()
   {
      removeFromList();
      BreakpointTable::get().onCodeBlockUnloaded(this);
      delete[] code;
      delete[] globalStrings;
      delete[] functionStrings;
      delete[] globalFloats;
      delete[] functionFloats;
   }

   void KorkCodeBlock::addToList() { mNext = smList; smList = this; }
   void KorkCodeBlock::removeFromList()
   {
      KorkCodeBlock** w = &smList;
      while (*w && *w != this) w = &(*w)->mNext;
      if (*w) *w = mNext;
   }

   KorkCodeBlock* KorkCodeBlock::find(StringTableEntry n)
   {
      for (KorkCodeBlock* b = smList; b; b = b->mNext)
         if (b->name == n) return b;
      return nullptr;
   }

   void KorkCodeBlock::calcBreakList()
   {
      breakList.clear();
      if (!lineBreakPairs || !lineBreakPairCount) return;
      for (U32 i = 0; i < lineBreakPairCount; i++)
      {
         U32 ln = lineBreakPairs[i * 2];
         if (!breakList.empty() && breakList[breakList.size() - 2] == ln) continue;
         breakList.push_back(ln);
         breakList.push_back(lineBreakPairs[i * 2 + 1]);
      }
   }

   Con::EvalResult KorkCodeBlock::exec(U32               ip,
                                       const char*       functionName,
                                       Namespace*        thisNamespace,
                                       U32               argc,
                                       ConsoleValue*     argv,
                                       bool              noCalls,
                                       StringTableEntry  packageName,
                                       S32               setFrame)
   {

      ConsoleStackFrameSaver stackSaver;
      stackSaver.save();

      // -----------------------------------------------------------------------
      // Value stack — local to this exec() call.
      // stackTop is the index of the current top element.
      // Stack grows upward: stack[0] is the bottom.
      // -----------------------------------------------------------------------
      ConsoleValue stack[kMaxStackSize];
      S32 stackTop = -1;   // empty; first push does ++stackTop then writes

      //-----------------------------------------------------------------------
      // Iterator stack — one entry per active foreach loop.
      //-----------------------------------------------------------------------
      IterStackRecord iterStack[kMaxIterDepth];
      U32 iterTop = 0;   // next free slot (like stackTop but for iterators)
      U32 iterDepth = 0;   // how many nested foreach loops are active

      //-----------------------------------------------------------------------
      // Current string/float tables — swapped between global and function scope
      //-----------------------------------------------------------------------
      F64* curFloatTable = nullptr;
      char* curStringTable = nullptr;

      //-----------------------------------------------------------------------
      // Field access state — tracks the last object/field accessed by slot ops
      //-----------------------------------------------------------------------
      StringTableEntry curField = nullptr;
      StringTableEntry prevField = nullptr;
      SimObject* curObject = nullptr;
      SimObject* prevObject = nullptr;
      SimObject* thisObject = nullptr;
      SimObject* currentNewObject = nullptr;
      char             curFieldArray[256] = {};
      char             prevFieldArray[256] = {};

      // Object creation stack — needed for nested 'new' declarations
      static constexpr U32 kObjStackSize = 32;
      StringTableEntry objParent;
      struct ObjStackEntry { SimObject* obj; U32 failJump; };
      ObjStackEntry  objCreationStack[kObjStackSize];
      S32            objCreationDepth = 0;
      U32            failJump = 0;

      //-----------------------------------------------------------------------
      // Register / misc
      //-----------------------------------------------------------------------
      StringTableEntry callFnName = nullptr;
      StringTableEntry callFnNamespace = nullptr;
      StringTableEntry callFnPackage = nullptr;
      Namespace::Entry* nsEntry = nullptr;
      Namespace* ns = nullptr;
      S32               callArgc = 0;
      ConsoleValue* callArgv = nullptr;
      S32               currentRegister = -1;  // -1 = global/field mode
      ConsoleValue      returnValue;

      // Doc block state
      const char* curFNDocBlock = nullptr;
      const char* curNSDocBlock = nullptr;
      char        nsDocBlockClass[128] = {};

      const char* val = nullptr;

      static const S32 FieldBufferSizeNumeric = 32;
      static const S32 FieldBufferSizeString = 4096;

      const bool isCodelet = (!argv && setFrame == -2);

      //-----------------------------------------------------------------------
      // Frame setup  (identical logic to legacy CodeBlock::exec)
      //-----------------------------------------------------------------------
      {
         static const U32 TRACE_BUFFER_SIZE = 1024;
         static char traceBuffer[TRACE_BUFFER_SIZE];
         bool doPopFrame = false;

         if (argv)
         {
            U32 fnArgc = code[ip + 2 + 6];
            U32 regCount = code[ip + 2 + 7];
            StringTableEntry thisFnName = readSTE(code, ip);
            S32 wantedArgc = getMin((S32)argc - 1, (S32)fnArgc);

            if (Con::gTraceOn)
            {
               traceBuffer[0] = 0;
               dStrcat(traceBuffer, "Entering ", TRACE_BUFFER_SIZE);
               if (packageName) {
                  dStrcat(traceBuffer, "[", TRACE_BUFFER_SIZE);
                  dStrcat(traceBuffer, packageName, TRACE_BUFFER_SIZE);
                  dStrcat(traceBuffer, "]", TRACE_BUFFER_SIZE);
               }
               if (thisNamespace && thisNamespace->mName)
                  dSprintf(traceBuffer + dStrlen(traceBuffer),
                     TRACE_BUFFER_SIZE - dStrlen(traceBuffer),
                     "%s::%s(", thisNamespace->mName, thisFnName);
               else
                  dSprintf(traceBuffer + dStrlen(traceBuffer),
                     TRACE_BUFFER_SIZE - dStrlen(traceBuffer),
                     "%s(", thisFnName);
               for (S32 i = 0; i < wantedArgc; i++) {
                  dStrcat(traceBuffer, argv[i + 1].getString(), TRACE_BUFFER_SIZE);
                  if (i < wantedArgc - 1)
                     dStrcat(traceBuffer, ", ", TRACE_BUFFER_SIZE);
               }
               dStrcat(traceBuffer, ")", TRACE_BUFFER_SIZE);
               Con::printf("%s", traceBuffer);
            }

            KorkScript::gEvalState.pushFrame(thisFnName, thisNamespace, (S32)regCount);
            doPopFrame = true;

            for (S32 i = 0; i < wantedArgc; i++)
               KorkScript::gEvalState.moveConsoleValue((S32)code[ip + 10 + i], argv[i + 1]);

            // Fill in any missing arguments with their defaults (codelets)
            if (wantedArgc < (S32)fnArgc)
            {
               Namespace::Entry* entry = thisNamespace
                  ? thisNamespace->lookup(thisFnName) : nullptr;
               const U32 flagBase = ip + 10 + fnArgc;
               const U32 offsetBase = ip + 10 + 2 * fnArgc;
               for (U32 i = (U32)wantedArgc; i < fnArgc; i++)
               {
                  S32 reg = (S32)code[ip + 10 + i];
                  U32 argFlags = code[flagBase + i];
                  if (argFlags & 0x1)
                  {
                     U32 codeletIp = entry
                        ? entry->mDefaultOffsets[i]
                        : code[offsetBase + i];
                     if (codeletIp)
                     {
                        Con::EvalResult r = exec(codeletIp, nullptr, nullptr,
                           0, nullptr, false, nullptr, -2);
                        KorkScript::gEvalState.moveConsoleValue(reg, r.value);
                     }
                  }
               }
            }

            ip = ip + 10 + 3 * fnArgc;
            curFloatTable = functionFloats;
            curStringTable = functionStrings;
         }
         else if (isCodelet)
         {
            curFloatTable = functionFloats;
            curStringTable = functionStrings;
            KorkScript::gEvalState.pushFrame(nullptr, nullptr, 0);
            doPopFrame = true;
            setFrame = -1;
         }
         else
         {
            curFloatTable = globalFloats;
            curStringTable = globalStrings;

            if (KorkScript::gEvalState.getStackDepth() <= (U32)setFrame) setFrame = -1;
            if (setFrame <= 0) KorkScript::gEvalState.pushFrame(nullptr, nullptr, (S32)argc);
            else
            {
               S32 idx = KorkScript::gEvalState.getTopOfStack() - setFrame - 1;
               KorkScript::gEvalState.pushFrameRef(idx);
            }
            doPopFrame = true;
         }

         KorkScript::gEvalState.getCurrentFrame().module = this;
         KorkScript::gEvalState.getCurrentFrame().ip = ip;

         const bool telOn = TelDebugger && TelDebugger->isConnected();
         if (telOn && setFrame < 0) TelDebugger->pushStackFrame();
         if (name) { Con::gCurrentFile = name; Con::gCurrentRoot = modPath; }

         //===================================================================
         // Dispatch loop
         //===================================================================
         for (;;)
         {
            // Fetch next opcode and advance instruction pointer
            const U32 instruction = code[ip++];

            // Check breakpoint table before processing this instruction.
            // We do NOT patch the code array — the table is queried here.
            if (BreakpointTable::get().shouldBreak(fullPath, ip - 1))
            {
               KorkScript::gEvalState.getCurrentFrame().module = this;
               KorkScript::gEvalState.getCurrentFrame().ip = ip - 1;
               U32 breakLine, dummy;
               findBreakLine(ip - 1, breakLine, dummy);
               if (breakLine && TelDebugger && TelDebugger->isConnected())
                  TelDebugger->executionStopped(this, breakLine);
            }

         // Re-entry point after a breakpoint so the instruction is
         // processed normally even if no debugger is attached.
         continueDispatch:
            switch (static_cast<Op>(instruction))
            {

            //---------------------------------------------------------------
            // OP_FUNC_DECL
            // Register a function in the namespace system.
            // Layout: fnName(2) ns(2) pkg(2) flags(1) endIp(1)
            //         argc(1) localCount(1) regs[argc] flags[argc] offsets[argc]
            // We read the header, register the function, store the argument
            // flags and default offsets, then jump to endIp to skip the body.
            //---------------------------------------------------------------
            case Op::FuncDecl:
            {
               if (!noCalls)
               {
                  callFnName = readSTE(code, ip);
                  callFnNamespace = readSTE(code, ip + 2);
                  callFnPackage = readSTE(code, ip + 4);
                  bool hasBody = (code[ip + 6] & 0x01) != 0;

                  Namespace::unlinkPackages();
                  ns = (callFnNamespace == nullptr && callFnPackage == nullptr)
                     ? Namespace::global()
                     : Namespace::find(callFnNamespace, callFnPackage);

                  // Register with offset pointing to the function header start
                  ns->addFunction(callFnName, this, hasBody ? ip : 0);

                  // Attach doc block to the namespace if one was pending
                  if (curNSDocBlock)
                  {
                     if (callFnNamespace == StringTable->lookup(nsDocBlockClass))
                     {
                        char* usage = dStrdup(curNSDocBlock);
                        ns->mUsage = usage;
                        ns->mCleanUpUsage = true;
                        curNSDocBlock = nullptr;
                     }
                  }

                  // Record per-argument flags and default codelet IPs so
                  // exec() can fill in missing arguments at call time
                  const U32 fnArgc = code[ip + 8];
                  const U32 flagBase = ip + 10 + fnArgc;
                  const U32 offsetBase = ip + 10 + 2 * fnArgc;

                  Namespace::Entry* entry = ns->lookup(callFnName);
                  if (entry)
                  {
                     entry->mArgFlags.setSize(fnArgc);
                     entry->mDefaultOffsets.setSize(fnArgc);
                     for (U32 fa = 0; fa < fnArgc; ++fa)
                     {
                        entry->mArgFlags[fa] = code[flagBase + fa];
                        entry->mDefaultOffsets[fa] = code[offsetBase + fa];
                     }
                  }

                  Namespace::relinkPackages();
                  curFNDocBlock = nullptr;
               }

               // Jump past the entire function (header + body + codelets)
               // endIp is at code[ip + 7]
               ip = code[ip + 7];
               break;
            }

            // ---------------------------------------------------------------
            // OP_DEFAULT_END
            // Return from a default-value codelet.
            // The codelet's result is on top of the stack; pop it into
            // returnValue and exit this exec() call.
            // ---------------------------------------------------------------
            case Op::DefaultEnd:
            {
               returnValue = stack[stackTop];
               stackTop--;
               // Clean up any foreach loops that may have been open
               while (iterDepth > 0)
               {
                  iterStack[--iterTop].mIsStringIter = false;
                  iterDepth--;
                  stackTop--;
               }
               goto execFinished;
            }

            // ---------------------------------------------------------------
            // OP_CREATE_OBJECT
            // Begin creating a new SimObject.
            // Reads: parent(2) isDataBlock(1) isInternal(1) isSingleton(1)
            //        lineNumber(1) failJump(1)
            // Arguments are taken from gCallStack (set up by preceding
            // OP_PUSH_FRAME / OP_PUSH instructions):
            //   callArgv[1] = class name
            //   callArgv[2] = object name
            //   callArgv[3..] = constructor arguments
            // ---------------------------------------------------------------
            case Op::CreateObject:
            {
               objParent = readSTE(code, ip);
               const bool isDataBlock = code[ip + 2] != 0;
               const bool isInternal = code[ip + 3] != 0;
               const bool isSingleton = code[ip + 4] != 0;
               const U32  lineNumber = code[ip + 5];
               failJump = code[ip + 6];

               // noCalls means analysis mode — skip object creation entirely
               if (noCalls) { ip = failJump; break; }

               // Save outer object state onto the creation stack
               AssertFatal(objCreationDepth < (S32)kObjStackSize,
                  "KorkScript::CodeBlock2 — object creation stack overflow");
               objCreationStack[objCreationDepth].obj = currentNewObject;
               objCreationStack[objCreationDepth].failJump = failJump;
               objCreationDepth++;

               gCallStack.argvc(nullptr, callArgc, &callArgv);
               AssertFatal(callArgc >= 3, "OP_CREATE_OBJECT needs at least 3 args");

               const char* className = callArgv[1].getString();
               const char* objectName = callArgv[2].getString();

               currentNewObject = nullptr;

               // ----- Datablock re-declaration -----
               if (isDataBlock)
               {
                  SimObject* existing = Sim::getDataBlockGroup()->findObject(objectName);
                  if (existing && dStricmp(existing->getClassName(), className))
                  {
                     Con::errorf(ConsoleLogEntry::General,
                        "Cannot re-declare datablock %s with a different class.",
                        objectName);
                     ip = failJump;
                     gCallStack.popFrame();
                     break;
                  }
                  if (existing)
                     currentNewObject = existing;
               }
               // ----- Singleton re-use or class-name collision check -----
               else if (!isInternal)
               {
                  AbstractClassRep* rep = AbstractClassRep::findClassRep(objectName);
                  if (rep)
                  {
                     Con::errorf(ConsoleLogEntry::General,
                        "%s: Cannot name object [%s] the same as a script class.",
                        getFileLine(ip), objectName);
                     ip = failJump;
                     gCallStack.popFrame();
                     break;
                  }

                  SimObject* existing = Sim::findObject(objectName);
                  if (existing)
                  {
                     if (isSingleton)
                     {
                        // Singleton: reuse the existing object if same class
                        if (dStricmp(existing->getClassName(), className) != 0)
                        {
                           Con::errorf(ConsoleLogEntry::General,
                              "%s: Cannot re-declare singleton [%s] with class [%s], was [%s].",
                              getFileLine(ip), objectName, className, existing->getClassName());
                           ip = failJump;
                           gCallStack.popFrame();
                           break;
                        }
                        currentNewObject = existing;
                     }
                  }
               }

               gCallStack.popFrame();

               // ----- Create new object if not reusing existing -----
               if (!currentNewObject)
               {
                  ConsoleObject* newObj = ConsoleObject::create(className);
                  if (!newObj)
                  {
                     Con::errorf(ConsoleLogEntry::General,
                        "%s: Unable to instantiate class %s.",
                        getFileLine(ip - 1), className);
                     ip = failJump;
                     break;
                  }

                  if (isDataBlock)
                  {
                     SimDataBlock* db = dynamic_cast<SimDataBlock*>(newObj);
                     if (!db)
                     {
                        Con::errorf(ConsoleLogEntry::General,
                           "%s: %s is not a datablock class.",
                           getFileLine(ip - 1), className);
                        delete newObj;
                        ip = failJump;
                        break;
                     }
                     db->assignId();
                  }

                  currentNewObject = dynamic_cast<SimObject*>(newObj);
                  if (!currentNewObject)
                  {
                     Con::errorf(ConsoleLogEntry::General,
                        "%s: %s does not derive from SimObject.",
                        getFileLine(ip - 1), className);
                     delete newObj;
                     ip = failJump;
                     break;
                  }

                  currentNewObject->setDeclarationLine(lineNumber);
                  currentNewObject->setFilename(name);

                  // Copy-construct from parent if specified
                  if (objParent && *objParent)
                  {
                     SimObject* parent;
                     if (Sim::findObject(objParent, parent))
                     {
                        currentNewObject->setCopySource(parent);
                        currentNewObject->assignFieldsFrom(parent);
                     }
                     else
                     {
                        Con::errorf(ConsoleLogEntry::General,
                           "%s: Parent object %s not found.",
                           getFileLine(ip - 1), objParent);
                        delete currentNewObject;
                        currentNewObject = nullptr;
                        ip = failJump;
                        break;
                     }
                  }

                  // Name the object
                  if (objectName[0])
                  {
                     if (!isInternal) currentNewObject->assignName(objectName);
                     else            currentNewObject->setInternalName(objectName);
                     currentNewObject->setOriginalName(objectName);
                  }

                  // Run constructor arguments
                  if (!currentNewObject->processArguments(callArgc - 3, callArgv + 3))
                  {
                     delete currentNewObject;
                     currentNewObject = nullptr;
                     ip = failJump;
                     break;
                  }

                  if (!isDataBlock)
                  {
                     currentNewObject->setModStaticFields(true);
                     currentNewObject->setModDynamicFields(true);
                  }
               }
               else
               {
                  // Reloading / singleton — reset and re-copy parent if needed
                  currentNewObject->reloadReset();
                  if (*objParent)
                  {
                     SimObject* parent;
                     if (Sim::findObject(objParent, parent))
                     {
                        currentNewObject->setCopySource(parent);
                        currentNewObject->assignFieldsFrom(parent);
                     }
                  }
               }

               // Advance past the 7 header words
               ip += 7;
               break;
            }

            // ---------------------------------------------------------------
            // OP_ADD_OBJECT
            // Register currentNewObject with the Sim system and add to group.
            // Reads: placeAtRoot(1)
            // Pushes the new object's ID onto the value stack.
            // ---------------------------------------------------------------
            case Op::AddObject:
            {
               const bool placeAtRoot = code[ip++] != 0;

               // Clear doc block so it doesn't bleed into next function decl
               curFNDocBlock = nullptr;
               curNSDocBlock = nullptr;

               if (!currentNewObject) break;

               const bool isMessage = dynamic_cast<Message*>(currentNewObject) != nullptr;

               if (!currentNewObject->isProperlyAdded())
               {
                  bool registered;
                  if (isMessage)
                  {
                     SimObjectId msgId = Message::getNextMessageID();
                     registered = (msgId != 0xffffffff)
                        ? currentNewObject->registerObject(msgId)
                        : false;
                  }
                  else
                     registered = currentNewObject->registerObject();

                  if (!registered)
                  {
                     Con::warnf(ConsoleLogEntry::General,
                        "%s: registerObject failed for %s::%s.",
                        getFileLine(ip - 2),
                        currentNewObject->getClassName(),
                        currentNewObject->getName());
                     delete currentNewObject;
                     currentNewObject = nullptr;
                     ip = failJump;
                     break;
                  }
               }

               // Preload datablocks
               SimDataBlock* dataBlock = dynamic_cast<SimDataBlock*>(currentNewObject);
               if (dataBlock)
               {
                  String err;
                  if (!dataBlock->preload(true, err))
                  {
                     Con::errorf(ConsoleLogEntry::General,
                        "%s: preload failed for %s: %s",
                        getFileLine(ip - 2), currentNewObject->getName(), err.c_str());
                     dataBlock->deleteObject();
                     currentNewObject = nullptr;
                     ip = failJump;
                     break;
                  }
               }

               // Add to the appropriate group
               if (!placeAtRoot || !currentNewObject->getGroup())
               {
                  if (!isMessage)
                  {
                     SimGroup* grp = nullptr;
                     SimSet* set = nullptr;

                     if (!placeAtRoot)
                     {
                        U32 groupId = (U32)stack[stackTop].getInt();
                        if (!Sim::findObject(groupId, grp))
                           Sim::findObject(groupId, set);
                     }
                     else
                     {
                        if (Con::gInstantGroup.isEmpty() ||
                           !Sim::findObject(Con::gInstantGroup, grp))
                           grp = Sim::getRootGroup();
                     }

                     if (!grp) grp = Sim::getRootGroup();
                     grp->addObject(currentNewObject);

                     if (!currentNewObject->getGroup())
                        Sim::getRootGroup()->addObject(currentNewObject);

                     if (set) set->addObject(currentNewObject);
                  }
               }

               // Push the new object's ID
               S32 newId = currentNewObject->getId();
               if (placeAtRoot)
                  stack[stackTop].setInt(newId);       // overwrite group ID
               else
               {
                  stackTop++;
                  stack[stackTop].setInt(newId);        // push new entry
               }
               break;
            }

            // ---------------------------------------------------------------
            // OP_END_OBJECT
            // Reads: placeAtRoot(1)
            // If not placing at root, pop the parent group ID off the stack.
            // ---------------------------------------------------------------
            case Op::EndObject:
            {
               const bool placeAtRoot = code[ip++] != 0;
               if (!placeAtRoot) stackTop--;
               break;
            }

            // ---------------------------------------------------------------
            // OP_FINISH_OBJECT
            // Call onPostAdd() on the finished object and restore the outer
            // object creation context from the creation stack.
            // ---------------------------------------------------------------
            case Op::FinishObject:
            {
               if (currentNewObject) currentNewObject->onPostAdd();
               AssertFatal(objCreationDepth > 0, "OP_FINISH_OBJECT: creation stack empty");
               objCreationDepth--;
               currentNewObject = objCreationStack[objCreationDepth].obj;
               failJump = objCreationStack[objCreationDepth].failJump;
               break;
            }

            // ---------------------------------------------------------------
            // Jump instructions
            // All jumps read the destination IP from code[ip].
            // "Not" variants jump when the condition is FALSE (skip when TRUE).
            // NP = "no pop" — peek at stack without consuming (for short-circuit).
            // ---------------------------------------------------------------
            case Op::JmpIffNot:   // jump if float on stack is falsy (== 0.0)
               if (stack[stackTop--].getFloat()) { ip++; break; }
               ip = code[ip];
               break;
            case Op::JmpIfNot:    // jump if int on stack is falsy (== 0)
               if (stack[stackTop--].getInt()) { ip++; break; }
               ip = code[ip];
               break;
            case Op::JmpNotString: // jump if string on stack is empty/false
               if (stack[stackTop--].getBool()) { ip++; break; }
               ip = code[ip];
               break;
            case Op::JmpIff:      // jump if float on stack is truthy (!= 0.0)
               if (!stack[stackTop--].getFloat()) { ip++; break; }
               ip = code[ip];
               break;
            case Op::JmpIf:       // jump if int on stack is truthy (!= 0)
               if (!stack[stackTop--].getInt()) { ip++; break; }
               ip = code[ip];
               break;
            case Op::JmpIfNotNP:  // short-circuit AND: peek, jump if false
               if (stack[stackTop].getInt()) { stackTop--; ip++; break; }
               ip = code[ip];
               break;
            case Op::JmpIfNP:     // short-circuit OR: peek, jump if true
               if (!stack[stackTop].getInt()) { stackTop--; ip++; break; }
               ip = code[ip];
               break;
            case Op::Jmp:         // unconditional jump
               ip = code[ip];
               break;

               // ---------------------------------------------------------------
               // Return instructions
               // Each cleans up any open foreach loops before exiting.
               // ---------------------------------------------------------------
            case Op::ReturnVoid:
            {
               while (iterDepth > 0)
               {
                  iterStack[--iterTop].mIsStringIter = false;
                  iterDepth--;
                  stackTop--;
               }
               returnValue.setEmptyString();
               goto execFinished;
            }
            case Op::Return:
            {
               returnValue = stack[stackTop--];
               while (iterDepth > 0)
               {
                  iterStack[--iterTop].mIsStringIter = false;
                  iterDepth--;
                  stackTop--;
               }
               goto execFinished;
            }
            case Op::ReturnFlt:
               returnValue.setFloat(stack[stackTop--].getFloat());
               while (iterDepth > 0)
               {
                  iterStack[--iterTop].mIsStringIter = false; iterDepth--; stackTop--;
               }
               goto execFinished;
            case Op::ReturnUInt:
               returnValue.setInt(stack[stackTop--].getInt());
               while (iterDepth > 0)
               {
                  iterStack[--iterTop].mIsStringIter = false; iterDepth--; stackTop--;
               }
               goto execFinished;

               // ---------------------------------------------------------------
               // Comparison — all operate on the top two stack values and leave
               // a U32 (0 or 1) result in stack[stackTop-1], popping one entry.
               // The RIGHT operand is stack[stackTop], LEFT is stack[stackTop-1].
               // ---------------------------------------------------------------
            case Op::CmpEq:
               stack[stackTop - 1].setInt(stack[stackTop].getFloat() == stack[stackTop - 1].getFloat());
               stackTop--; break;
            case Op::CmpNe:
               stack[stackTop - 1].setInt(stack[stackTop].getFloat() != stack[stackTop - 1].getFloat());
               stackTop--; break;
            case Op::CmpGr:
               stack[stackTop - 1].setInt(stack[stackTop].getFloat() > stack[stackTop - 1].getFloat());
               stackTop--; break;
            case Op::CmpGe:
               stack[stackTop - 1].setInt(stack[stackTop].getFloat() >= stack[stackTop - 1].getFloat());
               stackTop--; break;
            case Op::CmpLt:
               stack[stackTop - 1].setInt(stack[stackTop].getFloat() < stack[stackTop - 1].getFloat());
               stackTop--; break;
            case Op::CmpLe:
               stack[stackTop - 1].setInt(stack[stackTop].getFloat() <= stack[stackTop - 1].getFloat());
               stackTop--; break;

               // ---------------------------------------------------------------
               // Bitwise / integer binary operations
               // ---------------------------------------------------------------
            case Op::Xor:
               stack[stackTop - 1].setInt(stack[stackTop].getInt() ^ stack[stackTop - 1].getInt());
               stackTop--; break;
            case Op::BitAnd:
               stack[stackTop - 1].setInt(stack[stackTop].getInt() & stack[stackTop - 1].getInt());
               stackTop--; break;
            case Op::BitOr:
               stack[stackTop - 1].setInt(stack[stackTop].getInt() | stack[stackTop - 1].getInt());
               stackTop--; break;
            case Op::And:
               stack[stackTop - 1].setInt(stack[stackTop].getInt() && stack[stackTop - 1].getInt());
               stackTop--; break;
            case Op::Or:
               stack[stackTop - 1].setInt(stack[stackTop].getInt() || stack[stackTop - 1].getInt());
               stackTop--; break;
            case Op::Shl:
               stack[stackTop - 1].setInt(stack[stackTop - 1].getInt() << stack[stackTop].getInt());
               stackTop--; break;
            case Op::Shr:
               stack[stackTop - 1].setInt(stack[stackTop - 1].getInt() >> stack[stackTop].getInt());
               stackTop--; break;
            case Op::Mod:
            {
               S64 divisor = stack[stackTop - 1].getInt();
               stack[stackTop - 1].setInt(divisor ? stack[stackTop].getInt() % divisor : 0);
               stackTop--; break;
            }

            // ---------------------------------------------------------------
            // Unary integer operations (operate on top of stack in-place)
            // ---------------------------------------------------------------
            case Op::Not:
               stack[stackTop].setBool(!stack[stackTop].getInt());    break;
            case Op::NotF:
               stack[stackTop].setInt(!stack[stackTop].getFloat());   break;
            case Op::OnesComplement:
               stack[stackTop].setInt(~stack[stackTop].getInt());     break;

               // ---------------------------------------------------------------
               // Float arithmetic binary operations
               // ---------------------------------------------------------------
            case Op::Add:
               stack[stackTop - 1].setFloat(stack[stackTop - 1].getFloat() + stack[stackTop].getFloat());
               stackTop--; break;
            case Op::Sub:
               stack[stackTop - 1].setFloat(stack[stackTop - 1].getFloat() - stack[stackTop].getFloat());
               stackTop--; break;
            case Op::Mul:
               stack[stackTop - 1].setFloat(stack[stackTop - 1].getFloat() * stack[stackTop].getFloat());
               stackTop--; break;
            case Op::Div:
               stack[stackTop - 1].setFloat(stack[stackTop - 1].getFloat() / stack[stackTop].getFloat());
               stackTop--; break;
            case Op::Neg:
               stack[stackTop].setFloat(-stack[stackTop].getFloat());  break;

               // ---------------------------------------------------------------
               // OP_INC — increment a local register in-place (no stack usage)
               // Reads: register(1)
               // Used for post-increment expressions on local variables.
               // ---------------------------------------------------------------
            case Op::Inc:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               KorkScript::gEvalState.setLocalFloatVariable(reg,
                  KorkScript::gEvalState.getLocalFloatVariable(reg) + 1.0);
               break;
            }

            // ---------------------------------------------------------------
            // Variable name setup — OP_SETCURVAR / OP_SETCURVAR_CREATE
            // These point gEvalState.currentVariable at the named variable's
            // Dictionary::Entry so that subsequent LOADVAR / SAVEVAR can
            // read/write it without another lookup.
            //
            // After setting a variable, we clear the field/object state
            // (prevField, curObject, etc.) because we're now in "variable
            // mode", not "field mode".  curFNDocBlock is also cleared so
            // doc blocks don't bleed past the assignment.
            // ---------------------------------------------------------------
            case Op::SetCurVar:
            {
               StringTableEntry varName = readSTE(code, ip); ip += 2;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               currentRegister = -1;
               KorkScript::gEvalState.setCurVarName(varName);
               curFNDocBlock = curNSDocBlock = nullptr;
               break;
            }
            case Op::SetCurVarCreate:
            {
               StringTableEntry varName = readSTE(code, ip); ip += 2;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               currentRegister = -1;
               KorkScript::gEvalState.setCurVarNameCreate(varName);
               curFNDocBlock = curNSDocBlock = nullptr;
               break;
            }
            // Array variants: the variable name is built at runtime by
            // concatenating the base name (on the stack) with the index.
            case Op::SetCurVarArray:
            {
               StringTableEntry varName = StringTable->insert(stack[stackTop].getString());
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               currentRegister = -1;
               KorkScript::gEvalState.setCurVarName(varName);
               curFNDocBlock = curNSDocBlock = nullptr;
               break;
            }
            case Op::SetCurVarArrayCreate:
            {
               StringTableEntry varName = StringTable->insert(stack[stackTop].getString());
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               currentRegister = -1;
               KorkScript::gEvalState.setCurVarNameCreate(varName);
               curFNDocBlock = curNSDocBlock = nullptr;
               break;
            }

            //---------------------------------------------------------------
            // Load a variable's value and push it onto the stack.
            //---------------------------------------------------------------
            case Op::LoadVarUInt:
               currentRegister = -1;
               stack[++stackTop].setInt(KorkScript::gEvalState.getIntVariable());
               break;
            case Op::LoadVarFlt:
               currentRegister = -1;
               stack[++stackTop].setFloat(KorkScript::gEvalState.getFloatVariable());
               break;
            case Op::LoadVarStr:
               currentRegister = -1;
               stack[++stackTop].setString(KorkScript::gEvalState.getStringVariable());
               break;

            //---------------------------------------------------------------
            // Save the stack top into the current variable (no pop).
            //---------------------------------------------------------------
            case Op::SaveVarUInt:
               KorkScript::gEvalState.setIntVariable((S32)stack[stackTop].getInt()); break;
            case Op::SaveVarFlt:
               KorkScript::gEvalState.setFloatVariable(stack[stackTop].getFloat());  break;
            case Op::SaveVarStr:
               KorkScript::gEvalState.setStringVariable(stack[stackTop].getString()); break;

            //---------------------------------------------------------------
            // Local register load/save
            // Reads the register index from the next code word, then
            // directly accesses the current call frame's register array.
            //---------------------------------------------------------------
            case Op::LoadLocalVarUInt:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               stack[++stackTop].setInt(KorkScript::gEvalState.getLocalIntVariable(reg));
               break;
            }
            case Op::LoadLocalVarFlt:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               stack[++stackTop].setFloat(KorkScript::gEvalState.getLocalFloatVariable(reg));
               break;
            }
            case Op::LoadLocalVarStr:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               stack[++stackTop].setString(KorkScript::gEvalState.getLocalStringVariable(reg));
               break;
            }
            case Op::SaveLocalVarUInt:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               KorkScript::gEvalState.setLocalIntVariable(reg, stack[stackTop].getInt());
               break;
            }
            case Op::SaveLocalVarFlt:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               KorkScript::gEvalState.setLocalFloatVariable(reg, stack[stackTop].getFloat());
               break;
            }
            case Op::SaveLocalVarStr:
            {
               S32 reg = (S32)code[ip++];
               currentRegister = reg;
               prevField = curField = nullptr;
               prevObject = curObject = nullptr;
               val = stack[stackTop].getString();
               KorkScript::gEvalState.setLocalStringVariable(reg, val, (S32)dStrlen(val));
               break;
            }

            //---------------------------------------------------------------
            // Object / field cursor ops
            // These point the VM at a specific object and field so that
            // subsequent LOADFIELD / SAVEFIELD can access it.
            //---------------------------------------------------------------
            case Op::SetCurObject:
            {
               // Save previous for vector-field access
               prevObject = curObject;
               val = stack[stackTop].getString();
               // Reject multi-word strings — they are never valid object IDs
               const char* check = val;
               while (*check && *check != ' ') check++;
               curObject = (*check == ' ') ? nullptr : Sim::findObject(val);
               break;
            }
            case Op::SetCurObjectNew:
               curObject = currentNewObject;
               break;
            case Op::SetCurObjectInternal:
            {
               // --> operator: find an object by its internal name within a set
               bool recurse = code[ip++] != 0;
               if (curObject)
               {
                  SimSet* set = dynamic_cast<SimSet*>(curObject);
                  if (set)
                  {
                     StringTableEntry intName = StringTable->insert(stack[stackTop].getString());
                     SimObject* found = set->findObjectByInternalName(intName, recurse);
                     stack[stackTop].setInt(found ? found->getId() : 0);
                  }
                  else
                  {
                     Con::errorf(ConsoleLogEntry::Script,
                        "%s: -> used on non-set %s", getFileLine(ip - 2), curObject->getName());
                     stack[stackTop].setInt(0);
                  }
               }
               else
               {
                  Con::errorf(ConsoleLogEntry::Script,
                     "%s: -> group object not found.", getFileLine(ip - 2));
                  stack[stackTop].setInt(0);
               }
               break;
            }
            case Op::SetCurField:
            {
               prevField = curField;
               dStrcpy(prevFieldArray, curFieldArray, 256);
               curField = readSTE(code, ip); ip += 2;
               curFieldArray[0] = '\0';
               break;
            }
            case Op::SetCurFieldArray:
               dStrcpy(curFieldArray, stack[stackTop].getString(), 256);
               break;
            case Op::SetCurFieldType:
               if (curObject)
                  curObject->setDataFieldType(code[ip], curField, curFieldArray);
               ip++;
               break;

            //---------------------------------------------------------------
            // Field load — read from curObject.curField and push result.
            // If curObject is null this may be a vector component access.
            //---------------------------------------------------------------
            case Op::LoadFieldUInt:
            {
               if (curObject)
                  stack[++stackTop].setInt(dAtol(curObject->getDataField(curField, curFieldArray)));
               else
               {
                  char buff[FieldBufferSizeNumeric] = {};
                  getFieldComponent(prevObject, prevField, prevFieldArray, curField, buff, currentRegister);
                  stack[++stackTop].setInt(dAtol(buff));
               }
               break;
            }
            case Op::LoadFieldFlt:
            {
               if (curObject)
                  stack[++stackTop].setFloat(dAtod(curObject->getDataField(curField, curFieldArray)));
               else
               {
                  char buff[FieldBufferSizeNumeric] = {};
                  getFieldComponent(prevObject, prevField, prevFieldArray, curField, buff, currentRegister);
                  stack[++stackTop].setFloat(dAtod(buff));
               }
               break;
            }
            case Op::LoadFieldStr:
            {
               if (curObject)
                  stack[++stackTop].setString(curObject->getDataField(curField, curFieldArray));
               else
               {
                  char buff[FieldBufferSizeString] = {};
                  getFieldComponent(prevObject, prevField, prevFieldArray, curField, buff, currentRegister);
                  stack[++stackTop].setString(buff);
               }
               break;
            }

            //---------------------------------------------------------------
            // Field save — write stack top to curObject.curField.
            //---------------------------------------------------------------
            case Op::SaveFieldUInt:
            case Op::SaveFieldFlt:
            case Op::SaveFieldStr:
               if (curObject)
                  curObject->setDataField(curField, curFieldArray, stack[stackTop].getString());
               else
               {
                  setFieldComponent(prevObject, prevField, prevFieldArray, curField, stack[stackTop].getString(), currentRegister);
                  prevObject = nullptr;
               }
               break;

            //---------------------------------------------------------------
            // OP_POP_STK — discard the top value (for void function returns)
            //---------------------------------------------------------------
            case Op::PopStk:
               stackTop--;
               break;

            //---------------------------------------------------------------
            // Immediate value loads — push a constant from the instruction
            // stream or from the string/float tables.
            //---------------------------------------------------------------
            case Op::LoadImmedUInt:
               stack[++stackTop].setInt((S64)code[ip++]);
               break;
            case Op::LoadImmedFlt:
               stack[++stackTop].setFloat(curFloatTable[code[ip++]]);
               break;
            case Op::LoadImmedStr:
               stack[++stackTop].setString(curStringTable + code[ip++]);
               break;
            case Op::LoadImmedIdent:
               stack[++stackTop].setString(readSTE(code, ip));
               ip += 2;
               break;

            //---------------------------------------------------------------
            // OP_TAG_TO_STR — convert a tag string to its registered ID form
            // the first time it is executed, then fall through to LoadImmedStr.
            // Self-modifying: rewrites the opcode to LoadImmedStr after first
            // execution so subsequent calls skip the GameAddTaggedString work.
            //---------------------------------------------------------------
            case Op::TagToStr:
               code[ip - 1] = op(Op::LoadImmedStr);
               if ((U8)curStringTable[code[ip]] != StringTagPrefixByte)
               {
                  U32 tagId = GameAddTaggedString(curStringTable + code[ip]);
                  dSprintf(curStringTable + code[ip] + 1, 7, "%d", tagId);
                  *(curStringTable + code[ip]) = StringTagPrefixByte;
               }
               stack[++stackTop].setString(curStringTable + code[ip++]);
               break;

            //---------------------------------------------------------------
            // OP_DOCBLOCK_STR
            // Sets the pending doc block pointer for the next function decl.
            // If the block contains @class / \class it's a namespace doc.
            //---------------------------------------------------------------
            case Op::DocBlockStr:
            {
               const char* docblock = curStringTable + code[ip++];
               const char* classTag = dStrstr(docblock, "@class");
               if (!classTag) classTag = dStrstr(docblock, "\\class");
               if (classTag)
               {
                  classTag += 7;
                  S32 idx = 0;
                  while (*classTag && *classTag != ' ' && *classTag != '\n'
                     && idx < (S32)sizeof(nsDocBlockClass) - 1)
                     nsDocBlockClass[idx++] = *classTag++;
                  nsDocBlockClass[idx] = '\0';
                  curNSDocBlock = classTag + 1;
               }
               else
                  curFNDocBlock = docblock;
               break;
            }

            //---------------------------------------------------------------
            // OP_CALLFUNC
            // Reads: fnName(2) namespace(2) callType(1)
            //
            // callType maps directly to TS2::CallType enum values:
            //   Function = 0  — plain global function call
            //   Static   = 1  — Namespace::function() static call
            //   Method   = 2  — object.method() — callArgv[1] is the object
            //   Parent   = 3  — Parent::function() — walk up namespace chain
            //
            //---------------------------------------------------------------
            case Op::CallFunc:
            {
               callFnName = readSTE(code, ip);
               callFnNamespace = readSTE(code, ip + 2);
               U32 callType = code[ip + 4];

               // Save current IP for error messages before advancing
               if (!KorkScript::gEvalState.stack.empty())
               {
                  KorkScript::gEvalState.getCurrentFrame().module = this;
                  KorkScript::gEvalState.getCurrentFrame().ip = ip - 1;
               }
               ip += 5;

               gCallStack.argvc(callFnName, callArgc, &callArgv);

               /*Con::printf("CALL: %s type=%u argc=%u",
                  callFnName, callType, callArgc);

               for (U32 i = 0; i < callArgc; i++)
               {
                  Con::printf("  argv[%u] = '%s'",
                     i,
                     callArgv[i].getString());
               }*/

               nsEntry = nullptr;
               thisObject = nullptr;

               switch (static_cast<CallType>(callType))
               {
               case CallType::Function:
                  nsEntry = Namespace::global()->lookup(callFnName);
                  // nsEntry may be null — handled by the single cleanup block below
                  break;

               case CallType::Static:
                  ns = Namespace::find(callFnNamespace);
                  nsEntry = ns ? ns->lookup(callFnName) : nullptr;
                  break;

               case CallType::Method:
               {
                  thisObject = getThisObject(callArgv[1]);
                  if (!thisObject)
                  {
                     Con::warnf(ConsoleLogEntry::General,
                        "%s: Unable to find object '%s' for method '%s'",
                        getFileLine(ip - 6),
                        callArgv[1].getString(), callFnName);
                  }
                  else
                  {
                     ns = thisObject->getNamespace();
                     nsEntry = ns ? ns->lookup(callFnName) : nullptr;
                  }
                  break;
               }
               case CallType::Parent:
                  // Parent::func(%arg) — walk up the namespace chain.
                  //
                  // IMPORTANT: we do NOT unconditionally require callArgv[1]
                  // to be a valid object.  For plain function parent calls,
                  // callArgv[1] is a normal script argument.  Only for method
                  // parent calls (where %this is explicitly passed) will
                  // getThisObject succeed.  A null thisObject here is correct
                  // and harmless for ConsoleFunctionType dispatch.
                  // (See the ParentCall bug fix in the legacy runtime.)
                  if (callArgc > 1)
                     thisObject = getThisObject(callArgv[1]);  // may be null — that's fine

                  if (thisNamespace)
                  {
                     ns = thisNamespace->mParent;
                     nsEntry = ns ? ns->lookup(callFnName) : nullptr;
                  }
                  else
                  {
                     ns = nullptr;
                     nsEntry = nullptr;
                  }
                  break;
               }

               // Couldn't find the function
               if (!nsEntry || noCalls)
               {
                  if (!noCalls)
                  {
                     Con::warnf(ConsoleLogEntry::General,
                        "%s: Unknown command '%s'.", getFileLine(ip - 4), callFnName);
                     if (static_cast<CallType>(callType) == CallType::Method && thisObject)
                     {
                        Con::warnf(ConsoleLogEntry::General,
                           "  Object %s(%d) %s",
                           thisObject->getName() ? thisObject->getName() : "",
                           thisObject->getId(),
                           Con::getNamespaceList(ns));
                     }
                  }
                  gCallStack.popFrame();
                  stack[++stackTop].setEmptyString();
                  break;
               }

               // ---- Script function dispatch ----
               if (nsEntry->mType == Namespace::Entry::ConsoleFunctionType)
               {
                  if (nsEntry->mFunctionOffset)
                  {
                     ConsoleValue ret = nsEntry->mModule->exec(
                        nsEntry->mFunctionOffset, callFnName,
                        nsEntry->mNamespace, callArgc, callArgv,
                        false, nsEntry->mPackage).value;
                     stack[++stackTop] = ret;
                  }
                  else
                     stack[++stackTop].setEmptyString();

                  gCallStack.popFrame();
                  break;
               }

               // ---- Argument count validation ----
               if ((nsEntry->mMinArgs && S32(callArgc) < nsEntry->mMinArgs) ||
                  (nsEntry->mMaxArgs && S32(callArgc) > nsEntry->mMaxArgs))
               {
                  Con::warnf(ConsoleLogEntry::Script,
                     "%s: %s::%s — wrong arg count. got %d, expected %d..%d",
                     getFileLine(ip - 4),
                     ns ? ns->mName : "",
                     callFnName, callArgc,
                     nsEntry->mMinArgs, nsEntry->mMaxArgs);
                  gCallStack.popFrame();
                  stack[++stackTop].setEmptyString();
                  break;
               }

               // ---- Native (C++) callback dispatch ----
               switch (nsEntry->mType)
               {
               case Namespace::Entry::StringCallbackType:
               {
                  const char* r = nsEntry->cb.mStringCallbackFunc(thisObject, callArgc, callArgv);
                  gCallStack.popFrame();
                  stack[++stackTop].setString(r);
                  break;
               }
               case Namespace::Entry::IntCallbackType:
               {
                  S64 r = nsEntry->cb.mIntCallbackFunc(thisObject, callArgc, callArgv);
                  gCallStack.popFrame();
                  if (code[ip] == op(Op::PopStk)) { ip++; break; }
                  stack[++stackTop].setInt(r);
                  break;
               }
               case Namespace::Entry::FloatCallbackType:
               {
                  F64 r = nsEntry->cb.mFloatCallbackFunc(thisObject, callArgc, callArgv);
                  gCallStack.popFrame();
                  if (code[ip] == op(Op::PopStk)) { ip++; break; }
                  stack[++stackTop].setFloat(r);
                  break;
               }
               case Namespace::Entry::VoidCallbackType:
               {
                  nsEntry->cb.mVoidCallbackFunc(thisObject, callArgc, callArgv);
                  gCallStack.popFrame();
                  if (code[ip] == op(Op::PopStk)) { ip++; break; }
                  if (Con::getBoolVariable("$Con::warnVoidAssignment", true))
                     Con::warnf(ConsoleLogEntry::General,
                        "%s: Result of void function '%s' used in expression.",
                        getFileLine(ip - 4), callFnName);
                  stack[++stackTop].setEmptyString();
                  break;
               }
               case Namespace::Entry::BoolCallbackType:
               {
                  bool r = nsEntry->cb.mBoolCallbackFunc(thisObject, callArgc, callArgv);
                  gCallStack.popFrame();
                  if (code[ip] == op(Op::PopStk)) { ip++; break; }
                  stack[++stackTop].setBool(r);
                  break;
               }
               default: break;
               }
               break;
            }

            //---------------------------------------------------------------
            // String concatenation
            // OP_ADVANCE_STR_APPENDCHAR appends one separator char to stack top.
            // OP_REWIND_STR / OP_TERMINATE_REWIND_STR concat top two stack
            // values and pop one, leaving the result at stack[stackTop-1].
            //---------------------------------------------------------------
            case Op::AdvanceStrAppendChar:
            {
               char sep[2] = { (char)code[ip++], '\0' };
               S32 len;
               const char* cat = tsconcat(stack[stackTop].getString(), sep, len);
               stack[stackTop].setString(cat);
               break;
            }
            case Op::RewindStr:
            case Op::TerminateRewindStr:
            {
               S32 len;
               const char* cat = tsconcat(stack[stackTop - 1].getString(),
                  stack[stackTop].getString(), len);
               stack[stackTop - 1].setString(cat);
               stackTop--;
               break;
            }

            //---------------------------------------------------------------
            // OP_COMPARE_STR — case-insensitive string equality test.
            // Pops two strings, pushes 1 if equal, 0 if not.
            //---------------------------------------------------------------
            case Op::CompareStr:
               stack[stackTop - 1].setBool(!dStricmp(stack[stackTop].getString(),
                  stack[stackTop - 1].getString()));
               stackTop--;
               break;

            //---------------------------------------------------------------
            // Argument passing
            // OP_PUSH_FRAME marks the start of a new call frame on gCallStack.
            // OP_PUSH moves a value from the value stack to gCallStack.
            // These work together with OP_CALLFUNC.
            //---------------------------------------------------------------
            case Op::Push:
               gCallStack.push(stack[stackTop--]);
               break;
            case Op::PushFrame:
               gCallStack.pushFrame(code[ip++]);
               break;

            //---------------------------------------------------------------
            // OP_ASSERT — conditional fatal error for script asserts.
            // Reads: msgOffset(1)
            //---------------------------------------------------------------
            case Op::Assert:
            {
               if (!stack[stackTop--].getBool())
               {
                  const char* message = curStringTable + code[ip];
                  U32 breakLine, dummy;
                  findBreakLine(ip - 1, breakLine, dummy);
                  if (PlatformAssert::processAssert(PlatformAssert::Fatal,
                     name ? name : "eval", breakLine, message))
                  {
                     if (TelDebugger && TelDebugger->isConnected() && breakLine > 0)
                        TelDebugger->breakProcess();
                     else
                        Platform::debugBreak();
                  }
               }
               ip++;
               break;
            }

            //---------------------------------------------------------------
            // OP_BREAK — legacy opcode for telnet debugger breakpoints.
            // In TS2 we do NOT patch code with OP_BREAK; we use BreakpointTable
            // instead.  This case handles any OP_BREAK that might appear in
            // bytecode imported from the legacy pipeline.
            //---------------------------------------------------------------
            case Op::Break:
            {
               if (!KorkScript::gEvalState.stack.empty())
               {
                  KorkScript::gEvalState.getCurrentFrame().module = this;
                  KorkScript::gEvalState.getCurrentFrame().ip = ip - 1;
               }
               U32 breakLine, dummy;
               findBreakLine(ip - 1, breakLine, dummy);
               if (breakLine && TelDebugger && TelDebugger->isConnected())
                  TelDebugger->executionStopped(this, breakLine);
               // Re-dispatch the instruction that was at this location
               // (OP_BREAK is always patched over a real opcode in legacy;
               //  since TS2 never patches, this is a no-op path)
               goto continueDispatch;
            }

            //---------------------------------------------------------------
            // foreach / foreach$ iterators
            //
            // OP_ITER_BEGIN / OP_ITER_BEGIN_STR
            // Sets up a new iterator on the iter stack.
            // Layout: isGlobal(1) [varName(2) | reg(1)] failIp(1)
            //   isGlobal=1 → the loop variable is a $global, name follows
            //   isGlobal=0 → the loop variable is a local register, index follows
            //
            // OP_ITER
            // Advances one iteration; jumps to breakIp when exhausted.
            //
            // OP_ITER_END
            // Cleans up the topmost iterator and pops the container value.
            //---------------------------------------------------------------
            case Op::IterBeginStr:
               iterStack[iterTop].mIsStringIter = true;
               // fall through to OP_ITER_BEGIN
               [[fallthrough]];
            case Op::IterBegin:
            {
               bool isGlobal_ = code[ip] != 0;
               U32  failIp = code[ip + (isGlobal_ ? 3 : 2)];

               IterStackRecord& iter = iterStack[iterTop];
               iter.mIsGlobalVariable = isGlobal_;

               if (isGlobal_)
               {
                  StringTableEntry varName = readSTE(code, ip + 1);
                  iter.mVar.mVariable = Con::gGlobalVars.add(varName);
               }
               else
                  iter.mVar.mRegister = (S32)code[ip + 1];

               if (iter.mIsStringIter)
               {
                  // foreach$ — iterate over whitespace-separated tokens
                  iter.mData.mStr.mString = stack[stackTop].getString();
                  iter.mData.mStr.mIndex = 0;
               }
               else
               {
                  // foreach — iterate over a SimSet
                  SimSet* set;
                  if (!Sim::findObject(stack[stackTop].getString(), set))
                  {
                     Con::errorf(ConsoleLogEntry::General,
                        "foreach: no SimSet '%s'. Did you mean foreach$?",
                        stack[stackTop].getString());
                     ip = failIp;
                     stackTop--;
                     continue;
                  }
                  iter.mData.mObj.mSet = set;
                  iter.mData.mObj.mIndex = 0;
               }

               iterTop++;
               iterDepth++;
               ip += isGlobal_ ? 4 : 3;
               break;
            }

            case Op::Iter:
            {
               U32 breakIp = code[ip];
               IterStackRecord& iter = iterStack[iterTop - 1];

               if (iter.mIsStringIter)
               {
                  // Advance through the tab/space-separated string
                  const char* str = iter.mData.mStr.mString;
                  U32 startIndex = iter.mData.mStr.mIndex;
                  U32 endIndex = startIndex;

                  if (!str[startIndex]) { ip = breakIp; continue; }

                  // Find end of this token
                  while (str[endIndex] && !dIsspace(str[endIndex])) endIndex++;

                  if (endIndex > startIndex)
                  {
                     // Temporarily null-terminate — safe because the string
                     // lives on our local value stack so we own the buffer.
                     char saved = str[endIndex];
                     const_cast<char*>(str)[endIndex] = '\0';

                     if (iter.mIsGlobalVariable)
                        iter.mVar.mVariable->setStringValue(&str[startIndex]);
                     else
                        KorkScript::gEvalState.setLocalStringVariable(
                           iter.mVar.mRegister,
                           &str[startIndex],
                           (S32)(endIndex - startIndex));

                     const_cast<char*>(str)[endIndex] = saved;
                  }
                  else
                  {
                     if (iter.mIsGlobalVariable)
                        iter.mVar.mVariable->setStringValue("");
                     else
                        KorkScript::gEvalState.setLocalStringVariable(iter.mVar.mRegister, "", 0);
                  }

                  // Skip separator
                  if (str[endIndex]) endIndex++;
                  iter.mData.mStr.mIndex = endIndex;
               }
               else
               {
                  // Advance through the SimSet
                  U32    index = iter.mData.mObj.mIndex;
                  SimSet* set = iter.mData.mObj.mSet;

                  if (index >= set->size()) { ip = breakIp; continue; }

                  SimObjectId id = set->at(index)->getId();

                  if (iter.mIsGlobalVariable)
                     iter.mVar.mVariable->setIntValue(id);
                  else
                     KorkScript::gEvalState.setLocalIntVariable(iter.mVar.mRegister, id);

                  iter.mData.mObj.mIndex = index + 1;
               }
               ip++;
               break;
            }

            case Op::IterEnd:
            {
               iterTop--;
               iterDepth--;
               stackTop--;  // pop the container value pushed for this foreach
               iterStack[iterTop].mIsStringIter = false;
               break;
            }

            //---------------------------------------------------------------
            // Invalid / unrecognised opcode — fatal in debug, skip in release
            //---------------------------------------------------------------
            case Op::Invalid:
            default:
               AssertISV(false, "TS2::CodeBlock2 — invalid opcode encountered.");
               goto execFinished;

            } // end switch
         } // end for(;;)

      execFinished:

         //-------------------------------------------------------------------
         // Teardown
         //-------------------------------------------------------------------
         if (telOn && setFrame < 0)
            TelDebugger->popStackFrame();

         if (doPopFrame)
            KorkScript::gEvalState.popFrame();
      } // end frame-setup scope

      // Trace "leaving" log
      if (Con::gTraceOn && functionName)
      {
         static const U32 TRACE_BUFFER_SIZE = 1024;
         static char traceBuffer[TRACE_BUFFER_SIZE];
         traceBuffer[0] = 0;
         dStrcat(traceBuffer, "Leaving ", TRACE_BUFFER_SIZE);
         if (packageName) {
            dStrcat(traceBuffer, "[", TRACE_BUFFER_SIZE);
            dStrcat(traceBuffer, packageName, TRACE_BUFFER_SIZE);
            dStrcat(traceBuffer, "]", TRACE_BUFFER_SIZE);
         }
         if (thisNamespace && thisNamespace->mName)
            dSprintf(traceBuffer + dStrlen(traceBuffer),
               TRACE_BUFFER_SIZE - dStrlen(traceBuffer),
               "%s::%s() - return %s", thisNamespace->mName, functionName,
               returnValue.getString());
         else
            dSprintf(traceBuffer + dStrlen(traceBuffer),
               TRACE_BUFFER_SIZE - dStrlen(traceBuffer),
               "%s() - return %s", functionName, returnValue.getString());
         Con::printf("%s", traceBuffer);
      }

      return Con::EvalResult(returnValue);
   }
   const char* KorkCodeBlock::getFunctionArgs(StringTableEntry, U32 offset)
   {
      if (!code || offset >= codeSize) return "";
      U32 fnArgc = code[offset + 8];
      if (!fnArgc) return "";

      static char buf[512];
      buf[0] = '\0';

      StringTableEntry ns = readSTE(code, offset + 2);
      StringTableEntry fn = readSTE(code, offset);

      for (U32 i = 0; i < fnArgc; i++)
      {
         S32 reg = (S32)code[offset + 10 + i];
         StringTableEntry varName = nullptr;
         // Look up via VarRegisterTable
         S32 idx = variableRegisters.lookup(ns, fn, nullptr);
         (void)idx; // TODO: proper lookup once VarRegisterTable API confirmed
         if (varName)
            dStrcat(buf, varName, sizeof(buf));
         else
         {
            char tmp[32];
            dSprintf(tmp, sizeof(tmp), "arg%u", i);
            dStrcat(buf, tmp, sizeof(buf));
         }
         if (i < fnArgc - 1) dStrcat(buf, ", ", sizeof(buf));
      }
      return buf;
   }

   U32 KorkCodeBlock::findFirstBreakLine(U32 lineNumber)
   {
      if (!lineBreakPairs) return 0;
      for (U32 i = 0; i < lineBreakPairCount; i++)
         if (lineBreakPairs[i * 2] >= lineNumber) return lineBreakPairs[i * 2 + 1];
      return 0;
   }

   void KorkCodeBlock::findBreakLine(U32 ip, U32& line, U32& instruction)
   {
      line = instruction = 0;
      if (!lineBreakPairs || !lineBreakPairCount) return;
      S32 lo = 0, hi = (S32)lineBreakPairCount - 1;
      if (lineBreakPairs[0] > ip || lineBreakPairs[hi * 2 + 1] < ip) return;
      while (hi - lo > 1) {
         S32 mid = (lo + hi) / 2;
         if (lineBreakPairs[mid * 2 + 1] <= ip) lo = mid; else hi = mid;
      }
      line = lineBreakPairs[lo * 2];
      instruction = lineBreakPairs[lo * 2 + 1];
   }

   const char* KorkCodeBlock::getFileLine(U32 ip)
   {
      U32 line, inst; findBreakLine(ip, line, inst);
      static char buf[256];
      dSprintf(buf, sizeof(buf), "%s (%u)", name ? name : "<eval>", line);
      return buf;
   }

   bool KorkCodeBlock::setBreakpoint(U32 line)
   {
      Breakpoint bp; bp.kind = Breakpoint::Kind::Line;
      bp.fileName = fullPath; bp.line = line; bp.enabled = true;
      BreakpointTable::get().add(bp); return true;
   }

   void KorkCodeBlock::clearBreakpoint(U32 line) { BreakpointTable::get().remove(fullPath, line); }
   void KorkCodeBlock::clearAllBreaks()
   {
      for (U32 i = 0; i < breakList.size(); i += 2) BreakpointTable::get().remove(fullPath, breakList[i]);
   }
   void KorkCodeBlock::setAllBreaks()
   {
      for (U32 i = 0; i < breakList.size(); i += 2) setBreakpoint(breakList[i]);
   }

   bool KorkCodeBlock::writeDSO(const char* dsoPath) const
   {
      FileStream st;
      if (!st.open(dsoPath, Torque::FS::File::Write)) {
         Con::errorf("KorkScript::CodeBlock2::writeDSO — could not open '%s'", dsoPath);
         return false;
      }
      st.write(kDSOVersion);
      st.write(globalStringsMaxLen);
      if (globalStringsMaxLen)   st.write(globalStringsMaxLen, globalStrings);
      st.write(functionStringsMaxLen);
      if (functionStringsMaxLen) st.write(functionStringsMaxLen, functionStrings);
      st.write((U32)0);  // globalFloatCount  — TODO store as field
      st.write((U32)0);  // functionFloatCount — TODO store as field
      st.write(codeSize);
      if (codeSize) st.write(codeSize * sizeof(U32), code);
      st.write(lineBreakPairCount);
      if (lineBreakPairCount) st.write(lineBreakPairCount * 2 * sizeof(U32), lineBreakPairs);
      st.close();
      return true;
   }

   bool KorkCodeBlock::readDSO(const char* dsoPath)
   {
      FileStream st;
      if (!st.open(dsoPath, Torque::FS::File::Read)) return false;

      U32 version = 0; st.read(&version);
      if (version != kDSOVersion) {
         Con::errorf("KorkScript::KorkCodeBlock::readDSO — version mismatch in '%s'", dsoPath);
         return false;
      }

      st.read(&globalStringsMaxLen);
      if (globalStringsMaxLen) {
         globalStrings = new char[globalStringsMaxLen];
         st.read(globalStringsMaxLen, globalStrings);
      }
      st.read(&functionStringsMaxLen);
      if (functionStringsMaxLen) {
         functionStrings = new char[functionStringsMaxLen];
         st.read(functionStringsMaxLen, functionStrings);
      }

      U32 gfCount = 0; st.read(&gfCount);
      if (gfCount) { globalFloats = new F64[gfCount]; st.read(gfCount * sizeof(F64), globalFloats); }
      U32 ffCount = 0; st.read(&ffCount);
      if (ffCount) { functionFloats = new F64[ffCount]; st.read(ffCount * sizeof(F64), functionFloats); }

      st.read(&codeSize);
      st.read(&lineBreakPairCount);
      U32 total = codeSize + lineBreakPairCount * 2;
      code = new U32[total];
      if (codeSize) st.read(codeSize * sizeof(U32), code);
      lineBreakPairs = lineBreakPairCount ? code + codeSize : nullptr;
      if (lineBreakPairCount) st.read(lineBreakPairCount * 2 * sizeof(U32), lineBreakPairs);

      // TODO: read ident table and apply STE fixups

      st.close();
      calcBreakList();
      BreakpointTable::get().onCodeBlockLoaded(this);
      return true;
   }

   void KorkCodeBlock::release()
   {
      decRefCount();

      if (mRefCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
         delete this;
   }

}
