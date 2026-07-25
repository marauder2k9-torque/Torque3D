#ifndef _SCENEOBJECT_H_
#include "scene/sceneObject.h"
#endif
#ifndef _GFXVERTEXBUFFER_H_
#include "gfx/gfxVertexBuffer.h"
#endif
#ifndef _GFXPRIMITIVEBUFFER_H_
#include "gfx/gfxPrimitiveBuffer.h"
#endif

class ComponentSceneObject : public SceneObject
{
   typedef SceneObject Parent;

   // Networking masks, just the next freemask for components.
   enum MaskBits
   {
      ComponentListMask = Parent::NextFreeMask << 0,
      ComponentFieldMaskStart = Parent::NextFreeMask << 1,
   };

public:
   ComponentSceneObject();
   virtual ~ComponentSceneObject();

   DECLARE_CONOBJECT(ComponentSceneObject);

   //--------------------------------------------------------------------------
   // Object Editing
   // Since there is always a server and a client object in Torque and we
   // actually edit the server object we need to implement some basic
   // networking functions
   //--------------------------------------------------------------------------
   // Set up any fields that we want to be editable (like position)
   static void initPersistFields();

   // Handle when we are added to the scene and removed from the scene
   bool onAdd() override;
   void onRemove() override;

   // This function handles sending the relevant data from the server
   // object to the client object
   U32 packUpdate(NetConnection* conn, U32 mask, BitStream* stream) override;
   // This function handles receiving relevant data from the server
   // object and applying it to the client object
   void unpackUpdate(NetConnection* conn, BitStream* stream) override;

   /// Reserves ComponentListMask above - see MaskBits.
   U32 getComponentListMask() const override { return ComponentListMask; }

   /// Reserves the ComponentFieldMaskStart run above - see MaskBits.
   U32 getComponentMaskStart() const override { return ComponentFieldMaskStart; }

   // This is the function that allows this object to submit itself for rendering
   void prepRenderImage(SceneRenderState* state) override;

protected:
   void syncFromComponents();
};
