#ifndef _NEWCONSOLE_NETFIELDATTRIBUTE_H_
#define _NEWCONSOLE_NETFIELDATTRIBUTE_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif

namespace newConsole
{

   /// How a replicated field's wire representation is chosen.
   ///
   /// @note Every encoding goes through BitStream::writeInt/readInt with
   ///   an explicit, field-authored bit count - never writeRangedU32/
   ///   readRangedU32/writeRangedF32/readRangedF32, whose width is
   ///   derived implicitly. RangedInt still takes a range for clamping/
   ///   offsetting, but the bit count always comes from the field.
   enum class NetFieldEncoding : U8
   {
      /// Fixed bit width, written/read as-is. Default 8 bits - covers
      /// bools, small enums, small ranged ints with no budget thinking needed.
      FixedBits,

      /// Integer clamped to [min, max], offset so 0 sits at min, written
      /// with writeInt(value - min, fixedBits). fixedBits is authored
      /// explicitly; a range that doesn't fit is a registration-time
      /// error (see ScriptClassRepBuilder::field), not silently widened.
      RangedInt,

      /// Float clamped to [min, max], quantized to fixedBits steps,
      /// written with writeInt. Precision follows from range and bit
      /// count (e.g. 8 bits over [0,1] = 1/255 units).
      QuantizedFloat,

      /// Length-prefixed, variable bit count - for String fields and
      /// anything else with no fixed budget.
      VariableLength,
   };

   /// Declarative replication metadata for one field. Attached via
   /// SCRIPT_FIELD(...).network(...); carries no pack/unpack code
   /// itself - it's the contract a separate generator reads.
   ///
   /// @note Server-authoritative: describes server-to-client on a
   ///   dirty-mask update only. No attribute means no replication.
   struct NetFieldAttribute
   {
      NetFieldEncoding encoding = NetFieldEncoding::FixedBits;

      /// Which dirty-mask bit(s) trigger sending this field.
      ///
      ///  - 0: reflected/inspectable, never sent (ADD_FIELD default).
      ///  - kAlwaysDirty: always dirty, full sync not incremental diff -
      ///    correct default when no bit is picked, and the only correct
      ///    choice for a datablock field (sent whole on connect).
      ///  - any other bit pattern: incremental, sent only when a setter
      ///    marks that bit dirty (see ScriptObject's change hook).
      U32 dirtyMask = 0;

      /// Reserved dirtyMask meaning "always dirty" - distinct from 0
      /// (never sent) and a specific bit (incrementally dirty).
      static constexpr U32 kAlwaysDirty = 0xFFFFFFFFu;

      /// Bit width passed to writeInt/readInt for every encoding except
      /// VariableLength - author-chosen, never derived from range.
      U8 fixedBits = 8;

      /// Bounds for RangedInt/QuantizedFloat, ignored otherwise. Used
      /// for clamping/offsetting, never to compute a bit count.
      F64 rangeMin = 0.0;
      F64 rangeMax = 0.0;

      constexpr NetFieldAttribute() = default;

      static constexpr NetFieldAttribute fixed(U32 mask, U8 bits = 8)
      {
         NetFieldAttribute a;
         a.encoding = NetFieldEncoding::FixedBits;
         a.dirtyMask = mask;
         a.fixedBits = bits;
         return a;
      }

      /// Always-dirty variant of fixed(). Right default for ADD_FIELD
      /// with no explicit mask, and correct for a datablock field.
      static constexpr NetFieldAttribute alwaysDirty(U8 bits = 8)
      {
         return fixed(kAlwaysDirty, bits);
      }

      static constexpr NetFieldAttribute ranged(U32 mask, F64 minValue, F64 maxValue, U8 bits)
      {
         NetFieldAttribute a;
         a.encoding = NetFieldEncoding::RangedInt;
         a.dirtyMask = mask;
         a.rangeMin = minValue;
         a.rangeMax = maxValue;
         a.fixedBits = bits;
         return a;
      }

      static constexpr NetFieldAttribute quantized(U32 mask, F64 minValue, F64 maxValue, U8 bits)
      {
         NetFieldAttribute a;
         a.encoding = NetFieldEncoding::QuantizedFloat;
         a.dirtyMask = mask;
         a.rangeMin = minValue;
         a.rangeMax = maxValue;
         a.fixedBits = bits;
         return a;
      }

      static constexpr NetFieldAttribute variableLength(U32 mask)
      {
         NetFieldAttribute a;
         a.encoding = NetFieldEncoding::VariableLength;
         a.dirtyMask = mask;
         return a;
      }

      bool replicates() const { return dirtyMask != 0; }

      /// True if [rangeMin, rangeMax] can't be represented losslessly
      /// in fixedBits - registration-time check against an
      /// author-chosen bit count that could be too small.
      bool rangeExceedsFixedBits() const
      {
         if (encoding != NetFieldEncoding::RangedInt)
            return false;
         F64 span = rangeMax - rangeMin;
         F64 maxRepresentable = static_cast<F64>((U64(1) << fixedBits) - 1);
         return span > maxRepresentable;
      }
   };

} // namespace newConsole

#endif // !_NEWCONSOLE_NETFIELDATTRIBUTE_H_
