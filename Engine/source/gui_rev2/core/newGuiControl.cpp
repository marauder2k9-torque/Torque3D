//-----------------------------------------------------------------------------
// gui_rev2/core/newGuiControl.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/core/newGuiControl.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/core/newGuiCanvas.h"

IMPLEMENT_CONOBJECT(NewGuiControl);

// See elevateToFront()'s doc comment - starts at kElevationBase and only ever increases, so the
// most recently elevated control always strictly outranks every previously elevated one.
S32 NewGuiControl::smNextElevationLayer = NewGuiControl::kElevationBase;

NewGuiControl::NewGuiControl()
   : mStyleDirty(true),
   mContentDirty(true),
   mArrangementDirty(true),
   mCachedInheritedGeneration(0xFFFFFFFF),   // Guaranteed mismatch - first StylePass always resolves.
   mWidth(NewGuiDimension::fromAuto()),
   mHeight(NewGuiDimension::fromAuto()),
   mLeft(NewGuiDimension::fromAuto()),
   mTop(NewGuiDimension::fromAuto()),
   mRight(NewGuiDimension::fromAuto()),
   mBottom(NewGuiDimension::fromAuto()),
   mPreserveAspect(false),
   mPreferredSize(0, 0),
   mBounds(0, 0, 0, 0),
   mRenderLayer(0),
   mResolvedUIScaleX(1.0f),
   mResolvedUIScaleY(1.0f),
   mRenderLayerOverride(-1),
   mIsTabbable(true),
   mTabIndex(-1),
   mStyle(NULL),
   mTooltipText(NULL),
   mTooltipContent(NULL),
   mTooltipDelayMS(-1),
   mMouseOver(false),
   mMouseActive(false),
   mFirstResponder(false),
   mDisabled(false),
   mChecked(false),
   mHasError(false),
   mVisible(true),
   mActive(true),
   mHitTestable(true)
{
}

NewGuiControl::~NewGuiControl()
{
}

bool NewGuiControl::onAdd()
{
   if (!Parent::onAdd())
      return false;

   mStyleDirty = true;
   mContentDirty = true;
   mArrangementDirty = true;

   return true;
}

void NewGuiControl::onRemove()
{
   Parent::onRemove();
}

void NewGuiControl::addObject(SimObject* object)
{
   Parent::addObject(object);

   // A new child's own subtree has never been through a pass, and it changes this control's own preferred size.
   setStyleDirty();
   setContentDirty();
   setArrangementDirty();
}

void NewGuiControl::removeObject(SimObject* object)
{
   Parent::removeObject(object);

   setContentDirty();
   setArrangementDirty();
}

void NewGuiControl::setStyleDirty()
{
   if (mStyleDirty)
      return;

   mStyleDirty = true;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->setStyleDirty();
   }

   // Also walk up to the root so the driver (which only checks the root's own flag) knows a restyle is needed.
   for (NewGuiControl* ancestor = this; ancestor; ancestor = dynamic_cast<NewGuiControl*>(ancestor->getGroup()))
      ancestor->mStyleDirty = true;
}

void NewGuiControl::setContentDirty()
{
   if (mContentDirty)
      return;

   mContentDirty = true;

   NewGuiControl* parent = dynamic_cast<NewGuiControl*>(getGroup());
   if (parent)
      parent->setContentDirty();
}

void NewGuiControl::setArrangementDirty()
{
   if (mArrangementDirty)
      return;

   mArrangementDirty = true;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->setArrangementDirty();
   }

   // Also walk up to the root so the driver (which only checks the root's own flag) knows a re-arrange is needed.
   for (NewGuiControl* ancestor = this; ancestor; ancestor = dynamic_cast<NewGuiControl*>(ancestor->getGroup()))
      ancestor->mArrangementDirty = true;
}

NewGuiStyleStateMask NewGuiControl::computeStateMask() const
{
   NewGuiStyleStateMask mask = NewGuiState_Normal;

   if (mMouseOver)        mask |= NewGuiState_Hover;
   if (mMouseActive)      mask |= NewGuiState_Active;
   if (mFirstResponder)   mask |= NewGuiState_Focus;
   if (mDisabled)         mask |= NewGuiState_Disabled;
   if (mChecked)          mask |= NewGuiState_Checked;
   if (mHasError)         mask |= NewGuiState_Error;

   return mask;
}

