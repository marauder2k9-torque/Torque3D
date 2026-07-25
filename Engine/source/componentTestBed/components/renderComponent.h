#ifndef _RENDERCOMPONENT_H_
#define _RENDERCOMPONENT_H_

#ifndef _SIMCOMPONENT_H_
#include "sim/component/simComponent.h"
#endif

#ifndef _MPOINT3_H_
#include "math/mPoint3.h"
#endif

#ifndef _COLOR_H_
#include "core/color.h"
#endif

class SceneRenderState;
struct ObjectRenderInst;
class BaseMatInstance;

/// Draws a solid + wireframe box for whatever it's attached to.
///
/// This is the thin-SceneObject replacement for "give me something
/// visible in the world" - a first, deliberately simple stand-in for a
/// real mesh/shape component later. The actual draw call
/// (GFXTransformSaver, GFXStateBlockDesc, GFX->getDrawUtil()->drawCube)
/// is the exact pattern already used and proven working in
/// SceneSpace::_renderObject (see sceneSpace.cpp) - same calls, just
/// driven by this component's own mSize/mColor instead of an object's
/// mObjBox/an editor-only material color.
///
/// This component does not itself decide WHEN it gets submitted for
/// rendering - that's still SceneObject::prepRenderImage()'s job (see
/// ComponentSceneObject::prepRenderImage), since submission needs
/// SceneRenderState/RenderPassManager, which only a SceneObject-derived
/// owner has access to via the scene-management machinery. This
/// component just owns the "what does it look like" half: size, color,
/// and the actual draw call once asked to run it.
///
/// If the owner also has a TransformComponent attached, render() uses
/// its transform; otherwise it renders at identity (origin, no
/// rotation). No net mask bits are reserved for size/color - they're
/// set-once-at-authoring-time properties for this first version, not
/// runtime-networked state (see getNetworkedFieldCount() below).
class RenderComponent : public SimComponent
{
   typedef SimComponent Parent;

public:
   RenderComponent();

   /// Half-extents of the box in each axis (so "size" of 1,1,1 draws a
   /// 2x2x2 box centered on the transform - matches Box3F's own
   /// min/max-corner convention, see getLocalBounds() below).
   const Point3F& getSize() const { return mSize; }
   virtual void setSize(const Point3F& size);

   const LinearColorF& getColor() const { return mColor; }
   virtual void setColor(const LinearColorF& color);

   /// Object-space bounding box implied by mSize, centered on the
   /// origin. ComponentSceneObject::syncFromComponents() reads this to
   /// keep the owner's mObjBox (which SceneContainer's binning depends
   /// on) in sync - see that class for why this is a pull rather than a
   /// push.
   Box3F getLocalBounds() const;

   /// Actually issues the draw call. Called back via the
   /// ObjectRenderInst::renderDelegate the owner binds in its own
   /// prepRenderImage() (see ComponentSceneObject::prepRenderImage) -
   /// this component has no prepRenderImage/ObjectRenderInst-allocation
   /// of its own, since that requires a SceneRenderState-aware owner.
   void render(ObjectRenderInst* ri, SceneRenderState* state, BaseMatInstance* overrideMat);

   U32 packUpdate(NetConnection* con, U32 mask, BitStream* stream) override;
   void unpackUpdate(NetConnection* con, BitStream* stream) override;

   static void initPersistFields();

   DECLARE_CONOBJECT(RenderComponent);

protected:
   Point3F mSize;
   LinearColorF mColor;
};

#endif
