#include "platform/platform.h"
#include "componentTestBed/componentSceneObject.h"
// #include "componentTestBed/components/transformComponent.h"
#include "componentTestBed/components/renderComponent.h"
#include "math/mathIO.h"
#include "scene/sceneRenderState.h"
#include "console/consoleTypes.h"
#include "core/stream/bitStream.h"
#include "materials/materialManager.h"
#include "materials/baseMatInstance.h"
#include "renderInstance/renderPassManager.h"
#include "lighting/lightQuery.h"
#include "console/engineAPI.h"

IMPLEMENT_CO_NETOBJECT_V1(ComponentSceneObject);

ComponentSceneObject::ComponentSceneObject()
{
   mNetFlags.set(Ghostable | ScopeAlways);

   // Set it as a "static" object that casts shadows
   mTypeMask |= StaticObjectType | StaticShapeObjectType;
}

ComponentSceneObject::~ComponentSceneObject()
{
}

void ComponentSceneObject::initPersistFields()
{
   docsURL;

   // SceneObject already handles exposing the transform
   Parent::initPersistFields();
}

bool ComponentSceneObject::onAdd()
{
   if (!Parent::onAdd())
      return false;
    
   // Set up a 1x1x1 bounding box
   mObjBox.set(Point3F(-0.5f, -0.5f, -0.5f),
      Point3F(0.5f, 0.5f, 0.5f));

   resetWorldBox();

   // Add this object to the scene
   addToScene();

   return true;
}

void ComponentSceneObject::onRemove()
{
   // Remove this object from the scene
   removeFromScene();

   Parent::onRemove();
}

void ComponentSceneObject::syncFromComponents()
{
   // Same 1x1x1 default the old hardcoded onAdd() used, kept as the
   // fallback for as long as there's no RenderComponent attached yet.
   RenderComponent* render = dynamic_cast<RenderComponent*>(getComponentByClass("RenderComponent"));
   if (render)
      mObjBox = render->getLocalBounds();
   else
      mObjBox.set(Point3F(-0.5f, -0.5f, -0.5f), Point3F(0.5f, 0.5f, 0.5f));

   // TransformComponent* xform = dynamic_cast<TransformComponent*>(getComponentByClass("TransformComponent"));
   // if (xform)
   // {
   //    mObjToWorld = mWorldToObj = xform->getTransform();
   //    mWorldToObj.affineInverse();
   // }

   resetWorldBox();
   setRenderTransform(mObjToWorld);
}

U32 ComponentSceneObject::packUpdate(NetConnection* conn, U32 mask, BitStream* stream)
{
   // Allow the Parent to get a crack at writing its info
   U32 retMask = Parent::packUpdate(conn, mask, stream);

   return retMask;
}

void ComponentSceneObject::unpackUpdate(NetConnection* conn, BitStream* stream)
{
   // Let the Parent read any info it sent
   Parent::unpackUpdate(conn, stream);

   syncFromComponents();
}

void ComponentSceneObject::prepRenderImage(SceneRenderState* state)
{
   RenderComponent* render = dynamic_cast<RenderComponent*>(getComponentByClass("RenderComponent"));
   if (!render)
      return;

   if (!render->isEnabled())
      return;

   ObjectRenderInst* ri = state->getRenderPass()->allocInst<ObjectRenderInst>();
   ri->renderDelegate.bind(render, &RenderComponent::render);
   ri->type = RenderPassManager::RIT_Object;
   ri->defaultKey = 0;
   ri->defaultKey2 = 0;
   state->getRenderPass()->addInst(ri);
}
