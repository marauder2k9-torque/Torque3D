#ifndef _SIMCOMPONENT_H_
#define _SIMCOMPONENT_H_

#ifndef _CONSOLEOBJECT_H_
#include "console/consoleObject.h"
#endif

#ifndef _BITSET_H_
#include "core/bitSet.h"
#endif

#ifndef _TAML_CALLBACKS_H_
#include "persistence/taml/tamlCallbacks.h"
#endif

class SimObject;
class NetConnection;
struct Move;
class GameConnection;
class BitStream;

class SimComponent : public SimObject
{
   typedef SimObject Parent;

public:
   SimComponent();
   virtual ~SimComponent();

   bool onAdd() override;
   void onRemove() override;
   static void initPersistFields();

   /// Called once, immediately after this component is added to owner's
   /// list and mOwner has been set. Return false to abort the attach
   /// (owner will roll the attach back and the component is deleted).
   virtual bool onComponentAdd(SimObject* owner);

   SimObject* getOwner() const { return mOwner; }

   /// User-assigned instance tag, e.g. "secondaryShield". May be NULL.
   /// Used to disambiguate multiple instances of the same component
   /// class on one owner; NOT used for field name resolution.
   StringTableEntry getInstanceName() const { return mInstanceName; }
   void setInstanceName(StringTableEntry name) { mInstanceName = name; }

   /// Display name used for the inspector section header. Defaults to
   /// the component's class name; override for a friendlier label.
   virtual StringTableEntry getComponentDisplayName() const { return StringTable->insert(getClassName()); }

   bool isEnabled() const { return mEnabled; }
   virtual void setEnabled(bool enabled) { mEnabled = enabled; }

   /// Called after a static (typed, AbstractClassRep-backed) field on
   /// THIS component is set via setDataField.
   void onStaticModified(const char* slotName, const char* newValue) override {}

   /// Called after a dynamic field on this component is set. Dynamic
   /// fields have no fixed bit mapping and play no part in networking.
   void onDynamicModified(const char* slotName, const char* newValue) override {}

   // Ticking passed down from Owner
   virtual void processTick(const Move* move) {}
   virtual void interpolateMove(F32 delta) {}
   virtual void advanceMove(F32 dt) {}

   /// Only called by the owner if (mask & getOwnerNetMask()) is non-zero
   /// (base bit or ANY field bit within this component's range is set) -
   /// see NetObject::packUpdate's component dispatch loop.
   virtual U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream) { return 0; }
   virtual void unpackUpdate(NetConnection* con, BitStream* stream) {}
   virtual void writePacketData(GameConnection* conn, BitStream* stream) {}
   virtual void readPacketData(GameConnection* conn, BitStream* stream) {}

   /// Mirrors SimDataBlock::packData/unpackData.
   virtual void packData(BitStream* stream) {}
   virtual void unpackData(BitStream* stream) {}

   /// How many mask bits this component needs
   virtual U32 getNetworkedFieldCount() const { return 1; }

   U32 getOwnerNetMask() const { return mOwnerNetMask; }
   void bindOwnerNetMask(U32 bits) { mOwnerNetMask = bits; }

   DECLARE_CONOBJECT(SimComponent);

protected:
   SimObject* mOwner;
   StringTableEntry  mInstanceName;
   U32               mOwnerNetMask;    // base bit this component is assigned within the owner's real mask
   bool              mEnabled;

   void onTamlPreWrite(void) override {}
   void onTamlPostWrite(void) override {}
   void onTamlPreRead(void) override {}
   void onTamlPostRead(const TamlCustomNodes& customNodes) override {}
   void onTamlAddParent(SimObject* pParentObject) override {}
   void onTamlCustomWrite(TamlCustomNodes& customNodes) override {}
   void onTamlCustomRead(const TamlCustomNodes& customNodes) override {}
};

#endif
