//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiScroll.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiScroll.h"
#include "gui_rev2/core/newGuiRenderBatch.h"

IMPLEMENT_CONOBJECT(NewGuiScroll);

static const S32 kMinThumbLength = 20;      ///< Smallest a thumb is ever allowed to shrink to, in pixels.
static const S32 kPageOverlap = 20;         ///< Pixels of overlap kept when paging via a gutter click.
static const S32 kPixelsPerWheelStep = 40;  ///< Wheel-step -> pixel conversion.

NewGuiScroll::NewGuiScroll()
   : mScrollBarMode(ScrollBar_Vertical),
   mScrollOffset(0, 0),
   mScrollBarThickness(16),
   mContentExtent(0, 0),
   mDragInProgress(false),
   mDragLastPoint(0, 0),
   mThumbDragAxis(ScrollAxis_None),
   mThumbDragStartAxisPos(0),
   mThumbDragStartScrollOffset(0),
   mThumbHoverAxis(ScrollAxis_None)
{
}

NewGuiScroll::~NewGuiScroll()
{
}

bool NewGuiScroll::_setScrollBarMode(void* obj, const char* index, const char* data)
{
   NewGuiScroll* scroll = static_cast<NewGuiScroll*>(obj);

   ScrollBarMode mode = ScrollBar_None;
   if (dStricmp(data, "vertical") == 0)
      mode = ScrollBar_Vertical;
   else if (dStricmp(data, "horizontal") == 0)
      mode = ScrollBar_Horizontal;
   else if (dStricmp(data, "both") == 0)
      mode = ScrollBar_Both;
   else
      mode = ScrollBar_None;

   scroll->mScrollBarMode = mode;

   // A gutter may appear/disappear - re-arrange, and re-clamp in case a disabled axis had an offset.
   scroll->clampScrollOffset();
   scroll->setArrangementDirty();
   return false;
}

void NewGuiScroll::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Scroll");

   ADD_FIELD("scrollBarMode", TypeString, 0)
      .onSet(_setScrollBarMode)
      .doc("Which axes may scroll: none (default), vertical, horizontal, both.");

   ADD_FIELD("scrollBarThickness", TypeS32, Offset(mScrollBarThickness, NewGuiScroll))
      .doc("Pixels reserved for a scrollbar gutter on whichever axis is scrolling, and the thickness the visible thumb/track is drawn at.");

   GROUP_END("Scroll");
}

bool NewGuiScroll::needsVerticalScrollbar() const
{
   if (mScrollBarMode != ScrollBar_Vertical && mScrollBarMode != ScrollBar_Both)
      return false;
   return mContentExtent.y > mBounds.extent.y;
}

bool NewGuiScroll::needsHorizontalScrollbar() const
{
   if (mScrollBarMode != ScrollBar_Horizontal && mScrollBarMode != ScrollBar_Both)
      return false;
   return mContentExtent.x > mBounds.extent.x;
}

// Local-space (0,0 = mBounds.point) gutter rects. Each shrinks by the other gutter's thickness
// when both show, leaving a dead square in the corner neither claims.
RectI NewGuiScroll::getVerticalGutterRect() const
{
   if (!needsVerticalScrollbar())
      return RectI(0, 0, 0, 0);

   S32 height = mBounds.extent.y;
   if (needsHorizontalScrollbar())
      height = getMax(0, height - mScrollBarThickness);

   return RectI(Point2I(mBounds.extent.x - mScrollBarThickness, 0), Point2I(mScrollBarThickness, height));
}

RectI NewGuiScroll::getHorizontalGutterRect() const
{
   if (!needsHorizontalScrollbar())
      return RectI(0, 0, 0, 0);

   S32 width = mBounds.extent.x;
   if (needsVerticalScrollbar())
      width = getMax(0, width - mScrollBarThickness);

   return RectI(Point2I(0, mBounds.extent.y - mScrollBarThickness), Point2I(width, mScrollBarThickness));
}

