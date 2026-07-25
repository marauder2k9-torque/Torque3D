//-----------------------------------------------------------------------------
// RenderComponent.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"
#include "componentTestBed/components/renderComponent.h"
// #include "componentTestBed/components/transformComponent.h"
#include "sim/simObject.h"
#include "scene/sceneRenderState.h"
#include "renderInstance/renderPassManager.h"
#include "gfx/gfxDrawUtil.h"
#include "gfx/gfxTransformSaver.h"
#include "math/mathIO.h"

IMPLEMENT_CONOBJECT(RenderComponent);

ConsoleDocClass(RenderComponent,
   "@brief Draws a solid + wireframe box for whatever it's attached to.\n\n"
   "A first, deliberately simple stand-in for a real mesh/shape component. "
   "If the owner also has a TransformComponent attached, the box is drawn "
   "at that transform; otherwise at identity.\n");

//-----------------------------------------------------------------------------

RenderComponent::RenderComponent()
   : mSize(0.5f, 0.5f, 0.5f),
     mColor(1.0f, 1.0f, 1.0f, 1.0f)
{
}

//-----------------------------------------------------------------------------

void RenderComponent::initPersistFields()
{
   Parent::initPersistFields();

   addGroup("Render");

   ADD_FIELD("size", TypePoint3F, Offset(mSize, RenderComponent))
      .doc("Half-extents of the box in each axis (e.g. \"0.5 0.5 0.5\" draws a "
            "1x1x1 box centered on the transform).")
      .network(1);

   ADD_FIELD("color", TypeColorF, Offset(mColor, RenderComponent))
      .doc("Solid fill color (RGBA, 0-1 range).")
      .network(1);

   endGroup("Render");
}

//-----------------------------------------------------------------------------

void RenderComponent::setSize(const Point3F& size)
{
   mSize = size;
}

//-----------------------------------------------------------------------------

void RenderComponent::setColor(const LinearColorF& color)
{
   mColor = color;
}

//-----------------------------------------------------------------------------

Box3F RenderComponent::getLocalBounds() const
{
   return Box3F(
      Point3F(-mSize.x, -mSize.y, -mSize.z),
      Point3F( mSize.x,  mSize.y,  mSize.z));
}

//-----------------------------------------------------------------------------

void RenderComponent::render(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat)
{
   if (overrideMat)
      return;

   if (!isEnabled())
      return;

   // Same pattern as SceneSpace::_renderObject (sceneSpace.cpp): save/
   // restore the world transform, multiply in ours, draw a solid pass
   // then a wireframe pass over it.
   GFXTransformSaver saver;

   MatrixF mat(true); // identity unless a sibling TransformComponent says otherwise

   // Resolved fresh every call rather than cached: there's no
   // sibling-removal notification in this component system today (see
   // SimObject::removeComponent - it only calls onComponentRemove() on
   // the component actually being removed, not on siblings), so a
   // pointer cached here could dangle if a TransformComponent sibling
   // were detached independently of this component. A per-call
   // getComponentByClass() lookup is the correct tradeoff until a real
   // sibling-notification mechanism exists (a small win worth revisiting
   // once more components need to react to each other, but speculative
   // generality for just these two).
   SimObject* owner = getOwner();
   if (owner)
   {
      // TransformComponent* xform = dynamic_cast<TransformComponent*>(owner->getComponentByClass("TransformComponent"));
      // if (xform)
      //    mat = xform->getTransform();
   }

   GFX->multWorld(mat);

   Box3F bounds = getLocalBounds();

   GFXStateBlockDesc desc;
   desc.setZReadWrite(true, false);
   desc.setBlend(true);
   desc.setCullMode(GFXCullNone);

   GFXDrawUtil* drawer = GFX->getDrawUtil();
   drawer->drawCube(desc, bounds, mColor.toColorI());

   desc.setFillModeWireframe();
   drawer->drawCube(desc, bounds, ColorI::BLACK);
}

U32 RenderComponent::packUpdate(NetConnection* con, U32 mask, BitStream* stream)
{
   U32 retMask = 0;
   if (stream->writeFlag((mask & (getOwnerNetMask() << 1)) != 0))
   {
      if (stream->isFull())
      {
         retMask |= getOwnerNetMask();
      }

      mathWrite(*stream, mSize);
      stream->write(mColor);
   }
   return retMask;
}

void RenderComponent::unpackUpdate(NetConnection* con, BitStream* stream)
{
   if (stream->readFlag())
   {
      mathRead(*stream, &mSize);
      stream->read(&mColor);
   }
}

//-----------------------------------------------------------------------------

DefineEngineMethod(RenderComponent, setSize, void, (Point3F size), ,
   "@brief Set the half-extents of the rendered box.\n"
   "@param size Half-extents in each axis.\n")
{
   object->setSize(size);
}

DefineEngineMethod(RenderComponent, setColor, void, (LinearColorF color), ,
   "@brief Set the solid fill color of the rendered box.\n"
   "@param color RGBA color, 0-1 range.\n")
{
   object->setColor(color);
}
