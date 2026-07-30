//-----------------------------------------------------------------------------
// guiEditCtrlNew.cpp
// See guiEditCtrlNew.h for scope notes.
//-----------------------------------------------------------------------------

#include "gui_refactor/editor/guiEditCtrlNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "gui_refactor/core/guiText.h"
#include "console/consoleTypes.h"
#include "console/engineAPI.h"

IMPLEMENT_CONOBJECT(GuiEditCtrlNew);

//-----------------------------------------------------------------------------

namespace
{
   const ColorI kSelectionOutlineColor(0, 160, 255, 255);
   const ColorI kHandleFillColor(255, 255, 255, 255);
   const ColorI kHandleBorderColor(0, 160, 255, 255);
   const ColorI kPaletteBackgroundColor(48, 48, 48, 255);
   const ColorI kPaletteButtonColor(80, 80, 80, 255);
   const ColorI kPaletteButtonHoverColor(100, 100, 100, 255);
   const ColorI kPaletteDragPreviewColor(0, 160, 255, 120);
}

//-----------------------------------------------------------------------------

GuiEditCtrlNew::GuiEditCtrlNew()
   : mEditRoot(NULL),
   mSelected(NULL),
   mEditActive(false),
   mDragging(false),
   mActiveHandle(EditHandle_None),
   mDragStartMouse(0, 0),
   mDragStartBounds(0, 0, 0, 0),
   mPaletteDragIndex(-1),
   mPaletteDragPos(0, 0),
   mOnRenderCallCount(0)
{
   mIsContainer = true; // lets the editor accept dropped controls, per GuiControlNew::mIsContainer's doc comment
   mCanHit = true;
   mCapturesInput = true;
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::initPersistFields()
{
   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

bool GuiEditCtrlNew::onWake()
{
   if (!Parent::onWake())
      return false;

   return true;
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::addPaletteEntry(const char* className, const char* displayName)
{
   GuiEditPaletteEntry entry;
   entry.className = StringTable->insert(className);
   entry.displayName = StringTable->insert(displayName && displayName[0] ? displayName : className);

   // Text/alignment never change again for this entry's lifetime, so
   // configure them once here rather than every draw -- see
   // GuiEditPaletteEntry::labelText's doc comment. Font/box-extent are
   // NOT set here: font depends on mStyle, which may not be assigned
   // yet at addPaletteEntry() time, and box extent tracks the button's
   // live device-pixel size (changes on window resize) -- both are
   // reconfirmed in _drawPalette() every draw, which is cheap and
   // correctly a no-op when unchanged (see GuiText's own setters, each
   // gated on "did this value actually change" before marking layout
   // dirty -- guiText.cpp).
   entry.labelText.setText(entry.displayName);
   entry.labelText.setAlignHorizontal(GuiTextAlignHorizontal_Center);
   entry.labelText.setAlignVertical(GuiTextAlignVertical_Middle);

   mPaletteEntries.push_back(entry);
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::setEditActive(bool active)
{
   if (mEditActive == active)
      return;

   mEditActive = active;
   GuiControlNew::smDesignTime = active;
   GuiControlNew::smEditorHandle = active ? this : NULL;

   // GuiCanvasNew::rootMouseDown() routes purely through
   // findHitControl() (see its own doc comment there) -- it never
   // consults smEditorHandle/onMouseDownEditor at all, so those hooks
   // are NOT a real interception mechanism in this codebase as it
   // stands today. The only way to guarantee this editor -- not
   // whatever's been placed under mEditRoot -- receives every click
   // while active is to make mEditRoot un-hit-testable FROM ABOVE:
   // findHitControl(), called on the canvas/editor, checks each CHILD's
   // mCanHit before recursing into it (see guiControlNew.cpp) -- so
   // mEditRoot->setCanHit(false) stops that outer traversal from ever
   // descending into the edited tree, and it falls back to returning
   // this editor instead. This does NOT block _findSelectableAt()'s own
   // hit-testing below, since that calls mEditRoot->findHitControl()
   // DIRECTLY -- that call checks mEditRoot's CHILDREN's mCanHit, never
   // mEditRoot's own, so the edited tree is still fully searchable from
   // the editor's internal logic even while invisible to the canvas.
   // Restored when deactivating so the edited tree behaves normally
   // (e.g. for spot-checking real interaction) when the editor isn't
   // intercepting.
   if (mEditRoot)
      mEditRoot->setCanHit(!active);

   if (!active)
   {
      // Drop any in-progress interaction rather than leaving it dangling
      // across an activate/deactivate toggle.
      mDragging = false;
      mActiveHandle = EditHandle_None;
      mPaletteDragIndex = -1;
   }
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::setEditRoot(GuiControlNew* root)
{
   if (mEditRoot == root)
      return;

   // Restore hit-testing on whatever the PREVIOUS root was before
   // switching -- see setEditActive()'s doc comment on why mCanHit is
   // what actually gates input delivery here. Without this, calling
   // setEditRoot() a second time while active would leave the old root
   // permanently un-hit-testable.
   if (mEditRoot && mEditActive)
      mEditRoot->setCanHit(true);

   mEditRoot = root;

   if (mEditRoot && mEditActive)
      mEditRoot->setCanHit(false);
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::select(GuiControlNew* ctrl)
{
   mSelected = ctrl;
   mDragging = false;
   mActiveHandle = EditHandle_None;
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiEditCtrlNew::_findSelectableAt(const Point2I& localPt) const
{
   if (!mEditRoot)
      return NULL;

   // localPt is in THIS control's local space; re-express relative to
   // mEditRoot via canvas-global space (mEditRoot is a descendant of
   // this editor, but not necessarily a direct child once nested
   // containers are involved -- global space is the one frame both
   // controls agree on regardless of depth).
   const Point2I globalPt = localToGlobalCoord(localPt);
   const Point2I rootLocalPt = mEditRoot->globalToLocalCoord(globalPt);

   GuiControlNew* hit = _findDeepestAtIgnoringHitFlags(mEditRoot, rootLocalPt);

   // Can be mEditRoot itself (nothing deeper under the point) -- that's
   // "clicked empty space inside the root," not a selectable child.
   if (hit == mEditRoot)
      return NULL;

   return hit;
}

//-----------------------------------------------------------------------------

GuiControlNew* GuiEditCtrlNew::_findDeepestAtIgnoringHitFlags(GuiControlNew* ctrl, const Point2I& localPt)
{
   // Deliberately reimplements findHitControl()'s bounds-overlap/z-order
   // walk rather than calling it (or findHitControls()), because BOTH of
   // the engine's real hit-test paths gate on flags that mean "don't
   // intercept mouse input at runtime" -- exactly right for actual
   // clicks, exactly wrong for editor selection:
   //   - findHitControl() requires getCapturesInput() == true on the
   //     result (see guiControlNew.cpp) -- every concrete control here
   //     sets this explicitly except GuiLabelCtrlNew, which correctly
   //     leaves it false (a passive label shouldn't swallow clicks meant
   //     for whatever's behind it at runtime).
   //   - findHitControls() (plural) drops that gate but still requires
   //     mCanHit == true -- and GuiLabelCtrlNew's constructor explicitly
   //     sets setCanHit(false) ("static labels don't intercept mouse
   //     events by default"), which is the right runtime default for a
   //     label and the wrong one for "can the editor select this."
   // Selection is a pure "what's under this point" query that should be
   // decided entirely by geometry, not by whichever concrete control
   // class happens to opt out of runtime mouse interaction and for
   // completely unrelated (correct, in their own context) reasons. Only
   // mVisible is still honored -- an invisible control isn't a
   // reasonable thing to select regardless of interaction flags.
   if (!ctrl->isVisible())
      return NULL;

   for (GuiControlNew::iterator i = ctrl->end(); i != ctrl->begin();)
   {
      --i;
      GuiControlNew* child = static_cast<GuiControlNew*>(*i);

      if (child->isVisible() && child->pointInControl(localPt))
      {
         const Point2I childLocalPt = localPt - child->getPosition();
         GuiControlNew* deeper = _findDeepestAtIgnoringHitFlags(child, childLocalPt);
         if (deeper)
            return deeper;
         return child;
      }
   }

   return ctrl;
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::_getHandlePoints(const RectI& bounds, Point2I outPoints[9]) const
{
   const S32 l = bounds.point.x;
   const S32 t = bounds.point.y;
   const S32 r = bounds.point.x + bounds.extent.x;
   const S32 b = bounds.point.y + bounds.extent.y;
   const S32 cx = (l + r) / 2;
   const S32 cy = (t + b) / 2;

   outPoints[EditHandle_TopLeft] = Point2I(l, t);
   outPoints[EditHandle_Top] = Point2I(cx, t);
   outPoints[EditHandle_TopRight] = Point2I(r, t);
   outPoints[EditHandle_Right] = Point2I(r, cy);
   outPoints[EditHandle_BottomRight] = Point2I(r, b);
   outPoints[EditHandle_Bottom] = Point2I(cx, b);
   outPoints[EditHandle_BottomLeft] = Point2I(l, b);
   outPoints[EditHandle_Left] = Point2I(l, cy);
}

//-----------------------------------------------------------------------------

GuiEditHandle GuiEditCtrlNew::_hitTestHandle(const Point2I& localPt) const
{
   if (!mSelected)
      return EditHandle_None;

   // mSelected's bounds, re-expressed in THIS control's local space --
   // same global-space bridge as _findSelectableAt().
   GuiControlNew* selectedParent = mSelected->getParent();
   if (!selectedParent)
      return EditHandle_None;

   const Point2I selGlobalPos = selectedParent->localToGlobalCoord(mSelected->getPosition());
   const Point2I selLocalPos = globalToLocalCoord(selGlobalPos);
   const RectI selLocalBounds(selLocalPos, mSelected->getExtent());

   Point2I handlePoints[9];
   _getHandlePoints(selLocalBounds, handlePoints);

   for (S32 h = EditHandle_TopLeft; h <= EditHandle_Left; ++h)
   {
      const Point2I& hp = handlePoints[h];
      if (localPt.x >= hp.x - smHandleHalfSize && localPt.x <= hp.x + smHandleHalfSize &&
         localPt.y >= hp.y - smHandleHalfSize && localPt.y <= hp.y + smHandleHalfSize)
      {
         return (GuiEditHandle)h;
      }
   }

   return EditHandle_None;
}

//-----------------------------------------------------------------------------

S32 GuiEditCtrlNew::_hitTestPalette(const Point2I& localPt) const
{
   for (S32 i = 0; i < mPaletteButtonRects.size(); ++i)
   {
      const RectI& r = mPaletteButtonRects[i];
      if (localPt.x >= r.point.x && localPt.x <= r.point.x + r.extent.x &&
         localPt.y >= r.point.y && localPt.y <= r.point.y + r.extent.y)
      {
         return i;
      }
   }
   return -1;
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::_applyBoundsPreservingMode(GuiControlNew* target, const RectI& newBounds)
{
   if (!target)
      return;

   GuiControlNew* parent = target->getParent();
   F32 parentW = 0.0f, parentH = 0.0f;
   if (parent)
   {
      parentW = (F32)parent->getExtent().x;
      parentH = (F32)parent->getExtent().y;
   }

   // Captured once, up front -- see this method's doc comment in the
   // header for why (avoids each field's percent math reading a
   // partially-updated extent from an earlier field write in this same
   // call).
   const RectI oldBounds(target->getPosition(), target->getExtent());

   auto writeAxis = [&](GuiDimension& field, PendingDimensionAxis axis,
      bool isWidthAxis, bool isPositionAxis, S32 newValuePx)
   {
      GuiDimension newDim;
      if (field.isPercent() && parent)
      {
         const F32 reference = isWidthAxis ? parentW : parentH;
         const F32 pct = (reference > 0.0f) ? (100.0f * (F32)newValuePx / reference) : 0.0f;
         newDim = GuiDimension::fromPercent(pct);
      }
      else
      {
         // Auto has no numeric value to preserve, and Pixels stays
         // Pixels -- both become an explicit pixel value, matching the
         // "explicit placement" intent of a direct editor drag.
         newDim = GuiDimension::fromPixels((F32)newValuePx);
      }

      field = newDim;
      target->_resolveAndApplyDimension(field, axis, isWidthAxis, isPositionAxis);
   };

   // Position first (left/top), matching the order script-driven field
   // sets already happen in, then size.
   writeAxis(target->mLeft, GuiControlNew::PendingLeft, true, true, newBounds.point.x);
   writeAxis(target->mTop, GuiControlNew::PendingTop, false, true, newBounds.point.y);
   writeAxis(target->mWidth, GuiControlNew::PendingWidth, true, false, newBounds.extent.x);
   writeAxis(target->mHeight, GuiControlNew::PendingHeight, false, false, newBounds.extent.y);

   (void)oldBounds; // kept for potential future undo support; unused for now
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::onRender(Point2I offset, const RectI& updateRect)
{
   ++mOnRenderCallCount; // see debugDumpAllocState()

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;

   GuiRenderBatch& batch = root->getRenderBatch();

   // mEditRoot is a CHILD of this editor (added via addObject(), same as
   // any other GuiControlNew hierarchy -- see setEditRoot()'s doc
   // comment), so it's already drawn by the normal child traversal here.
   // No separate manual render/offset math needed.
   renderChildControls(offset, updateRect);

   if (!mEditActive)
      return;

   // Computed once per frame (not cached) and used as the base layer for
   // ALL editor chrome below -- see GuiControlNew::
   // _findMaxRenderLayerInSubtree()'s doc comment (guiControlNew.h) for
   // why a fixed small offset from this editor's own getRenderLayer()
   // isn't enough once the edited tree has any nesting. Shared with
   // GuiControlNew::bringToFront(), which needed the exact same "true
   // max render layer in a subtree" query for the exact same reason.
   const S32 chromeBaseLayer = GuiControlNew::_findMaxRenderLayerInSubtree(mEditRoot) + 1;

   _drawSelectionAndHandles(batch, offset, chromeBaseLayer);
   _drawPalette(batch, offset, chromeBaseLayer);

   if (mPaletteDragIndex >= 0)
      _drawPaletteDragPreview(batch, offset, chromeBaseLayer);
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::_drawSelectionAndHandles(GuiRenderBatch& batch, const Point2I& offset, S32 baseLayer)
{
   if (!mSelected)
      return;

   GuiControlNew* selectedParent = mSelected->getParent();
   if (!selectedParent)
      return;

   const Point2I selGlobalPos = selectedParent->localToGlobalCoord(mSelected->getPosition());
   const Point2I selLocalPos = globalToLocalCoord(selGlobalPos);
   const RectI selLocalBounds(selLocalPos, mSelected->getExtent());

   // offset is THIS control's device-pixel draw origin (same convention
   // as every other onRender()); selLocalBounds is in editor-local
   // logical units, so project through the canvas the same way
   // getDeviceBounds() would for a real control -- offset + logical*scale.
   GuiCanvasNew* root = getRoot();
   const F32 sx = root->getEffectiveScaleX();
   const F32 sy = root->getEffectiveScaleY();

   const RectI deviceBounds(
      Point2I(offset.x + (S32)(selLocalBounds.point.x * sx), offset.y + (S32)(selLocalBounds.point.y * sy)),
      Point2I((S32)(selLocalBounds.extent.x * sx), (S32)(selLocalBounds.extent.y * sy)));

   const S32 outlineThickness = 1;
   batch.pushQuad(RectI(deviceBounds.point, Point2I(deviceBounds.extent.x, outlineThickness)), kSelectionOutlineColor, baseLayer);
   batch.pushQuad(RectI(Point2I(deviceBounds.point.x, deviceBounds.point.y + deviceBounds.extent.y - outlineThickness), Point2I(deviceBounds.extent.x, outlineThickness)), kSelectionOutlineColor, baseLayer);
   batch.pushQuad(RectI(deviceBounds.point, Point2I(outlineThickness, deviceBounds.extent.y)), kSelectionOutlineColor, baseLayer);
   batch.pushQuad(RectI(Point2I(deviceBounds.point.x + deviceBounds.extent.x - outlineThickness, deviceBounds.point.y), Point2I(outlineThickness, deviceBounds.extent.y)), kSelectionOutlineColor, baseLayer);

   Point2I handlePointsLocal[9];
   _getHandlePoints(selLocalBounds, handlePointsLocal);

   const S32 handleDevHalf = (S32)(smHandleHalfSize * getMin(sx, sy));
   const S32 handleLayer = baseLayer + 1; // strictly above the selection outline itself

   for (S32 h = EditHandle_TopLeft; h <= EditHandle_Left; ++h)
   {
      const Point2I hpLocal = handlePointsLocal[h];
      const Point2I hpDevice(offset.x + (S32)(hpLocal.x * sx), offset.y + (S32)(hpLocal.y * sy));
      const RectI handleRect(
         Point2I(hpDevice.x - handleDevHalf, hpDevice.y - handleDevHalf),
         Point2I(handleDevHalf * 2, handleDevHalf * 2));

      batch.pushQuad(handleRect, kHandleFillColor, handleLayer);
      batch.pushQuad(RectI(handleRect.point, Point2I(handleRect.extent.x, 1)), kHandleBorderColor, handleLayer);
      batch.pushQuad(RectI(Point2I(handleRect.point.x, handleRect.point.y + handleRect.extent.y - 1), Point2I(handleRect.extent.x, 1)), kHandleBorderColor, handleLayer);
      batch.pushQuad(RectI(handleRect.point, Point2I(1, handleRect.extent.y)), kHandleBorderColor, handleLayer);
      batch.pushQuad(RectI(Point2I(handleRect.point.x + handleRect.extent.x - 1, handleRect.point.y), Point2I(1, handleRect.extent.y)), kHandleBorderColor, handleLayer);
   }
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::_drawPalette(GuiRenderBatch& batch, const Point2I& offset, S32 baseLayer)
{
   if (mPaletteEntries.empty())
      return;

   GuiCanvasNew* root = getRoot();
   const F32 sx = root->getEffectiveScaleX();
   const F32 sy = root->getEffectiveScaleY();

   const RectI paletteDeviceRect(offset, Point2I((S32)(getWidth() * sx), (S32)(smPaletteHeight * sy)));
   batch.pushQuad(paletteDeviceRect, kPaletteBackgroundColor, baseLayer);

   mPaletteButtonRects.clear();

   // Logical-space layout constants -- consistent with every other
   // dimension in this file (smHandleHalfSize, smPaletteHeight, etc.),
   // converted to device pixels only at the point of drawing. Hit-
   // testing (_hitTestPalette()) compares against the LOGICAL rects
   // stored in mPaletteButtonRects, never the device ones, so the two
   // can never drift apart the way they did when button geometry was
   // built directly in device space but compared against a logical
   // mouse point.
   const S32 buttonWidthLogical = 96;
   const S32 buttonPaddingLogical = 4;
   S32 cursorXLogical = buttonPaddingLogical;

   for (S32 i = 0; i < mPaletteEntries.size(); ++i)
   {
      const RectI buttonRectLogical(
         Point2I(cursorXLogical, buttonPaddingLogical),
         Point2I(buttonWidthLogical, smPaletteHeight - buttonPaddingLogical * 2));
      mPaletteButtonRects.push_back(buttonRectLogical);

      const RectI buttonRectDevice(
         Point2I(offset.x + (S32)(buttonRectLogical.point.x * sx), offset.y + (S32)(buttonRectLogical.point.y * sy)),
         Point2I((S32)(buttonRectLogical.extent.x * sx), (S32)(buttonRectLogical.extent.y * sy)));

      const bool isDragSource = (mPaletteDragIndex == i);
      batch.pushQuad(buttonRectDevice, isDragSource ? kPaletteButtonHoverColor : kPaletteButtonColor, baseLayer);

      if (mStyle)
      {
         const Resource<GFont> fontRes = mStyle->getResolvedFont(0);
         if (fontRes != NULL)
         {
            // Persistent per-entry GuiText (GuiEditPaletteEntry::labelText),
            // reconfigured and submit()'d -- deliberately NOT
            // GuiText::renderSimple(), which constructs a fresh stack-local
            // GuiText every call. renderSimple() is fine for a genuine
            // one-shot draw, but this runs every frame for as long as the
            // editor is active, and a fresh GuiText means mLayoutDirty
            // starts true every time -- a full decode/wrap/measure layout
            // pass redone from scratch every frame, forever, with nothing
            // ever reused. This is the EXACT bug class already hit once
            // with tooltip rendering (see GuiControlNew::
            // defaultTooltipRender()'s cache in guiControlNew.cpp and its
            // own doc comment) -- the fix there was a persistent GuiText
            // reused across calls, not a fresh one each time, and that's
            // the same fix applied here. setFont()/setBoxExtent() are
            // still called every draw, but GuiText's own setters no-op
            // (skip marking layout dirty) when the value hasn't actually
            // changed, so this only triggers real re-layout work when the
            // font or button size genuinely changes -- not every frame.
            GuiEditPaletteEntry& entry = mPaletteEntries[i];
            entry.labelText.setFont(fontRes);
            entry.labelText.setBoxExtent(buttonRectDevice.extent);
            entry.labelText.submit(batch, buttonRectDevice.point, ColorI(255, 255, 255, 255), 0, 0, baseLayer);
         }
      }
      // else: no style assigned to this editor -- button colors alone
      // distinguish entries. See setStyle()/mStyle; assign a GuiStyle to
      // the editor object from script to get labels.

      cursorXLogical += buttonWidthLogical + buttonPaddingLogical;
   }
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::_drawPaletteDragPreview(GuiRenderBatch& batch, const Point2I& offset, S32 baseLayer)
{
   GuiCanvasNew* root = getRoot();
   const F32 sx = root->getEffectiveScaleX();
   const F32 sy = root->getEffectiveScaleY();

   const S32 previewW = 96, previewH = 24;
   const RectI previewDeviceRect(
      Point2I(offset.x + (S32)(mPaletteDragPos.x * sx) - previewW / 2, offset.y + (S32)(mPaletteDragPos.y * sy) - previewH / 2),
      Point2I(previewW, previewH));

   // +2: above the palette buttons (baseLayer) and above the selection
   // handles (baseLayer + 1), so a drag preview is never hidden by
   // either while in progress.
   batch.pushQuad(previewDeviceRect, kPaletteDragPreviewColor, baseLayer + 2);
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::onMouseDown(const GuiEvent& event)
{
   if (!mEditActive)
   {
      Parent::onMouseDown(event);
      return;
   }

   const Point2I localPt = globalToLocalCoord(event.mousePoint);

   // Palette first -- it's drawn on top and its buttons take priority
   // over anything in the edited tree beneath the strip.
   const S32 paletteHit = _hitTestPalette(localPt);
   if (paletteHit >= 0)
   {
      mPaletteDragIndex = paletteHit;
      mPaletteDragPos = localPt;
      mouseLock(this);
      return;
   }

   // Resize handle on the current selection takes priority over
   // re-selecting/moving.
   const GuiEditHandle handle = _hitTestHandle(localPt);
   if (handle != EditHandle_None && mSelected)
   {
      mActiveHandle = handle;
      mDragging = true;
      mDragStartMouse = localPt;
      mDragStartBounds = RectI(mSelected->getPosition(), mSelected->getExtent());
      mouseLock(this);
      return;
   }

   GuiControlNew* hit = _findSelectableAt(localPt);
   select(hit);

   if (mSelected)
   {
      mActiveHandle = EditHandle_None;
      mDragging = true;
      mDragStartMouse = localPt;
      mDragStartBounds = RectI(mSelected->getPosition(), mSelected->getExtent());
      mouseLock(this);
   }
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::onMouseUp(const GuiEvent& event)
{
   if (!mEditActive)
   {
      Parent::onMouseUp(event);
      return;
   }

   if (mPaletteDragIndex >= 0)
   {
      const Point2I localPt = globalToLocalCoord(event.mousePoint);

      if (mEditRoot)
      {
         const GuiEditPaletteEntry& entry = mPaletteEntries[mPaletteDragIndex];

         // Only place if the drop point is actually over the edit root
         // (not still over the palette strip itself).
         if (localPt.y > smPaletteHeight)
         {
            ConsoleObject* obj = ConsoleObject::create(entry.className);
            GuiControlNew* newCtrl = dynamic_cast<GuiControlNew*>(obj);
            if (newCtrl)
            {
               if (newCtrl->registerObject())
               {
                  const Point2I rootLocalPt = mEditRoot->globalToLocalCoord(localToGlobalCoord(localPt));

                  static const S32 kDefaultW = 100, kDefaultH = 30;
                  newCtrl->mLeft = GuiDimension::fromPixels((F32)(rootLocalPt.x - kDefaultW / 2));
                  newCtrl->mTop = GuiDimension::fromPixels((F32)(rootLocalPt.y - kDefaultH / 2));
                  newCtrl->mWidth = GuiDimension::fromPixels((F32)kDefaultW);
                  newCtrl->mHeight = GuiDimension::fromPixels((F32)kDefaultH);

                  mEditRoot->addObject(newCtrl);
                  select(newCtrl);
               }
               else
               {
                  delete newCtrl;
               }
            }
         }
      }

      mPaletteDragIndex = -1;
      mouseUnlock();
      return;
   }

   if (mDragging)
   {
      mDragging = false;
      mActiveHandle = EditHandle_None;
      mouseUnlock();
   }
}

//-----------------------------------------------------------------------------

void GuiEditCtrlNew::onMouseDragged(const GuiEvent& event)
{
   if (!mEditActive)
   {
      Parent::onMouseDragged(event);
      return;
   }

   const Point2I localPt = globalToLocalCoord(event.mousePoint);

   if (mPaletteDragIndex >= 0)
   {
      mPaletteDragPos = localPt;
      return;
   }

   if (!mDragging || !mSelected)
      return;

   const Point2I delta = localPt - mDragStartMouse;

   RectI newBounds = mDragStartBounds;

   if (mActiveHandle == EditHandle_None)
   {
      // Move.
      newBounds.point = mDragStartBounds.point + delta;
   }
   else
   {
      // Resize: adjust the edge(s) implied by the active handle, keeping
      // the opposite edge(s) fixed. Clamp so extent never goes negative.
      S32 left = mDragStartBounds.point.x;
      S32 top = mDragStartBounds.point.y;
      S32 right = mDragStartBounds.point.x + mDragStartBounds.extent.x;
      S32 bottom = mDragStartBounds.point.y + mDragStartBounds.extent.y;

      const bool affectsLeft = (mActiveHandle == EditHandle_TopLeft || mActiveHandle == EditHandle_Left || mActiveHandle == EditHandle_BottomLeft);
      const bool affectsRight = (mActiveHandle == EditHandle_TopRight || mActiveHandle == EditHandle_Right || mActiveHandle == EditHandle_BottomRight);
      const bool affectsTop = (mActiveHandle == EditHandle_TopLeft || mActiveHandle == EditHandle_Top || mActiveHandle == EditHandle_TopRight);
      const bool affectsBottom = (mActiveHandle == EditHandle_BottomLeft || mActiveHandle == EditHandle_Bottom || mActiveHandle == EditHandle_BottomRight);

      if (affectsLeft)   left += delta.x;
      if (affectsRight)  right += delta.x;
      if (affectsTop)    top += delta.y;
      if (affectsBottom) bottom += delta.y;

      static const S32 kMinExtent = 8;
      if (right - left < kMinExtent)
      {
         if (affectsLeft) left = right - kMinExtent;
         else right = left + kMinExtent;
      }
      if (bottom - top < kMinExtent)
      {
         if (affectsTop) top = bottom - kMinExtent;
         else bottom = top + kMinExtent;
      }

      newBounds = RectI(Point2I(left, top), Point2I(right - left, bottom - top));
   }

   _applyBoundsPreservingMode(mSelected, newBounds);
}

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Script bindings
//-----------------------------------------------------------------------------

DefineEngineMethod(GuiEditCtrlNew, setEditRoot, void, (GuiControlNew* root), ,
   "Sets the control tree this editor places/selects/arranges controls within.\n"
   "@param root The control whose children are edited; new controls from the palette are added under it. "
   "Must already be a descendant of this editor (typically a direct child) -- see GuiEditCtrlNew::mEditRoot.")
{
   object->setEditRoot(root);
}

DefineEngineMethod(GuiEditCtrlNew, getEditRoot, GuiControlNew*, (), ,
   "@return the control tree currently being edited, or NULL if none is set.")
{
   return object->getEditRoot();
}

DefineEngineMethod(GuiEditCtrlNew, addPaletteEntry, void, (const char* className, const char* displayName), (""),
   "Registers a class as a palette entry that can be dragged onto the edit surface to create a new instance.\n"
   "@param className The ConsoleObject class to instantiate, e.g. \"GuiButtonCtrlNew\".\n"
   "@param displayName Optional label for the palette button; defaults to className if omitted.")
{
   object->addPaletteEntry(className, (displayName && displayName[0]) ? displayName : NULL);
}

DefineEngineMethod(GuiEditCtrlNew, setEditActive, void, (bool active), ,
   "Activates or deactivates editor input interception (selection/move/resize/palette).\n"
   "@param active True to start intercepting input as the editor; false to let input pass through normally.")
{
   object->setEditActive(active);
}

DefineEngineMethod(GuiEditCtrlNew, isEditActive, bool, (), ,
   "@return true if this editor is currently intercepting input.")
{
   return object->isEditActive();
}

DefineEngineMethod(GuiEditCtrlNew, select, void, (GuiControlNew* ctrl), ,
   "Selects the given control (must be within the current edit root), or pass 0/NULL to deselect.")
{
   object->select(ctrl);
}

DefineEngineMethod(GuiEditCtrlNew, getSelected, GuiControlNew*, (), ,
   "@return the currently selected control, or NULL if nothing is selected.")
{
   return object->getSelected();
}

//-----------------------------------------------------------------------------
// TEMPORARY diagnostics -- see the "Diagnostics" section of
// guiEditCtrlNew.h for why this exists and when to delete it.
//-----------------------------------------------------------------------------

void GuiEditCtrlNew::debugDumpAllocState()
{
   static U32 sLastCallCount = 0;
   static S32 sLastPaletteRectCapacity = -1;
   static S32 sLastPaletteEntryCapacity = -1;

   const S32 paletteRectCapacity = mPaletteButtonRects.capacity();
   const S32 paletteEntryCapacity = mPaletteEntries.capacity();
   const S32 editRootChildCount = mEditRoot ? mEditRoot->size() : -1;

   Con::printf("GuiEditCtrlNew::debugDumpAllocState [%s]:", getIdString());
   Con::printf("  onRender calls this session: %u (delta since last call: %u)",
      mOnRenderCallCount, mOnRenderCallCount - sLastCallCount);
   Con::printf("  mPaletteButtonRects: size=%d capacity=%d (prev capacity=%d)",
      mPaletteButtonRects.size(), paletteRectCapacity, sLastPaletteRectCapacity);
   Con::printf("  mPaletteEntries: size=%d capacity=%d (prev capacity=%d)",
      mPaletteEntries.size(), paletteEntryCapacity, sLastPaletteEntryCapacity);
   Con::printf("  mEditRoot child count: %d", editRootChildCount);
   Con::printf("  mEditActive=%d mSelected=%s mDragging=%d mPaletteDragIndex=%d",
      mEditActive, mSelected ? mSelected->getIdString() : "NULL", mDragging, mPaletteDragIndex);
   Con::printf("  -- if capacities above are NOT climbing call-to-call while your external memory");
   Con::printf("     reading IS climbing, the cause is outside GuiEditCtrlNew (check GuiCanvasNew's");
   Con::printf("     own per-frame update path, or anything else ticking every frame).");

   sLastCallCount = mOnRenderCallCount;
   sLastPaletteRectCapacity = paletteRectCapacity;
   sLastPaletteEntryCapacity = paletteEntryCapacity;
}

DefineEngineMethod(GuiEditCtrlNew, debugDumpAllocState, void, (), ,
   "TEMPORARY diagnostic -- prints this editor's own per-frame counters/container "
   "capacities to the console, and how much they changed since the last call. Call this "
   "twice a few seconds apart while memory is climbing: if these numbers are flat while "
   "external memory keeps rising, the cause is not inside GuiEditCtrlNew. See the class's "
   "header for more.")
{
   object->debugDumpAllocState();
}

//-----------------------------------------------------------------------------

bool GuiEditCtrlNew::onKeyDown(const GuiEvent& event)
{
   if (!mEditActive)
      return Parent::onKeyDown(event);

   // Ctrl+D: dump debugDumpAllocState()'s output to the console/log.
   // Added because this minimal test setup (GuiEditCtrlNew directly on
   // the canvas, no GuiConsoleCtrlNew) has no way to type a script
   // command in to call debugDumpAllocState() by hand -- Con::printf()
   // still reaches the normal engine log/stdout without any console
   // widget, so a keybind is the only UI-free way to trigger it. Gated
   // on Ctrl so a bare 'D' keystroke is still free for anything else
   // later (e.g. renaming a selection, if that's ever added).
   if (event.keyCode == KEY_D && (event.modifier & SI_CTRL))
   {
      debugDumpAllocState();
      return true;
   }

   // Deliberately minimal for this first pass -- see file header.
   // Delete/backspace removes the current selection, since leaving a
   // just-placed test control with no way to remove it short of editing
   // script/.gui by hand would be a real workflow gap even in a
   // no-inspector first cut.
   if ((event.keyCode == KEY_DELETE || event.keyCode == KEY_BACKSPACE) && mSelected)
   {
      GuiControlNew* toRemove = mSelected;
      select(NULL);
      toRemove->deleteObject();
      return true;
   }

   return Parent::onKeyDown(event);
}