// Proportional thumb sizing/position, relative to gutterRect's own local space.
RectI NewGuiScroll::computeThumbRect(const RectI& gutterRect, bool vertical) const
{
   const S32 trackLength = vertical ? gutterRect.extent.y : gutterRect.extent.x;
   const S32 viewportExtent = vertical ? mBounds.extent.y : mBounds.extent.x;
   const S32 contentExtent = vertical ? mContentExtent.y : mContentExtent.x;
   const S32 scrollOffset = vertical ? mScrollOffset.y : mScrollOffset.x;

   if (trackLength <= 0 || contentExtent <= 0 || viewportExtent <= 0)
      return RectI(0, 0, 0, 0);

   S32 thumbLength = (S32)(((F32)viewportExtent / (F32)contentExtent) * (F32)trackLength);
   thumbLength = mClamp(thumbLength, kMinThumbLength, trackLength);

   const S32 maxScrollOffset = getMax(0, contentExtent - viewportExtent);
   const S32 maxThumbTravel = trackLength - thumbLength;

   S32 thumbStart = 0;
   if (maxScrollOffset > 0 && maxThumbTravel > 0)
      thumbStart = (S32)(((F32)scrollOffset / (F32)maxScrollOffset) * (F32)maxThumbTravel);

   if (vertical)
      return RectI(Point2I(0, thumbStart), Point2I(gutterRect.extent.x, thumbLength));

   return RectI(Point2I(thumbStart, 0), Point2I(thumbLength, gutterRect.extent.y));
}

// Starts from Parent's padding-inset client rect, narrows for any showing gutter, then shifts by -mScrollOffset.
RectI NewGuiScroll::GetClientRect() const
{
   RectI clientRect = Parent::GetClientRect();

   if (needsVerticalScrollbar())
      clientRect.extent.x = getMax(0, clientRect.extent.x - mScrollBarThickness);

   if (needsHorizontalScrollbar())
      clientRect.extent.y = getMax(0, clientRect.extent.y - mScrollBarThickness);

   clientRect.point.x -= mScrollOffset.x;
   clientRect.point.y -= mScrollOffset.y;

   return clientRect;
}

// Deliberately does not use Parent's default (union of children) - that would size to fit all
// content, defeating the point of scrolling.
Point2I NewGuiScroll::ComputePreferredSize()
{
   static const S32 kDefaultAutoExtent = 200;
   return Point2I(kDefaultAutoExtent, kDefaultAutoExtent);
}

void NewGuiScroll::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   Parent::ArrangePass(slotRect, parentRenderLayer, uiScaleX, uiScaleY);

   RecomputeContentExtentAndReclamp([&]()
   {
      Parent::ArrangePass(slotRect, parentRenderLayer, uiScaleX, uiScaleY);
   });
}

// REQUIRED override - without it, a Stack-managed scroll (placed via ArrangePassWithFixedExtent(),
// not ArrangePass()) would never recompute mContentExtent, leaving it permanently un-scrollable.
void NewGuiScroll::ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   Parent::ArrangePassWithFixedExtent(finalBounds, parentRenderLayer, uiScaleX, uiScaleY);

   RecomputeContentExtentAndReclamp([&]()
   {
      Parent::ArrangePassWithFixedExtent(finalBounds, parentRenderLayer, uiScaleX, uiScaleY);
   });
}

// Shared by ArrangePass()/ArrangePassWithFixedExtent() so the two entry points can't drift apart.
// Requires mBounds/GetClientRect() to already be current - only reads already-resolved child bounds.
void NewGuiScroll::RecomputeContentExtentAndReclamp(const std::function<void()>& reArrangeSelf)
{
   Point2I maxExtent(0, 0);

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (!child)
         continue;

      const RectI& childBounds = child->getBounds();

      S32 right = (childBounds.point.x + mScrollOffset.x - mBounds.point.x) + childBounds.extent.x;
      S32 bottom = (childBounds.point.y + mScrollOffset.y - mBounds.point.y) + childBounds.extent.y;

      maxExtent.x = getMax(maxExtent.x, right);
      maxExtent.y = getMax(maxExtent.y, bottom);
   }

   // Trailing padding, so fully scrolling to the end leaves the same padding as at the start.
   maxExtent.x += (S32)(mResolvedStyle.padding.right * mResolvedUIScaleX);
   maxExtent.y += (S32)(mResolvedStyle.padding.bottom * mResolvedUIScaleY);

   if (maxExtent != mContentExtent)
   {
      mContentExtent = maxExtent;
      setArrangementDirty();
      reArrangeSelf();
   }

   clampScrollOffset();
}

