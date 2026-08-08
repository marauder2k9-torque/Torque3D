#ifndef _NEWCONSOLE_NETFIELDATTRIBUTE_H_
#define _NEWCONSOLE_NETFIELDATTRIBUTE_H_

#ifndef _TORQUE_TYPES_H_
#include "platform/platformTypes.h"
#endif

namespace newConsole
{

   /// How a replicated field's wire representation is chosen.
   ///
   /// @note Every encoding here goes through BitStream::writeInt/readInt with
   ///   an explicit, field-authored bit count - never writeRangedU32/
   ///   readRangedU32/writeRangedF32/readRangedF32, whose bit width is
   ///   derived implicitly inside BitStream itself (getBinLog2(getNextPow2
   ///   (...))). That derivation is exactly the kind of implicit, easy-to-
   ///   get-wrong-silently behavior this attribute exists to replace with a
   ///   number the field author actually chose and can see. RangedInt below
   ///   still takes a range - for clamping and to offset the value into
   ///   [0, 2^bits - 1] before writing - but the bit count itself always
   ///   comes from the field, never derived from the range.
   enum class NetFieldEncoding : U8
   {
      /// Fixed bit width, value written/read as-is (post any sign handling).
      /// Default width is 8 bits - covers bools, small enums, small ranged
      /// ints without the field author having to think about budgets.
      FixedBits,

      /// Integer clamped to [min, max], offset so 0 sits at min, then
      /// written with writeInt(value - min, fixedBits). fixedBits is
      /// authored explicitly, not derived from the range - a range that
      /// doesn't fit in fixedBits is a registration-time error (see
      /// ScriptClassRepBuilder::field), not a silently-widened bit count.
      RangedInt,

      /// Float clamped to [min, max], quantized to fixedBits steps across
      /// that range, written with writeInt. Precision is a function of
      /// range and bit count, not specified separately - an 8-bit field
      /// over [0,1] resolves to 1/255 units, over [-1000,1000] to ~7.8 units.
      QuantizedFloat,

      /// Length-prefixed, variable bit count - the only encoding that does
      /// not have a meaningful fixed budget. Used for String fields and
      /// anything else that cannot be range-bounded.
      VariableLength,
   };

   /// Declarative replication metadata for one field. Attached to a
   /// ScriptFieldRep via SCRIPT_FIELD(...).network(...); carries no
   /// BitStream-specific pack/unpack code itself - it is the contract a
   /// separate pack/unpack code generator reads, not the generator.
   ///
   /// @note Server-authoritative by construction: this describes what goes
   ///   from server to client on a dirty-mask update, never the reverse.
   ///   Fields with no NetFieldAttribute attached do not replicate at all.
   struct NetFieldAttribute
   {
      NetFieldEncoding encoding = NetFieldEncoding::FixedBits;

      /// Which dirty-mask bit(s) trigger sending this field. A field with
      /// dirtyMask == 0 is reflected/inspectable but never sent, even if an
      /// encoding is set - mask is what actually turns replication on.
      U32 dirtyMask = 0;

      /// Bit width actually passed to writeInt/readInt for every encoding
      /// except VariableLength - always author-chosen, never derived from
      /// rangeMin/rangeMax.
      U8 fixedBits = 8;

      /// Bounds for RangedInt / QuantizedFloat. Ignored otherwise. Used for
      /// clamping and offsetting the value before it is written with the
      /// field's own fixedBits - never used to compute a bit count.
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

      /// True if [rangeMin, rangeMax] cannot be represented losslessly in
      /// fixedBits bits - a registration-time check for RangedInt fields,
      /// since fixedBits is author-chosen rather than derived and can be
      /// authored too small by mistake.
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