void NewGuiControl::StylePass(const NewGuiResolvedStyle& inherited, U32 inheritedGeneration)
{
   if (mStyleDirty || inheritedGeneration != mCachedInheritedGeneration)
   {
      NewGuiStyleStateMask stateMask = computeStateMask();
      mResolvedStyle = NewGuiStyle::Cascade(inherited, mStyle, stateMask);
      mResolvedStyle.generation = inheritedGeneration + 1;

      mCachedInheritedGeneration = inheritedGeneration;
      mStyleDirty = false;
   }

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->StylePass(mResolvedStyle, mResolvedStyle.generation);
   }
}

Point2I NewGuiControl::MeasurePass()
{
   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->MeasurePass();
   }

   mPreferredSize = ComputePreferredSize();
   mContentDirty = false;
   return mPreferredSize;
}

Point2I NewGuiControl::ComputePreferredSize()
{
   // Base default: union of children's preferred sizes, assuming vertical stacking.
   S32 maxWidth = 0;
   S32 sumHeight = 0;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (!child)
         continue;

      maxWidth = getMax(maxWidth, child->getPreferredSize().x);
      sumHeight += child->getPreferredSize().y;
   }

   // This control's own padding is never part of a child's preferred size
   const NewGuiEdgeInsets& padding = mResolvedStyle.padding;
   maxWidth += (S32)padding.horizontal();
   sumHeight += (S32)padding.vertical();

   return Point2I(maxWidth, sumHeight);
}

S32 NewGuiControl::resolveAxis(const NewGuiDimension& dimension, S32 referenceLength, S32 preferredLength, F32 pixelScale)
{
   switch (dimension.mode)
   {
   case NewGuiDimension::Auto:
      return preferredLength;   // Preferred size is already in device pixels.

   case NewGuiDimension::Percent:
      return S32(F32(referenceLength) * dimension.value / 100.0f);   // referenceLength is already device pixels.

   case NewGuiDimension::Pixels:
   default:
      return S32(dimension.value * pixelScale);   // Authored at design resolution; pixelScale converts to device pixels.
   }
}

void NewGuiControl::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   mResolvedUIScaleX = uiScaleX;
   mResolvedUIScaleY = uiScaleY;

   // mPreserveAspect only affects this control's own extent; position always uses each axis's own scale.
   const F32 extentScaleX = mPreserveAspect ? getMin(uiScaleX, uiScaleY) : uiScaleX;
   const F32 extentScaleY = mPreserveAspect ? getMin(uiScaleX, uiScaleY) : uiScaleY;

   const bool rightDrivesExtent = mWidth.isAuto() && !mRight.isAuto();
   const bool bottomDrivesExtent = mHeight.isAuto() && !mBottom.isAuto();

   const S32 leftInset = !mLeft.isAuto() ? resolveAxis(mLeft, slotRect.extent.x, 0, uiScaleX) : 0;
   const S32 rightInset = !mRight.isAuto() ? resolveAxis(mRight, slotRect.extent.x, 0, uiScaleX) : 0;
   const S32 topInset = !mTop.isAuto() ? resolveAxis(mTop, slotRect.extent.y, 0, uiScaleY) : 0;
   const S32 bottomInset = !mBottom.isAuto() ? resolveAxis(mBottom, slotRect.extent.y, 0, uiScaleY) : 0;

   Point2I resolvedExtent(
      rightDrivesExtent ? getMax(0, slotRect.extent.x - leftInset - rightInset) : resolveAxis(mWidth, slotRect.extent.x, mPreferredSize.x, extentScaleX),
      bottomDrivesExtent ? getMax(0, slotRect.extent.y - topInset - bottomInset) : resolveAxis(mHeight, slotRect.extent.y, mPreferredSize.y, extentScaleY));

   S32 posX = slotRect.point.x;
   S32 posY = slotRect.point.y;

   if (!mLeft.isAuto())
      posX = slotRect.point.x + leftInset;
   else if (!mRight.isAuto())
      posX = slotRect.point.x + slotRect.extent.x - resolvedExtent.x - rightInset;

   if (!mTop.isAuto())
      posY = slotRect.point.y + topInset;
   else if (!mBottom.isAuto())
      posY = slotRect.point.y + slotRect.extent.y - resolvedExtent.y - bottomInset;

   mBounds = RectI(Point2I(posX, posY), resolvedExtent);
   mArrangementDirty = false;

   mRenderLayer = (mRenderLayerOverride >= 0) ? mRenderLayerOverride : (parentRenderLayer + 1);

   RectI clientRect = GetClientRect();
   RectI remainingRect = clientRect;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (!child)
         continue;

      RectI childSlot = GetChildSlot(child, clientRect, remainingRect);
      child->ArrangePass(childSlot, mRenderLayer, uiScaleX, uiScaleY);
      ShrinkRemainingRect(remainingRect, child->getBounds());
   }
}