void NewGuiScroll::clampScrollOffset()
{
   Point2I newOffset = mScrollOffset;

   if (mScrollBarMode == ScrollBar_Vertical || mScrollBarMode == ScrollBar_Both)
      newOffset.y = mClamp(newOffset.y, 0, getMax(0, mContentExtent.y - mBounds.extent.y));
   else
      newOffset.y = 0;

   if (mScrollBarMode == ScrollBar_Horizontal || mScrollBarMode == ScrollBar_Both)
      newOffset.x = mClamp(newOffset.x, 0, getMax(0, mContentExtent.x - mBounds.extent.x));
   else
      newOffset.x = 0;

   mScrollOffset = newOffset;
}

void NewGuiScroll::setScrollOffset(const Point2I& offset)
{
   Point2I old = mScrollOffset;
   mScrollOffset = offset;
   clampScrollOffset();

   if (mScrollOffset != old)
      setArrangementDirty();   // Children's on-screen position depends on GetClientRect()'s shift.
}

void NewGuiScroll::pageUp()
{
   Point2I newOffset = mScrollOffset;
   newOffset.y -= getMax(0, mBounds.extent.y - kPageOverlap);
   setScrollOffset(newOffset);
}

void NewGuiScroll::pageDown()
{
   Point2I newOffset = mScrollOffset;
   newOffset.y += getMax(0, mBounds.extent.y - kPageOverlap);
   setScrollOffset(newOffset);
}

void NewGuiScroll::pageLeft()
{
   Point2I newOffset = mScrollOffset;
   newOffset.x -= getMax(0, mBounds.extent.x - kPageOverlap);
   setScrollOffset(newOffset);
}

void NewGuiScroll::pageRight()
{
   Point2I newOffset = mScrollOffset;
   newOffset.x += getMax(0, mBounds.extent.x - kPageOverlap);
   setScrollOffset(newOffset);
}

void NewGuiScroll::renderChildControls(NewGuiRenderBatch* batch)
{
   if (!batch)
   {
      Parent::renderChildControls(batch);
      return;
   }

   batch->pushClipRect(mBounds);
   Parent::renderChildControls(batch);
   batch->popClipRect();
}

void NewGuiScroll::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   Parent::EmitDrawCommands(batch, bounds, style, layer);

   if (!batch)
      return;

   // Thumb fill: secondaryColor (defaults to textColor when unauthored - see
   // NewGuiResolvedStyle::secondaryColorAuthored)
   ColorI thumbColor(
      style.secondaryColor.red,
      style.secondaryColor.green,
      style.secondaryColor.blue,
      (U8)((F32)style.secondaryColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));

   const NewGuiSkinImage* trackImage = style.findSkinImage(StringTable->insert("track"));
   const NewGuiSkinImage* thumbImage = style.findSkinImage(StringTable->insert("thumb"));

   RectI vGutter = getVerticalGutterRect();
   if (vGutter.extent.y > 0)
   {
      RectI screenGutter(bounds.point + vGutter.point, vGutter.extent);

      if (trackImage && trackImage->hasImage())
         NewGuiStyleDrawSkinImage(batch, screenGutter, *trackImage, style.opacity, layer);

      RectI thumb = computeThumbRect(vGutter, true);
      if (thumb.extent.y > 0)
      {
         RectI screenThumb(bounds.point + vGutter.point + thumb.point, thumb.extent);
         if (thumbImage && thumbImage->hasImage())
            NewGuiStyleDrawSkinImage(batch, screenThumb, *thumbImage, style.opacity, layer);
         else
            batch->pushQuad(screenThumb, thumbColor, layer + 1);
      }
   }

   RectI hGutter = getHorizontalGutterRect();
   if (hGutter.extent.x > 0)
   {
      RectI screenGutter(bounds.point + hGutter.point, hGutter.extent);

      if (trackImage && trackImage->hasImage())
         NewGuiStyleDrawSkinImage(batch, screenGutter, *trackImage, style.opacity, layer);

      RectI thumb = computeThumbRect(hGutter, false);
      if (thumb.extent.x > 0)
      {
         RectI screenThumb(bounds.point + hGutter.point + thumb.point, thumb.extent);
         if (thumbImage && thumbImage->hasImage())
            NewGuiStyleDrawSkinImage(batch, screenThumb, *thumbImage, style.opacity, layer);
         else
            batch->pushQuad(screenThumb, thumbColor, layer + 1);
      }
   }
}

