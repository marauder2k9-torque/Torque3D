#include "newConsole/host/netFieldPacker.h"

#ifndef _PLATFORMASSERT_H_
#include "platform/platformAssert.h"
#endif
#ifndef _BITSTREAM_H_
#include "core/stream/bitStream.h"
#endif

namespace newConsole
{

   bool NetFieldPacker::packOneField(const ScriptFieldRep& field, const ScriptObject* self, BitStream* stream)
   {
      const NetFieldAttribute& net = field.network;
      ScriptValue value = field.get(self);

      switch (net.encoding)
      {
      case NetFieldEncoding::FixedBits:
      {
         S64 i = 0;
         value.tryGet<S64>(i);
         stream->writeInt(static_cast<S32>(i), net.fixedBits);
         return true;
      }
      case NetFieldEncoding::RangedInt:
      {
         S64 i = 0;
         value.tryGet<S64>(i);
         // Clamp then offset so 0 sits at rangeMin - written with the
         // field's own authored fixedBits via plain writeInt, never
         // writeRangedU32's implicit bit-count derivation.
         if (i < static_cast<S64>(net.rangeMin)) i = static_cast<S64>(net.rangeMin);
         if (i > static_cast<S64>(net.rangeMax)) i = static_cast<S64>(net.rangeMax);
         U32 offset = static_cast<U32>(i - static_cast<S64>(net.rangeMin));
         stream->writeInt(static_cast<S32>(offset), net.fixedBits);
         return true;
      }
      case NetFieldEncoding::QuantizedFloat:
      {
         F64 f = 0.0;
         value.tryGet<F64>(f);
         if (f < net.rangeMin) f = net.rangeMin;
         if (f > net.rangeMax) f = net.rangeMax;

         F64 span = net.rangeMax - net.rangeMin;
         U32 steps = (U32(1) << net.fixedBits) - 1;
         U32 quantized = (span > 0.0)
            ? static_cast<U32>(((f - net.rangeMin) / span) * steps + 0.5)
            : 0;
         stream->writeInt(static_cast<S32>(quantized), net.fixedBits);
         return true;
      }
      case NetFieldEncoding::VariableLength:
      {
         const char* s = "";
         value.tryGet<const char*>(s);
         stream->writeString(s);
         return true;
      }
      default:
         return false;
      }
   }

   void NetFieldPacker::unpackOneField(const ScriptFieldRep& field, ScriptObject* self, BitStream* stream)
   {
      const NetFieldAttribute& net = field.network;

      switch (net.encoding)
      {
      case NetFieldEncoding::FixedBits:
      {
         S32 i = stream->readInt(net.fixedBits);
         field.set(self, ScriptValue::makeInt(i));
         return;
      }
      case NetFieldEncoding::RangedInt:
      {
         // Mirrors packOneField exactly: readInt with the field's own
         // authored fixedBits, then re-add rangeMin to undo the write
         // side's offset. No readRangedU32 - that derives its own bit
         // count from the range internally and would read a different
         // number of bits than writeInt(offset, net.fixedBits) wrote,
         // desyncing every field packed after this one.
         S32 offset = stream->readInt(net.fixedBits);
         S64 i = static_cast<S64>(net.rangeMin) + static_cast<S64>(offset);
         field.set(self, ScriptValue::makeInt(i));
         return;
      }
      case NetFieldEncoding::QuantizedFloat:
      {
         U32 steps = (U32(1) << net.fixedBits) - 1;
         S32 quantized = stream->readInt(net.fixedBits);
         F64 span = net.rangeMax - net.rangeMin;
         F64 f = net.rangeMin + (span > 0.0 ? (F64(quantized) / F64(steps)) * span : 0.0);
         field.set(self, ScriptValue::makeFloat(f));
         return;
      }
      case NetFieldEncoding::VariableLength:
      {
         char buf[256];
         stream->readString(buf);
         field.set(self, ScriptValue::makeString(buf));
         return;
      }
      default:
         return;
      }
   }

   U32 NetFieldPacker::packFields(const ScriptClassRep& type, const ScriptObject* self,
      NetConnection* /*conn*/, U32 mask, BitStream* stream)
   {
      U32 retMask = 0;

      for (const ScriptClassRep* rep = &type; rep != nullptr; rep = rep->getParent())
      {
         const Vector<ScriptFieldRep>& fields = rep->getFields();
         for (U32 i = 0; i < fields.size(); ++i)
         {
            const ScriptFieldRep& field = fields[i];
            if (!field.network.replicates())
               continue;

            U32 fieldMask = field.network.dirtyMask;
            bool relevant = (mask & fieldMask) != 0;

            // Guard flag written unconditionally, exactly as
            // NetObject::packUpdate does for its own relevant/component
            // flags - unpackFields must read this same flag before deciding
            // whether to read a value, so packer and unpacker never drift
            // out of lockstep on a skipped field.
            stream->writeFlag(relevant);
            if (!relevant)
               continue;

            if (stream->isFull())
            {
               // No room even to attempt this field - keep its bit(s) dirty
               // for the next pass rather than write a truncated value.
               retMask |= (mask & fieldMask);
               continue;
            }

            if (!packOneField(field, self, stream))
               retMask |= (mask & fieldMask);
         }
      }

      // Matches the AssertFatal((retMask & (~updateMask)) == 0) invariant the
      // ghost pump relies on (see netGhost.cpp) - retMask must never introduce
      // a bit that was not already in mask.
      AssertFatal((retMask & ~mask) == 0, "NetFieldPacker::packFields - retMask not a subset of mask");
      return retMask;
   }

   void NetFieldPacker::unpackFields(const ScriptClassRep& type, ScriptObject* self,
      NetConnection* /*conn*/, BitStream* stream)
   {
      for (const ScriptClassRep* rep = &type; rep != nullptr; rep = rep->getParent())
      {
         const Vector<ScriptFieldRep>& fields = rep->getFields();
         for (U32 i = 0; i < fields.size(); ++i)
         {
            const ScriptFieldRep& field = fields[i];
            if (!field.network.replicates())
               continue;

            // Read back in exactly the order packFields wrote above - see
            // NetObject::unpackUpdate's own "Read back in exactly the order
            // packUpdate wrote above" comment for why this ordering matters.
            if (stream->readFlag())
               unpackOneField(field, self, stream);
         }
      }
   }

} // namespace newConsole