void NewGuiControl::ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   mResolvedUIScaleX = uiScaleX;
   mResolvedUIScaleY = uiScaleY;

   mBounds = finalBounds;
   mArrangementDirty = false;

   mRenderLayer = (mRenderLayerOverride >= 0) ? mRenderLayerOverride : (parentRenderLayer + 1);

   RectI clientRect = GetClientRect();
   RectI remainingRect = clientRect;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (!child)
         continue;

      RectI childSlot = GetChildSlot(child, clientRect, remainingRect);
      child->ArrangePass(childSlot, mRenderLayer, uiScaleX, uiScaleY);

      ShrinkRemainingRect(remainingRect, child->getBounds());
   }
}

void NewGuiControl::ShrinkRemainingRect(RectI& remainingRect, const RectI& childBounds)
{
   S32 childBottom = childBounds.point.y + childBounds.extent.y;
   S32 newTop = getMax(remainingRect.point.y, childBottom);
   S32 newHeight = getMax(0, (remainingRect.point.y + remainingRect.extent.y) - newTop);

   remainingRect = RectI(Point2I(remainingRect.point.x, newTop), Point2I(remainingRect.extent.x, newHeight));
}

// See the doc comment on NewGuiControl.h - one fresh layer, handed to this control and stamped
// recursively onto every descendant's mRenderLayerOverride, so the whole subtree resolves to
// the SAME override on its next ArrangePass() rather than fanning back out via the ordinary
// parent+1-per-depth cascade once this control's own layer moves.
void NewGuiControl::elevateToFront()
{
   S32 layer = smNextElevationLayer++;
   setRenderLayerOverrideRecursive(layer);
   setArrangementDirty();
}

// Mirrors elevateToFront() - recursively reverts every descendant's override to -1 (the
// ordinary parent.mRenderLayer + 1 default) in one shot.
void NewGuiControl::clearElevation()
{
   setRenderLayerOverrideRecursive(-1);
   setArrangementDirty();
}

// Shared by elevateToFront()/clearElevation() - a raw recursive field write, no dirty stamp of
// its own (both callers stamp once, at the top, after the whole subtree is already set).
void NewGuiControl::setRenderLayerOverrideRecursive(S32 layer)
{
   mRenderLayerOverride = layer;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->setRenderLayerOverrideRecursive(layer);
   }
}

RectI NewGuiControl::GetChildSlot(NewGuiControl* child, const RectI& clientRect, const RectI& remainingRect)
{
   if (!child->getAuthoredTop().isAuto() || !child->getAuthoredBottom().isAuto())
      return clientRect;

   return remainingRect;
}

RectI NewGuiControl::GetClientRect() const
{
   const NewGuiEdgeInsets& padding = mResolvedStyle.padding;

   S32 padLeft = (S32)(padding.left * mResolvedUIScaleX);
   S32 padRight = (S32)(padding.right * mResolvedUIScaleX);
   S32 padTop = (S32)(padding.top * mResolvedUIScaleY);
   S32 padBottom = (S32)(padding.bottom * mResolvedUIScaleY);

   S32 insetX = padLeft + padRight;
   S32 insetY = padTop + padBottom;

   return RectI(
      Point2I(mBounds.point.x + padLeft, mBounds.point.y + padTop),
      Point2I(getMax(0, mBounds.extent.x - insetX), getMax(0, mBounds.extent.y - insetY)));
}

void NewGuiControl::RenderPass(NewGuiRenderBatch* batch, S32 parentLayer)
{
   if (!mVisible)
      return;

   EmitDrawCommands(batch, mBounds, mResolvedStyle, mRenderLayer);

   renderChildControls(batch);
}

void NewGuiControl::renderChildControls(NewGuiRenderBatch* batch)
{
   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->RenderPass(batch, mRenderLayer);
   }
}

// Mirrors RenderPass()/renderChildControls(): self, then children, one subtree before the next
// sibling. mActive is checked (unlike RenderPass(), which only checks mVisible) since a control
// that can't be interacted with shouldn't be reachable by tabbing to it.
void NewGuiControl::collectTabStops(Vector<NewGuiControl*>& outStops)
{
   if (mIsTabbable && mActive && mVisible)
      outStops.push_back(this);

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child)
         child->collectTabStops(outStops);
   }
}