// Hit-tests localPoint against both thumb rects, push/popping the cursor ONLY on hover transition
// (never every Move while already hovering the same thumb, which would grow the cursor stack per pixel moved).
void NewGuiScroll::updateThumbHover(const Point2I& localPoint)
{
   // A drag already owns the cursor for its own duration - don't fight it.
   if (mThumbDragAxis != ScrollAxis_None || mDragInProgress)
      return;

   ScrollAxis newHoverAxis = ScrollAxis_None;

   RectI vGutter = getVerticalGutterRect();
   if (vGutter.extent.y > 0 && vGutter.pointInRect(localPoint))
   {
      RectI thumb = computeThumbRect(vGutter, true);
      Point2I localToGutter = localPoint - vGutter.point;
      if (thumb.extent.y > 0 && thumb.pointInRect(localToGutter))
         newHoverAxis = ScrollAxis_Vertical;
   }

   if (newHoverAxis == ScrollAxis_None)
   {
      RectI hGutter = getHorizontalGutterRect();
      if (hGutter.extent.x > 0 && hGutter.pointInRect(localPoint))
      {
         RectI thumb = computeThumbRect(hGutter, false);
         Point2I localToGutter = localPoint - hGutter.point;
         if (thumb.extent.x > 0 && thumb.pointInRect(localToGutter))
            newHoverAxis = ScrollAxis_Horizontal;
      }
   }

   if (newHoverAxis == mThumbHoverAxis)
      return;   // No transition.

   if (mThumbHoverAxis != ScrollAxis_None)
      popCursor();

   if (newHoverAxis != ScrollAxis_None)
      pushCursor(NewGuiCursorShape::Pointer);

   mThumbHoverAxis = newHoverAxis;
}

// Region hit-test: vertical gutter, then horizontal gutter, then content area. A gutter hit always
// claims the event; a content-area hit deliberately does not, so it can keep bubbling.
void NewGuiScroll::onMouseDown(NewGuiInputEvent& event)
{
   RectI vGutter = getVerticalGutterRect();
   if (vGutter.extent.y > 0 && vGutter.pointInRect(event.localPoint))
   {
      Point2I localToGutter = event.localPoint - vGutter.point;
      RectI thumb = computeThumbRect(vGutter, true);

      if (thumb.extent.y > 0 && thumb.pointInRect(localToGutter))
      {
         mThumbDragAxis = ScrollAxis_Vertical;
         mThumbDragStartAxisPos = event.localPoint.y;
         mThumbDragStartScrollOffset = mScrollOffset.y;

         pushCursor(NewGuiCursorShape::ResizeVertical);   // Popped in onMouseUp() when mThumbDragAxis clears.
      }
      else if (localToGutter.y < thumb.point.y)
         pageUp();
      else
         pageDown();

      event.handled = true;
      return;
   }

   RectI hGutter = getHorizontalGutterRect();
   if (hGutter.extent.x > 0 && hGutter.pointInRect(event.localPoint))
   {
      Point2I localToGutter = event.localPoint - hGutter.point;
      RectI thumb = computeThumbRect(hGutter, false);

      if (thumb.extent.x > 0 && thumb.pointInRect(localToGutter))
      {
         mThumbDragAxis = ScrollAxis_Horizontal;
         mThumbDragStartAxisPos = event.localPoint.x;
         mThumbDragStartScrollOffset = mScrollOffset.x;

         pushCursor(NewGuiCursorShape::ResizeHorizontal);
      }
      else if (localToGutter.x < thumb.point.x)
         pageLeft();
      else
         pageRight();

      event.handled = true;
      return;
   }

   // Content area - starts a content drag, but deliberately left unhandled so it can keep bubbling.
   mDragInProgress = true;
   mDragLastPoint = event.screenPoint;
}

