#include "newConsole/torquescript2/bytecodeSerialize.h"

#ifndef _STREAM_H_
#include "core/stream/stream.h"
#endif
#ifndef _HASHFUNCTION_H_
#include "core/util/hashFunction.h"
#endif
#ifndef _STRINGFUNCTIONS_H_
#include "core/strings/stringFunctions.h"
#endif

namespace newConsole
{
   namespace ts2
   {

      namespace
      {
         const U8 kMagic[4] = { 'T', 'S', 'C', '2' };
         const U8 kFormatVersion = 1;

         // Sanity ceiling for any length/count prefix read from a .tsc file,
         // applied before the corresponding setSize()/allocation. A
         // corrupt or malicious file can put an arbitrary U32 in a count
         // field; without this, readVector/readInternedString/etc. would
         // attempt a multi-gigabyte allocation on that value alone, before
         // the per-element reads below get a chance to fail on a genuinely
         // truncated stream. No real compiled unit approaches this size.
         const U32 kMaxReadableCount = 16u * 1024u * 1024u; // 16M elements/bytes

         template<typename T>
         bool writePod(Stream& stream, const T& value)
         {
            return stream.write(sizeof(T), &value);
         }

         template<typename T>
         bool readPod(Stream& stream, T& value)
         {
            return stream.read(sizeof(T), &value);
         }

         template<typename T>
         bool writeVector(Stream& stream, const Vector<T>& v)
         {
            U32 count = static_cast<U32>(v.size());
            if (!writePod(stream, count))
               return false;
            for (U32 i = 0; i < count; ++i)
               if (!writePod(stream, v[i]))
                  return false;
            return true;
         }

         template<typename T>
         bool readVector(Stream& stream, Vector<T>& v)
         {
            U32 count = 0;
            if (!readPod(stream, count))
               return false;
            if (count > kMaxReadableCount)
               return false;
            v.setSize(count);
            for (U32 i = 0; i < count; ++i)
               if (!readPod(stream, v[i]))
                  return false;
            return true;
         }

         /// Length-prefixed text - StringTableEntry is a raw pointer, never serialized as-is.
         bool writeInternedString(Stream& stream, StringTableEntry entry)
         {
            bool isNull = (entry == nullptr);
            if (!writePod(stream, isNull))
               return false;
            if (isNull)
               return true;

            U32 len = static_cast<U32>(dStrlen(entry));
            if (!writePod(stream, len))
               return false;
            return len == 0 || stream.write(len, entry);
         }

         bool readInternedString(Stream& stream, StringTableEntry& outEntry)
         {
            bool isNull = false;
            if (!readPod(stream, isNull))
               return false;
            if (isNull)
            {
               outEntry = nullptr;
               return true;
            }

            U32 len = 0;
            if (!readPod(stream, len))
               return false;

            if (len == 0)
            {
               outEntry = StringTable->insert("");
               return true;
            }

            if (len > kMaxReadableCount)
               return false;

            Vector<char> buf;
            buf.setSize(len + 1);
            if (!stream.read(len, buf.address()))
               return false;
            buf[len] = '\0';

            outEntry = StringTable->insert(buf.address());
            return true;
         }

         bool writeVectorOfInternedStrings(Stream& stream, const Vector<StringTableEntry>& v)
         {
            U32 count = static_cast<U32>(v.size());
            if (!writePod(stream, count))
               return false;
            for (U32 i = 0; i < count; ++i)
               if (!writeInternedString(stream, v[i]))
                  return false;
            return true;
         }

         bool readVectorOfInternedStrings(Stream& stream, Vector<StringTableEntry>& v)
         {
            U32 count = 0;
            if (!readPod(stream, count))
               return false;
            if (count > kMaxReadableCount)
               return false;
            v.setSize(count);
            for (U32 i = 0; i < count; ++i)
               if (!readInternedString(stream, v[i]))
                  return false;
            return true;
         }