void NewGuiControl::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   if (!batch)
      return;

   if (style.opacity <= 0.0f)
      return;

   // Uniform scale regardless of mPreserveAspect - a border/skin inset shouldn't look thicker on one side.
   const F32 uniformScale = getMin(mResolvedUIScaleX, mResolvedUIScaleY);

   const NewGuiSkinImage* backgroundImage = style.findSkinImage(StringTable->insert("background"));
   if (backgroundImage && backgroundImage->hasImage())
   {
      NewGuiStyleDrawSkinImage(batch, bounds, *backgroundImage, style.opacity, layer);
   }
   else if (style.backgroundColor.alpha > 0)
   {
      ColorI fillColor(
         style.backgroundColor.red,
         style.backgroundColor.green,
         style.backgroundColor.blue,
         (U8)((F32)style.backgroundColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));
      batch->pushQuad(bounds, fillColor, layer);
   }

   const NewGuiSkinImage* borderImage = style.findSkinImage(StringTable->insert("border"));
   if (borderImage && borderImage->hasImage())
   {
      NewGuiStyleDrawSkinImage(batch, bounds, *borderImage, style.opacity, layer);
   }
   else if (style.borderWidth > 0.0f && style.borderColor.alpha > 0 && !backgroundImage)
   {
      ColorI borderColor(
         style.borderColor.red,
         style.borderColor.green,
         style.borderColor.blue,
         (U8)((F32)style.borderColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));

      const F32 scaledBorderWidth = style.borderWidth * uniformScale;

      const Point2I topLeft = bounds.point;
      const Point2I topRight(bounds.point.x + bounds.extent.x, bounds.point.y);
      const Point2I bottomLeft(bounds.point.x, bounds.point.y + bounds.extent.y);
      const Point2I bottomRight(bounds.point.x + bounds.extent.x, bounds.point.y + bounds.extent.y);

      batch->pushLine(topLeft, topRight, borderColor, scaledBorderWidth, layer);
      batch->pushLine(topRight, bottomRight, borderColor, scaledBorderWidth, layer);
      batch->pushLine(bottomRight, bottomLeft, borderColor, scaledBorderWidth, layer);
      batch->pushLine(bottomLeft, topLeft, borderColor, scaledBorderWidth, layer);
   }
}

NewGuiControl* NewGuiControl::findHitControl(const Point2I& point)
{
   if (!mVisible || !mActive)
      return NULL;

   if (!mBounds.pointInRect(point))
      return NULL;

   // Back-to-front (reverse tree order) so the topmost-painted child wins.
   for (S32 i = S32(size()) - 1; i >= 0; --i)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(at(i));
      if (!child)
         continue;

      NewGuiControl* hit = child->findHitControl(point);
      if (hit)
         return hit;
   }

   return mHitTestable ? this : NULL;
}

void NewGuiControl::onInputEvent(NewGuiInputEvent& event)
{
   // Base default: not handled - left for ancestors to handle via dispatch.
}

void NewGuiControl::onMouseEnter(NewGuiInputEvent& event) { setMouseOver(true); }
void NewGuiControl::onMouseLeave(NewGuiInputEvent& event) { setMouseOver(false); }

void NewGuiControl::onMouseDown(NewGuiInputEvent& event)
{
   setMouseActive(true);
   event.handled = true;   // Claims the event by default.
}

void NewGuiControl::onMouseUp(NewGuiInputEvent& event)
{
   setMouseActive(false);
   event.handled = true;
}

void NewGuiControl::onActivate(NewGuiInputEvent& event)
{
   // Base default: no-op. NewGuiButton overrides this to call performClick().
}

void NewGuiControl::setMouseOver(bool over)
{
   if (mMouseOver == over)
      return;
   mMouseOver = over;
   setStyleDirty();
}

void NewGuiControl::setMouseActive(bool active)
{
   if (mMouseActive == active)
      return;
   mMouseActive = active;
   setStyleDirty();
}

void NewGuiControl::setFirstResponder(bool responder)
{
   if (mFirstResponder == responder)
      return;
   mFirstResponder = responder;
   setStyleDirty();
}

void NewGuiControl::setDisabled(bool disabled)
{
   if (mDisabled == disabled)
      return;
   mDisabled = disabled;
   setStyleDirty();
}

void NewGuiControl::setChecked(bool checked)
{
   if (mChecked == checked)
      return;
   mChecked = checked;
   setStyleDirty();
}