void NewGuiScroll::onMouseUp(NewGuiInputEvent& event)
{
   if (mThumbDragAxis != ScrollAxis_None)
   {
      popCursor();   // Matches whichever push onMouseDown() made.

      mThumbDragAxis = ScrollAxis_None;

      // Re-check hover now, in case release happens with the pointer still over the thumb - avoids
      // a one-frame flash back to the default arrow before the next Move fires.
      updateThumbHover(event.localPoint);

      event.handled = true;
      return;
   }

   mDragInProgress = false;
}

// Handles the case updateThumbHover() can't: no Move ever fires once the pointer is outside this
// control's bounds, so a leave-while-hovering-the-thumb transition has nowhere else to be popped.
void NewGuiScroll::onMouseLeave(NewGuiInputEvent& event)
{
   if (mThumbHoverAxis != ScrollAxis_None)
   {
      popCursor();
      mThumbHoverAxis = ScrollAxis_None;
   }

   Parent::onMouseLeave(event);
}

// Three sources of scrolling, unified onto mScrollOffset: wheel, thumb drag, and content drag.
void NewGuiScroll::onInputEvent(NewGuiInputEvent& event)
{
   if (event.action == NewGuiInputAction::Wheel)
   {
      if (event.wheelAxis == 1 && (mScrollBarMode == ScrollBar_Vertical || mScrollBarMode == ScrollBar_Both))
      {
         Point2I newOffset = mScrollOffset;
         newOffset.y -= (S32)(event.wheelDelta * kPixelsPerWheelStep);
         setScrollOffset(newOffset);
         event.handled = true;
      }
      else if (event.wheelAxis == 0 && (mScrollBarMode == ScrollBar_Horizontal || mScrollBarMode == ScrollBar_Both))
      {
         Point2I newOffset = mScrollOffset;
         newOffset.x -= (S32)(event.wheelDelta * kPixelsPerWheelStep);
         setScrollOffset(newOffset);
         event.handled = true;
      }
      return;
   }

   if (event.action != NewGuiInputAction::Move)
      return;

   updateThumbHover(event.localPoint);

   if (mThumbDragAxis != ScrollAxis_None)
   {
      const bool vertical = (mThumbDragAxis == ScrollAxis_Vertical);

      RectI gutter = vertical ? getVerticalGutterRect() : getHorizontalGutterRect();
      RectI thumb = computeThumbRect(gutter, vertical);

      const S32 trackLength = vertical ? gutter.extent.y : gutter.extent.x;
      const S32 thumbLength = vertical ? thumb.extent.y : thumb.extent.x;
      const S32 viewportExtent = vertical ? mBounds.extent.y : mBounds.extent.x;
      const S32 contentExtent = vertical ? mContentExtent.y : mContentExtent.x;
      const S32 maxScrollOffset = getMax(0, contentExtent - viewportExtent);
      const S32 maxThumbTravel = trackLength - thumbLength;

      if (maxThumbTravel <= 0 || maxScrollOffset <= 0)
      {
         event.handled = true;
         return;
      }

      // Re-derives from the fixed drag-start position every Move, rather than accumulating
      // per-Move deltas, to avoid compounding rounding error over a long drag.
      S32 axisPos = vertical ? event.localPoint.y : event.localPoint.x;
      S32 totalDelta = axisPos - mThumbDragStartAxisPos;

      F32 contentPerThumbPixel = (F32)maxScrollOffset / (F32)maxThumbTravel;

      Point2I newOffset = mScrollOffset;
      S32 newAxisOffset = mThumbDragStartScrollOffset + (S32)((F32)totalDelta * contentPerThumbPixel);

      if (vertical)
         newOffset.y = newAxisOffset;
      else
         newOffset.x = newAxisOffset;

      setScrollOffset(newOffset);
      event.handled = true;
      return;
   }

   if (mDragInProgress)
   {
      Point2I delta = event.screenPoint - mDragLastPoint;
      mDragLastPoint = event.screenPoint;

      // Dragging content down (positive delta) reveals content above - decreases mScrollOffset,
      // opposite in sign from the wheel/thumb-drag case (those are already in "scroll the view" terms).
      Point2I newOffset = mScrollOffset;
      newOffset.x -= delta.x;
      newOffset.y -= delta.y;
      setScrollOffset(newOffset);
      event.handled = true;
   }
}