         bool writeObjectDeclTemplate(Stream& stream, const BytecodeUnit::ObjectDeclTemplate& t)
         {
            return writePod(stream, t.classNameConstIndex)
               && writePod(stream, t.objectNameConstIndex)
               && writePod(stream, t.classNameIsDynamic)
               && writePod(stream, t.objectNameIsDynamic)
               && writePod(stream, t.classNameReg)
               && writePod(stream, t.objectNameReg)
               && writeInternedString(stream, t.parentName)
               && writePod(stream, t.isDatablock)
               && writePod(stream, t.isSingleton)
               && writePod(stream, t.isArrayElement);
         }

         bool readObjectDeclTemplate(Stream& stream, BytecodeUnit::ObjectDeclTemplate& t)
         {
            return readPod(stream, t.classNameConstIndex)
               && readPod(stream, t.objectNameConstIndex)
               && readPod(stream, t.classNameIsDynamic)
               && readPod(stream, t.objectNameIsDynamic)
               && readPod(stream, t.classNameReg)
               && readPod(stream, t.objectNameReg)
               && readInternedString(stream, t.parentName)
               && readPod(stream, t.isDatablock)
               && readPod(stream, t.isSingleton)
               && readPod(stream, t.isArrayElement);
         }

         bool writeLocalDebugInfo(Stream& stream, const BytecodeUnit::LocalDebugInfo& info)
         {
            return writeInternedString(stream, info.name)
               && writePod(stream, info.reg)
               && writePod(stream, info.firstValidInstruction)
               && writePod(stream, info.lastValidInstruction);
         }

         bool readLocalDebugInfo(Stream& stream, BytecodeUnit::LocalDebugInfo& info)
         {
            return readInternedString(stream, info.name)
               && readPod(stream, info.reg)
               && readPod(stream, info.firstValidInstruction)
               && readPod(stream, info.lastValidInstruction);
         }

         bool writeLocalDebugInfoVector(Stream& stream, const Vector<BytecodeUnit::LocalDebugInfo>& v)
         {
            U32 count = static_cast<U32>(v.size());
            if (!writePod(stream, count))
               return false;
            for (U32 i = 0; i < count; ++i)
               if (!writeLocalDebugInfo(stream, v[i]))
                  return false;
            return true;
         }

         bool readLocalDebugInfoVector(Stream& stream, Vector<BytecodeUnit::LocalDebugInfo>& v)
         {
            U32 count = 0;
            if (!readPod(stream, count))
               return false;
            if (count > kMaxReadableCount)
               return false;
            v.setSize(count);
            for (U32 i = 0; i < count; ++i)
               if (!readLocalDebugInfo(stream, v[i]))
                  return false;
            return true;
         }

         bool writeBytecodeUnit(Stream& stream, const BytecodeUnit& unit)
         {
            if (!writeInternedString(stream, unit.name))
               return false;
            if (!writeVector(stream, unit.code))
               return false;
            if (!writeVector(stream, unit.intConsts))
               return false;
            if (!writeVector(stream, unit.floatConsts))
               return false;
            if (!writeVectorOfInternedStrings(stream, unit.stringConsts))
               return false;
            if (!writeVectorOfInternedStrings(stream, unit.taggedStringConsts))
               return false;

            U32 declCount = static_cast<U32>(unit.objectDecls.size());
            if (!writePod(stream, declCount))
               return false;
            for (U32 i = 0; i < declCount; ++i)
               if (!writeObjectDeclTemplate(stream, unit.objectDecls[i]))
                  return false;

            if (!writePod(stream, unit.registerCount))
               return false;
            if (!writePod(stream, unit.paramCount))
               return false;

            // lineTable/origin/localDebugInfo - all empty/null if
            // stripDebugInfo was set (see scriptCompiler.cpp). A .tsc
            // written with debug info stripped carries none of these
            // three by construction, not merely by convention - a
            // stripped file has no source-identifying data to recover.
            if (!writeVector(stream, unit.lineTable))
               return false;
            if (!writeInternedString(stream, unit.origin))
               return false;
            if (!writeLocalDebugInfoVector(stream, unit.localDebugInfo))
               return false;

            return true;
         }