void NewGuiControl::setHasError(bool error)
{
   if (mHasError == error)
      return;
   mHasError = error;
   setStyleDirty();
}

void NewGuiControl::setStyleAsset(NewGuiStyle* style)
{
   if (mStyle.getObject() == style)
      return;
   mStyle = style;
   setStyleDirty();
}

NewGuiCanvas* NewGuiControl::getOwningCanvas() const
{
   for (SimObject* obj = getGroup(); obj; obj = obj->getGroup())
   {
      NewGuiCanvas* canvas = dynamic_cast<NewGuiCanvas*>(obj);
      if (canvas)
         return canvas;
   }

   return NULL;
}

void NewGuiControl::pushCursor(NewGuiCursorShape shape)
{
   NewGuiCanvas* canvas = getOwningCanvas();
   if (canvas)
      canvas->pushControlCursor(shape);
}

void NewGuiControl::popCursor()
{
   NewGuiCanvas* canvas = getOwningCanvas();
   if (canvas)
      canvas->popControlCursor();
}

// Field setters - route script-driven changes through here so every mutation stamps the correct dirty flag.
bool NewGuiControl::_setWidth(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   NewGuiDimension::setFromString(ctrl->mWidth, data);
   ctrl->setContentDirty();
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setHeight(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   NewGuiDimension::setFromString(ctrl->mHeight, data);
   ctrl->setContentDirty();
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setLeft(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   NewGuiDimension::setFromString(ctrl->mLeft, data);
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setTop(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   NewGuiDimension::setFromString(ctrl->mTop, data);
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setRight(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   NewGuiDimension::setFromString(ctrl->mRight, data);
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setBottom(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   NewGuiDimension::setFromString(ctrl->mBottom, data);
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setVisible(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->mVisible = dAtob(data);
   return false;
}

bool NewGuiControl::_setActive(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->mActive = dAtob(data);
   return false;
}

bool NewGuiControl::_setDisabled(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->setDisabled(dAtob(data));
   return false;
}

bool NewGuiControl::_setChecked(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->setChecked(dAtob(data));
   return false;
}

bool NewGuiControl::_setStyle(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   SimObject* found = Sim::findObject(data);
   ctrl->setStyleAsset(dynamic_cast<NewGuiStyle*>(found));
   return false;
}

bool NewGuiControl::_setTooltipText(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->setTooltipText(data);
   return false;
}

bool NewGuiControl::_setTooltipContent(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   SimObject* found = Sim::findObject(data);
   ctrl->setTooltipContent(dynamic_cast<NewGuiControl*>(found));
   return false;
}

bool NewGuiControl::_setRenderLayerOverride(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->mRenderLayerOverride = dAtoi(data);
   ctrl->setArrangementDirty();
   return false;
}

bool NewGuiControl::_setPreserveAspect(void* obj, const char* index, const char* data)
{
   NewGuiControl* ctrl = static_cast<NewGuiControl*>(obj);
   ctrl->setPreserveAspect(dAtob(data));
   return false;
}

void NewGuiControl::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Layout");

   ADD_FIELD("width", TypeNewGuiDimension, Offset(mWidth, NewGuiControl))
      .onSet(_setWidth)
      .doc("Authored width: px | % | auto (auto = size to preferred content).");

   ADD_FIELD("height", TypeNewGuiDimension, Offset(mHeight, NewGuiControl))
      .onSet(_setHeight)
      .doc("Authored height: px | % | auto.");

   ADD_FIELD("left", TypeNewGuiDimension, Offset(mLeft, NewGuiControl))
      .onSet(_setLeft)
      .doc("Authored left offset within this control's layout slot.");

   ADD_FIELD("top", TypeNewGuiDimension, Offset(mTop, NewGuiControl))
      .onSet(_setTop)
      .doc("Authored top offset within this control's layout slot.");

   ADD_FIELD("right", TypeNewGuiDimension, Offset(mRight, NewGuiControl))
      .onSet(_setRight)
      .doc("Authored right offset; only used when left is auto.");

   ADD_FIELD("bottom", TypeNewGuiDimension, Offset(mBottom, NewGuiControl))
      .onSet(_setBottom)
      .doc("Authored bottom offset; only used when top is auto.");

   ADD_FIELD("renderLayerOverride", TypeS32, Offset(mRenderLayerOverride, NewGuiControl))
      .onSet(_setRenderLayerOverride)
      .doc("Explicit paint-order layer. -1 (default) = parent's layer + 1.");

   ADD_FIELD("tabbable", TypeBool, Offset(mIsTabbable, NewGuiControl))
      .doc("Whether this control is a Tab-key/gamepad focus stop (default true). False removes only this control from the sequence - its children are still walked.");

   ADD_FIELD("tabIndex", TypeS32, Offset(mTabIndex, NewGuiControl))
      .doc("Explicit tab-order override. -1 (default) = auto (natural parent-then-children walk position, matching paint order).");

   ADD_FIELD("preserveAspect", TypeBool, Offset(mPreserveAspect, NewGuiControl))
      .onSet(_setPreserveAspect)
      .doc("When true, this control's own Pixels-mode width/height scale uniformly instead of independently per axis. No effect on Percent/auto sizing or positioning.");

   GROUP_END("Layout");

   GROUP_BEGIN("Style");

   ADD_FIELD("style", TypeSimObjectPtr, 0)
      .onSet(_setStyle)
      .doc("NewGuiStyle object providing this control's cascaded visual properties.");

   GROUP_END("Style");

   GROUP_BEGIN("Tooltip");

   ADD_FIELD("tooltip", TypeString, 0)
      .onSet(_setTooltipText)
      .doc("Plain-text tooltip shown after hovering this control. Ignored if tooltipContent is also set.");

   ADD_FIELD("tooltipContent", TypeSimObjectPtr, 0)
      .onSet(_setTooltipContent)
      .doc("A whole NewGuiControl subtree shown as this control's tooltip instead of plain text. Takes priority over tooltip if both are set.");

   ADD_FIELD("tooltipDelay", TypeS32, Offset(mTooltipDelayMS, NewGuiControl))
      .doc("Milliseconds of continuous hover before this control's tooltip appears. -1 (default) uses the canvas's own default delay.");

   GROUP_END("Tooltip");

   GROUP_BEGIN("State");

   ADD_FIELD("visible", TypeBool, Offset(mVisible, NewGuiControl))
      .onSet(_setVisible)
      .doc("Whether this control (and its subtree) renders.");

   ADD_FIELD("active", TypeBool, Offset(mActive, NewGuiControl))
      .onSet(_setActive)
      .doc("Whether this control participates in layout/input.");

   ADD_FIELD("disabled", TypeBool, Offset(mDisabled, NewGuiControl))
      .onSet(_setDisabled)
      .doc("Disabled interaction state - feeds the style cascade's state mask.");

   ADD_FIELD("checked", TypeBool, Offset(mChecked, NewGuiControl))
      .onSet(_setChecked)
      .doc("Checked interaction state - feeds the style cascade's state mask.");

   ADD_FIELD("hitTestable", TypeBool, Offset(mHitTestable, NewGuiControl))
      .doc("Whether this control itself can be the target of a hit test (default true). False lets pointer events pass through; children are still hit-testable.");

   GROUP_END("State");
}

DefineEngineMethod(NewGuiControl, pushCursor, void, (const char* shapeName), ,
   "Pushes a cursor shape (\"arrow\", \"pointer\", \"text\", \"wait\", \"crosshair\", \"resizeVertical\", \"resizeHorizontal\", \"resizeAll\", \"resizeDiagonalNESW\", \"resizeDiagonalNWSE\", \"waitArrow\", \"notAllowed\") via this control's owning canvas. Every pushCursor() needs a matching popCursor().")
{
   NewGuiCursorShape shape;
   if (!NewGuiCursorShapeFromString(shapeName, shape))
   {
      Con::warnf("NewGuiControl::pushCursor - unrecognized cursor shape '%s'.", shapeName);
      return;
   }

   object->pushCursor(shape);
}

DefineEngineMethod(NewGuiControl, popCursor, void, (), ,
   "Pops whatever this control's most recent pushCursor() call pushed.")
{
   object->popCursor();
}

DefineEngineMethod(NewGuiControl, elevateToFront, void, (), ,
   "Raises this control's paint order (and its whole child subtree, via the normal parent+1 cascade) "
   "above every other control currently in the tree - including other controls with their own "
   "renderLayerOverride - without touching tree/hit-test order. Each call wins over any previous one.")
{
   object->elevateToFront();
}

DefineEngineMethod(NewGuiControl, clearElevation, void, (), ,
   "Reverts an elevateToFront() call, returning this control's paint order to the ordinary parent + 1 default.")
{
   object->clearElevation();
}

DefineEngineMethod(NewGuiControl, isElevated, bool, (), ,
   "True if this control currently has an active elevateToFront() override.")
{
   return object->isElevated();
}
