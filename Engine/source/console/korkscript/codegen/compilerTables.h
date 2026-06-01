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

#ifndef _PLATFORM_H_
#include "platform/platform.h"
#endif
#ifndef _TVECTOR_H_
#include "core/util/tVector.h"
#endif
#ifndef _STREAM_H_
#include "core/stream/stream.h"
#endif
#ifndef _HASHFUNCTION_H_
#include "core/util/hashFunction.h"
#endif
#include <unordered_map>
#include <string>

namespace KorkScript
{
   using STEtoCodeFn = void(*)(StringTableEntry ste, U32 ip, U32* ptr);

   // KorkScript-owned implementations — defined in CompilerTables.cpp.
   void korkEvalSTEtoCode(StringTableEntry ste, U32 ip, U32* ptr);
   void korkCompileSTEtoCode(StringTableEntry ste, U32 ip, U32* ptr);

   class StringPool
   {
   public:
      StringPool() { reset(); }
      // Add a string, returning its byte offset in the final flat buffer.
      // If the string is already present (with same caseSens/tag flags) the
      // existing offset is returned — no duplicate is inserted.
      U32 add(const char* str, bool caseSens = true, bool tag = false);

      // Convenience overloads.
      U32 addInt(U32 value);
      U32 addFloat(F64 value);

      // Build the flat null-separated buffer.  Caller owns the returned array.
      char* build() const;

      // Total byte length of the flat buffer.
      U32 totalLen() const { return mTotalLen; }

      // Write to DSO stream (same wire format as legacy CompilerStringTable).
      void write(Stream& st) const;

      // Clear all entries.  Does not free mArena — call reset() for that.
      void reset();

   private:
      struct Entry
      {
         U32 start;
         U32 len;
         bool tag;
         std::string str;
      };

      std::unordered_map<std::string, U32> mMap;
      Vector<Entry*> mEntries;
      Vector<Entry> mStorage;

      U32 mTotalLen = 0;
      char mScratch[256];
   };

   class FloatPool
   {
   public:
      FloatPool() { reset(); }

      // Add a float, returning its index into the final array.
      // Exact duplicates are deduplicated.
      U32 add(F64 value);

      // Build the flat array.  Caller owns the returned array.
      F64* build() const;

      U32 count() const { return (U32)mValues.size(); }

      // Write to DSO stream.
      void write(Stream& st) const;

      void reset();
   private:
      static U64 key(F64 v) { U64 k; dMemcpy(&k, &v, 8); return k; }

      std::unordered_map<U64, U32>  mIndex;
      Vector<F64>                   mValues;
   };

   class IdentTable
   {
   public:
      // Record that [ste] appears at instruction pointer [ip].
      // Multiple IPs can share the same STE.
      void add(StringTableEntry ste, U32 ip);

      // Apply fixups: for each recorded (ste, ip) pair, write the string pool
      // offset of ste into the code array at [ip].
      // [pool] is the globalStringPool; [code] is the final code array.
      void applyFixups(const StringPool& pool, U32* code) const;

      // Write to DSO stream (same format as legacy CompilerIdentTable).
      void write(const StringPool& pool, Stream& st) const;

      void reset();

   private:
      struct IPList
      {
         StringTableEntry ste;
         Vector<U32> ips;
      };
      // Keyed by StringTableEntry pointer value — STEs are interned so pointer
      // equality is the same as string equality.
      std::unordered_map<StringTableEntry, IPList> mEntries;
   };

   class VarRegisterTable
   {
   public:
      void add(StringTableEntry namespaceName,
               StringTableEntry functionName,
               StringTableEntry varName);

      // Returns -1 if not found.
      S32  lookup(StringTableEntry namespaceName,
                  StringTableEntry functionName,
                  StringTableEntry varName) const;

      VarRegisterTable copy() const;

      void write(Stream& st) const;
      void reset();

   private:
      struct FuncKey
      {
         StringTableEntry ns;
         StringTableEntry fn;
         bool operator==(const FuncKey& o) const { return ns == o.ns && fn == o.fn; }
      };
      struct FuncKeyHash
      {
         size_t operator()(const FuncKey& k) const
         {
            // Combine pointer hashes — STEs are interned so pointers are unique.
            size_t h1 = k.ns
               ? Torque::hash((const U8*)k.ns, dStrlen(k.ns), 0)
               : 0;

            size_t h2 = k.fn
               ? Torque::hash((const U8*)k.fn, dStrlen(k.fn), 0)
               : 0;

            return h1 ^ (h2 << 32u | h2 >> 32u);
         }
      };

