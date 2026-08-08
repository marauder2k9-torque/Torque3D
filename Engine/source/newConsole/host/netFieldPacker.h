#ifndef _NEWCONSOLE_NETFIELDPACKER_H_
#define _NEWCONSOLE_NETFIELDPACKER_H_

#ifndef _NEWCONSOLE_NETFIELDATTRIBUTE_H_
#include "newConsole/host/netFieldAttribute.h"
#endif
#ifndef _NEWCONSOLE_HOSTBINDING_H_
#include "newConsole/host/hostBinding.h"
#endif

class BitStream;
class NetConnection;

namespace newConsole
{

/// Packs/unpacks every NetFieldAttribute-bearing field on a TypeInfo,
/// reproducing the same wire protocol NetObject::packUpdate/unpackUpdate
/// use by hand today:
///   - one writeFlag/readFlag guard per field, written unconditionally,
///     read back before the value itself, so packer and unpacker stay in
///     lockstep even when a field is skipped
///   - stream->isFull() checked before attempting a field's value, not
///     just before its guard flag - a flag with no value behind it would
///     desync the reader
///   - the returned mask is always a strict subset of the input mask
///     (AssertFatal((retMask & ~mask) == 0) in the ghost pump depends on
///     this; see netGhost.cpp's send loop)
///
/// @note This does not replace NetObject::packUpdate/unpackUpdate - a
///   class calls this from inside its own override, the same way it would
///   hand-write field packs today. Component/child-object packing (the
///   getComponentCount()/getOwnerNetMask() loop) is unrelated to this and
///   stays exactly as it is.
class NetFieldPacker
{
public:
   /// Packs every replicated field in @a type whose dirtyMask intersects
   /// @a mask. @return bits that could not be written this call (either
   /// because the field's mask bit was not set, or the stream filled up
   /// partway through) - always a subset of @a mask.
   static U32 packFields(const ScriptClassRep& type, const ScriptObject* self,
                          NetConnection* conn, U32 mask, BitStream* stream);

   /// Reads back exactly what packFields wrote, in the same field order.
   static void unpackFields(const ScriptClassRep& type, ScriptObject* self,
                             NetConnection* conn, BitStream* stream);

private:
   static bool packOneField(const ScriptFieldRep& field, const ScriptObject* self,
                             BitStream* stream);
   static void unpackOneField(const ScriptFieldRep& field, ScriptObject* self,
                               BitStream* stream);
};

} // namespace newConsole

#endif // !_NEWCONSOLE_NETFIELDPACKER_H_
