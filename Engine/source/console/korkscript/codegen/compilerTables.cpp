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

#include "compilerTables.h"

#include "core/stringTable.h"

namespace KorkScript
{
   //-----------------------------------------------------------------------
   // StringPool
   //-----------------------------------------------------------------------
   void StringPool::reset()
   {
      mMap.clear();
      mStorage.clear();
      mTotalLen = 0;
   }

   U32 StringPool::add(const char* str, bool caseSens, bool tag)
   {
      std::string key = str ? str : "";
      if (!caseSens)
      {
         for (char& c : key) c = (char)dTolower(c);
      }

      if (tag) key += '\x01';

      auto it = mMap.find(key);
      if (it != mMap.end())
         return it->second;

      // new entry.
      mStorage.reserve(mStorage.size() + 1);
      mStorage.push_back({});
      Entry& e = mStorage.back();

      e.str = str ? str : "";
      e.start = mTotalLen;
      e.tag = tag;

      U32 len = (U32)e.str.size() + 1;
      if (tag && len < 7) len = 7;
      e.len = len;

      mTotalLen += len;

      mMap[key] = e.start;

      return e.start;
   }

   U32 StringPool::addInt(U32 value)
   {
      dSprintf(mScratch, sizeof(mScratch), "%d", value);
      return add(mScratch);
   }

   U32 StringPool::addFloat(F64 value)
   {
      dSprintf(mScratch, sizeof(mScratch), "%g", value);
      return add(mScratch);
   }

   char* StringPool::build() const
   {
      if (mTotalLen == 0) return nullptr;

      char* buf = new char[mTotalLen];
      dMemset(buf, 0, mTotalLen);

      for (const Entry& e : mStorage)
      {
         if (e.str.empty()) continue;
         AssertFatal(e.start < mTotalLen, "KorkScript::StringPool::build - entry start offset exceeds total length");
         U32 avail = mTotalLen - e.start;
         AssertFatal(avail >= e.len, "KorkScript::StringPool::build - entry length exceeds available buffer space");
         dStrcpy(buf + e.start, e.str.c_str(), avail);
      }
      return buf;
   }

   void StringPool::write(Stream& st) const
   {
      st.write(mTotalLen);
      for (const Entry* e : mEntries)
         st.write(e->len, e->str.c_str());
   }

   //-----------------------------------------------------------------------
   // FloatPool
   //-----------------------------------------------------------------------
   void FloatPool::reset()
   {
      mIndex.clear();
      mValues.clear();
   }

   U32 FloatPool::add(F64 value)
   {
      U64 k = key(value);
      auto it = mIndex.find(k);
      if (it != mIndex.end())
         return it->second;

      U32 idx = (U32)mValues.size();
      mValues.push_back(value);
      mIndex[k] = idx;
      return idx;
   }

   F64* FloatPool::build() const
   {
      if (mValues.empty()) return nullptr;
      F64* arr = new F64[mValues.size()];
      dMemcpy(arr, mValues.address(), mValues.size() * sizeof(F64));
      return arr;
   }

   void FloatPool::write(Stream& st) const
   {
      st.write((U32)mValues.size());
      for (F64 v : mValues)
         st.write(v);
   }

   //-----------------------------------------------------------------------
   // IdentTable
   //-----------------------------------------------------------------------
   void IdentTable::reset()
   {
      mEntries.clear();
   }

   void IdentTable::add(StringTableEntry ste, U32 ip)
   {
      auto& ipList = mEntries[ste];
      ipList.ste = ste;
      ipList.ips.push_back(ip);
   }

   void IdentTable::applyFixups(const StringPool& pool, U32* code) const
   {
      // For each STE, find its byte offset in the global string pool and
      // write the live StringTableEntry pointer into the code array.
      // (At eval time we write pointers directly; at DSO-load time the loader
      //  does this step from the DSO stream instead.)
      for (const auto& pair : mEntries)
      {
         StringTableEntry ste = pair.first;
         for (U32 ip : pair.second.ips)
         {
            // Write the STE pointer directly into the two-word slot.
#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_ARM64)
            *reinterpret_cast<U64*>(code + ip) = reinterpret_cast<U64>(ste);
#else
            code[ip] = reinterpret_cast<U32>(ste);
#endif
         }
      }
   }