      std::unordered_map<FuncKey,
                         Vector<StringTableEntry>,
                         FuncKeyHash>  mTable;
   };

   class FuncVars
   {
   public:
      // Assign a register to [var], or return the existing register if [var]
      // was already assigned.  If the type changes between assignments the
      // variable is demoted to TypeReq::None (dynamic).
      // Returns the register index.
      S32 assign(StringTableEntry var,
         TypeReq          currentType,
         bool             isConstant = false);

      // Return the register index for [var].
      // Returns -1 if [var] has not been assigned (caller should warn).
      S32 lookup(StringTableEntry var) const;

      // Return the type recorded for [var].
      TypeReq lookupType(StringTableEntry var) const;

      // True if [var] was declared as a constant.
      bool isConstant(StringTableEntry var) const;

      // Total number of registers assigned so far.
      S32 count() const { return mCounter; }

      // Name of the variable assigned to register [reg], or null.
      StringTableEntry nameForRegister(S32 reg) const;

      void clear();

   private:
      struct Var
      {
         S32              reg;
         TypeReq          type;
         StringTableEntry name;
         bool             isConst;
      };

      std::unordered_map<StringTableEntry, Var>  mVars;
      std::unordered_map<S32, StringTableEntry>  mRegToName;
      S32 mCounter = 0;
   };

   class CodeStream
   {
   public:
      enum class FixType { LoopBlockStart, Break, Continue };

      CodeStream();

      //-----------------------------------------------------------------------
      // Emission
      //-----------------------------------------------------------------------

      /// Emit one opcode word.  Returns the IP of the emitted word.
      U32 emit(U32 opcode);

      /// Emit a StringTableEntry occupying two words (64-bit pointer).
      /// [steToCode] is the function that writes the STE into the two words —
      /// either evalSTEtoCode (live pointer) or compileSTEtoCode (ident table).
      U32 emitSTE(StringTableEntry ste, STEtoCodeFn steToCode);

      /// Patch [addr] to [value] after the fact.  Applied in finalise().
      void patch(U32 addr, U32 value);

      /// Return current instruction pointer (next word to be emitted).
      U32 tell() const { return (U32)mCode.size(); }

      /// Are we currently inside a loop fix scope?
      bool inLoop() const;

      //-----------------------------------------------------------------------
      // Fix scopes  (break / continue targets)
      //-----------------------------------------------------------------------

      /// Emit a placeholder that will be patched to [type]'s target.
      U32  emitFix(FixType type);

      void pushFixScope(bool isLoop);
      void popFixScope();

      /// Resolve all break/continue/loopstart fixups for the current scope.
      void fixLoop(U32 loopBlockStart, U32 breakPoint, U32 continuePoint);

      //-----------------------------------------------------------------------
      // Line break records  (debugger)
      //-----------------------------------------------------------------------
      void addBreakLine(U32 lineNumber, U32 ip);
      U32  getNumLineBreaks() const { return (U32)mBreakLines.size() / 2; }

      //-----------------------------------------------------------------------
      // Finalise
      //
      // Write the flat code array + line break pairs into caller-owned arrays.
      // [outSize]       — set to the number of opcode words (not bytes)
      // [outCode]       — set to a new U32[] of size (outSize + lineBreaks*2)
      // [outLineBreaks] — set to point into outCode past outSize
      // Caller must delete[] outCode.
      //-----------------------------------------------------------------------
      void finalise(U32* outSize, U32** outCode, U32** outLineBreaks);

      void reset();

   private:
      Vector<U32>  mCode;       // flat opcode buffer

      struct PatchEntry { U32 addr; U32 value; };
      Vector<PatchEntry> mPatches;

      struct FixEntry { U32 addr; FixType type; };
      Vector<FixEntry>   mFixList;
      Vector<U32>        mFixStack;     // indices into mFixList per scope
      Vector<bool>       mFixLoopStack; // is each scope a loop?

      Vector<U32>        mBreakLines;   // [line0, ip0, line1, ip1, ...]
   };
}