         bool readBytecodeUnit(Stream& stream, BytecodeUnit& unit)
         {
            if (!readInternedString(stream, unit.name))
               return false;
            if (!readVector(stream, unit.code))
               return false;
            if (!readVector(stream, unit.intConsts))
               return false;
            if (!readVector(stream, unit.floatConsts))
               return false;
            if (!readVectorOfInternedStrings(stream, unit.stringConsts))
               return false;
            if (!readVectorOfInternedStrings(stream, unit.taggedStringConsts))
               return false;

            U32 declCount = 0;
            if (!readPod(stream, declCount))
               return false;
            if (declCount > kMaxReadableCount)
               return false;
            unit.objectDecls.setSize(declCount);
            for (U32 i = 0; i < declCount; ++i)
               if (!readObjectDeclTemplate(stream, unit.objectDecls[i]))
                  return false;

            if (!readPod(stream, unit.registerCount))
               return false;
            if (!readPod(stream, unit.paramCount))
               return false;
            if (!readVector(stream, unit.lineTable))
               return false;
            if (!readInternedString(stream, unit.origin))
               return false;
            if (!readLocalDebugInfoVector(stream, unit.localDebugInfo))
               return false;

            return true;
         }

      } // namespace

      U64 hashSource(const char* source, U32 length)
      {
         return Torque::hash64(reinterpret_cast<const U8*>(source), length, 0);
      }

      bool writeCompiledModule(Stream& stream, const CompiledModule& module, String* outError)
      {
         if (!stream.write(sizeof(kMagic), kMagic) || !writePod(stream, kFormatVersion))
         {
            if (outError) *outError = "failed writing .tsc header";
            return false;
         }

         if (!writePod(stream, module.sourceHash))
         {
            if (outError) *outError = "failed writing source hash";
            return false;
         }

         if (module.functionNames.size() != module.functionUnits.size())
         {
            if (outError) *outError = "internal error: functionNames/functionUnits size mismatch";
            return false;
         }

         U32 count = static_cast<U32>(module.functionUnits.size());
         if (!writePod(stream, count))
         {
            if (outError) *outError = "failed writing function count";
            return false;
         }

         for (U32 i = 0; i < count; ++i)
         {
            if (!writeInternedString(stream, module.functionNames[i]) ||
               !writeBytecodeUnit(stream, module.functionUnits[i]))
            {
               if (outError) *outError = "failed writing function data";
               return false;
            }
         }

         return true;
      }

      bool readCompiledModule(Stream& stream, CompiledModule& outModule, String* outError)
      {
         U8 magic[4];
         if (!stream.read(sizeof(magic), magic) || dMemcmp(magic, kMagic, sizeof(kMagic)) != 0)
         {
            if (outError) *outError = "not a .tsc file (bad magic)";
            return false;
         }

         U8 version = 0;
         if (!readPod(stream, version))
         {
            if (outError) *outError = "failed reading .tsc version";
            return false;
         }
         if (version != kFormatVersion)
         {
            if (outError) *outError = "unsupported .tsc format version";
            return false;
         }

         if (!readPod(stream, outModule.sourceHash))
         {
            if (outError) *outError = "failed reading source hash";
            return false;
         }

         U32 count = 0;
         if (!readPod(stream, count))
         {
            if (outError) *outError = "failed reading function count";
            return false;
         }
         if (count > kMaxReadableCount)
         {
            if (outError) *outError = "function count exceeds sanity limit (corrupt file)";
            return false;
         }

         outModule.functionNames.setSize(count);
         outModule.functionUnits.setSize(count);

         for (U32 i = 0; i < count; ++i)
         {
            if (!readInternedString(stream, outModule.functionNames[i]) ||
               !readBytecodeUnit(stream, outModule.functionUnits[i]))
            {
               if (outError) *outError = "failed reading function data (truncated or corrupt file)";
               return false;
            }
         }

         return true;
      }

   } // namespace ts2
} // namespace newConsole