   void IdentTable::write(const StringPool& pool, Stream& st) const
   {
      // Count unique STEs.
      st.write((U32)mEntries.size());
      for (const auto& pair : mEntries)
      {
         // Offset into global string pool.
         const char* str = pair.first;
         U32 offset = const_cast<StringPool&>(pool).add(str, false);
         U32 count = (U32)pair.second.ips.size();

         st.write(offset);
         st.write(count);
         for (U32 ip : pair.second.ips)
            st.write(ip);
      }
   }

   //-----------------------------------------------------------------------
   // VarRegisterTable
   //-----------------------------------------------------------------------
   void VarRegisterTable::reset()
   {
      mTable.clear();
   }

   void VarRegisterTable::add(StringTableEntry ns,
                              StringTableEntry fn,
                              StringTableEntry varName)
   {
      auto& list = mTable[{ns, fn}];
      // Only add if not already present.
      for (auto& e : list)
         if (e == varName) return;
      list.push_back(varName);
   }

   S32 VarRegisterTable::lookup( StringTableEntry ns,
                                 StringTableEntry fn,
                                 StringTableEntry varName) const
   {
      auto it = mTable.find({ ns, fn });
      if (it == mTable.end()) return -1;

      const auto& list = it->second;
      for (S32 i = 0; i < (S32)list.size(); ++i)
         if (list[i] == varName) return i;

      return -1;
   }

   VarRegisterTable VarRegisterTable::copy() const
   {
      VarRegisterTable out;
      out.mTable = mTable;
      return out;
   }

   void VarRegisterTable::write(Stream& st) const
   {
      st.write((U32)mTable.size());
      for (const auto& pair : mTable)
      {
         // Write "namespace::function" as the key string, matching legacy format.
         StringTableEntry key = StringTable->insert(avar("%s::%s", pair.first.ns, pair.first.fn));
         st.writeString(key);

         const auto& vars = pair.second;
         st.write((U32)vars.size());
         for (StringTableEntry v : vars)
            st.writeString(v);
      }
   }

   //-----------------------------------------------------------------------
   // FuncVars
   //-----------------------------------------------------------------------
   void FuncVars::clear()
   {
      mVars.clear();
      mRegToName.clear();
      mCounter = 0;
   }

   S32 FuncVars::assign(StringTableEntry var, TypeReq currentType, bool isConst)
   {
      auto it = mVars.find(var);
      if (it != mVars.end())
      {
         // If type changes between assignments demote to None (dynamic).
         if (it->second.type != currentType && it->second.type != TypeReq::None)
            it->second.type = TypeReq::None;
         return it->second.reg;
      }

      S32 reg = mCounter++;
      mVars[var] = { reg, currentType, var, isConst };
      mRegToName[reg] = var;
      return reg;
   }

   S32 FuncVars::lookup(StringTableEntry var) const
   {
      auto it = mVars.find(var);
      if (it == mVars.end()) return -1;
      return it->second.reg;
   }

   TypeReq FuncVars::lookupType(StringTableEntry var) const
   {
      auto it = mVars.find(var);
      if (it == mVars.end()) return TypeReq::None;
      return it->second.type;
   }

   bool FuncVars::isConstant(StringTableEntry var) const
   {
      auto it = mVars.find(var);
      if (it == mVars.end()) return false;
      return it->second.isConst;
   }

   StringTableEntry FuncVars::nameForRegister(S32 reg) const
   {
      auto it = mRegToName.find(reg);
      if (it == mRegToName.end()) return nullptr;
      return it->second;
   }

   //-----------------------------------------------------------------------
   // CodeStream
   //-----------------------------------------------------------------------
   CodeStream::CodeStream()
   {
      // Pre-allocate a reasonable initial capacity to avoid early reallocs.
      mCode.reserve(4096);
   }

   U32 CodeStream::emit(U32 opcode)
   {
      U32 ip = (U32)mCode.size();
      mCode.push_back(opcode);
      return ip;
   }

   U32 CodeStream::emitSTE(StringTableEntry ste, STEtoCodeFn steToCode)
   {
      U32 ip = (U32)mCode.size();
      // Reserve two words for the 64-bit STE pointer.
      mCode.push_back(0);
      mCode.push_back(0);
      steToCode(ste, ip, mCode.address() + ip);
      return ip;
   }

   void CodeStream::patch(U32 addr, U32 value)
   {
      mPatches.push_back({ addr, value });
   }

   bool CodeStream::inLoop() const
   {
      for (bool b : mFixLoopStack)
         if (b) return true;
      return false;
   }

   U32 CodeStream::emitFix(FixType type)
   {
      U32 ip = (U32)mCode.size();
      mCode.push_back((U32)type); // placeholder
      mFixList.push_back({ ip, type });
      return ip;
   }

   void CodeStream::pushFixScope(bool isLoop)
   {
      mFixStack.push_back((U32)mFixList.size());
      mFixLoopStack.push_back(isLoop);
   }

   void CodeStream::popFixScope()
   {
      AssertFatal(!mFixStack.empty(), "CodeStream::fixLoop - Empty fix stack.");
      U32 oldSize = mFixStack.back();
      mFixStack.pop_back();
      mFixLoopStack.pop_back();
      mFixList.reserve(oldSize);
   }

   void CodeStream::fixLoop(U32 loopBlockStart, U32 breakPoint, U32 continuePoint)
   {
      AssertFatal(!mFixStack.empty(), "CodeStream::fixLoop - Empty fix stack.");
      U32 scopeStart = mFixStack.back();

      for (U32 i = scopeStart; i < (U32)mFixList.size(); ++i)
      {
         const FixEntry& fe = mFixList[i];
         U32 target = 0;
         switch (fe.type)
         {
         case FixType::LoopBlockStart: target = loopBlockStart; break;
         case FixType::Break:          target = breakPoint;     break;
         case FixType::Continue:       target = continuePoint;  break;
         }
         patch(fe.addr, target);
      }
   }

   void CodeStream::addBreakLine(U32 lineNumber, U32 ip)
   {
      mBreakLines.push_back(lineNumber);
      mBreakLines.push_back(ip);
   }

   void CodeStream::finalise(U32* outSize, U32** outCode, U32** outLineBreaks)
   {
      U32 codeWords = (U32)mCode.size();
      U32 breakWords = (U32)mBreakLines.size();
      U32 total = codeWords + breakWords;

      *outCode = new U32[total];
      dMemset(*outCode, 0, total * sizeof(U32));

      // Copy opcode words.
      dMemcpy(*outCode, mCode.address(), codeWords * sizeof(U32));

      // Apply patches.
      for (const PatchEntry& p : mPatches)
         (*outCode)[p.addr] = p.value;

      // Append line break pairs after the code.
      if (breakWords)
         dMemcpy(*outCode + codeWords, mBreakLines.address(), breakWords * sizeof(U32));

      *outSize = codeWords;
      *outLineBreaks = *outCode + codeWords;
   }

   void CodeStream::reset()
   {
      mCode.clear();
      mPatches.clear();
      mFixList.clear();
      mFixStack.clear();
      mFixLoopStack.clear();
      mBreakLines.clear();
   }

   //-----------------------------------------------------------------------
   // STE encoding functions
   //-----------------------------------------------------------------------
   void korkEvalSTEtoCode(StringTableEntry ste, U32 ip, U32* ptr)
   {
#if defined(TORQUE_CPU_X64) || defined(TORQUE_CPU_ARM64)
      * reinterpret_cast<U64*>(ptr) = reinterpret_cast<U64>(ste);
#else
      * ptr = reinterpret_cast<U32>(ste);
#endif
   }

   void korkCompileSTEtoCode(StringTableEntry ste, U32 ip, U32* ptr)
   {
      // Zero out the two-word slot — will be patched during finalise().
      ptr[0] = 0;
      ptr[1] = 0;
   }

}
