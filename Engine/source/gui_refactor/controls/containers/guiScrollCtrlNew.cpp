//-----------------------------------------------------------------------------
// guiScrollCtrlNew.cpp
//-----------------------------------------------------------------------------

#include "gui_refactor/controls/containers/guiScrollCtrlNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiScrollCtrlNew);

//-----------------------------------------------------------------------------

ImplementEnumType(GuiScrollBarMode,
   "Which axes a GuiScrollCtrlNew allows scrolling/shows a bar on.\n\n")
{
   GuiScrollBarMode_None, "none",
      "No scrolling on either axis -- content is clipped to the viewport with no way to reach the overflow."
},
{ GuiScrollBarMode_Vertical, "vertical",
   "Vertical scrolling only (the common case: logs, lists)." },
{ GuiScrollBarMode_Horizontal, "horizontal",
   "Horizontal scrolling only." },
{ GuiScrollBarMode_Both, "both",
   "Both axes scroll independently, each with its own bar." },
   EndImplementEnumType;

//-----------------------------------------------------------------------------

namespace
{
   // Fallback ONLY -- used if neither mThumbStyle nor mStyle (its own
   // fallback, see mThumbStyle's doc comment) resolve a backgroundColor
   // at all, e.g. no style assigned yet during early construction. Real
   // authored appearances (including hover/pressed variance) come from
   // the style system now, not these constants.
   const ColorI kFallbackTrackColor(30, 30, 34, 200);
   const ColorI kFallbackThumbColor(90, 90, 98, 255);

   const S32 kDefaultScrollBarThickness = 14;
   const S32 kDefaultWheelStep = 40;
   const S32 kMinThumbLength = 20; ///< Floor so a very tall/wide content area doesn't shrink the thumb to an unclickable sliver.
}

//-----------------------------------------------------------------------------

GuiScrollCtrlNew::GuiScrollCtrlNew()
   : mThumbStyle(NULL),
   mScrollBarMode(GuiScrollBarMode_Vertical),
   mScrollOffsetX(0),
   mScrollOffsetY(0),
   mMaxScrollX(0),
   mMaxScrollY(0),
   mContentWidth(0),
   mContentHeight(0),
   mContentOrigin(0, 0),
   mScrollBarThickness(kDefaultScrollBarThickness),
   mWheelStep(kDefaultWheelStep),
   mDraggingVThumb(false),
   mDraggingHThumb(false),
   mThumbDragGrabOffset(0),
   mScrollRangesDirty(true)
{
   setCanHit(true);
   setCapturesInput(true); // clicks on the scrollbar shouldn't fall through to whatever's behind this control
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::initPersistFields()
{
   docsURL;
   addGroup("Scroll");

   addField("scrollBarMode", TYPEID< GuiScrollBarMode >(), Offset(mScrollBarMode, GuiScrollCtrlNew),
      "Which axes scroll -- \"none\", \"vertical\" (default), \"horizontal\", or \"both\".");

   addField("scrollBarThickness", TypeS32, Offset(mScrollBarThickness, GuiScrollCtrlNew),
      "Track/thumb thickness in logical pixels. Default 14.");

   addField("wheelStep", TypeS32, Offset(mWheelStep, GuiScrollCtrlNew),
      "Logical pixels scrolled per mouse-wheel notch. Default 40.");

   addProtectedField("thumbStyle", TYPEID< GuiStyle >(), Offset(mThumbStyle, GuiScrollCtrlNew), &setThumbStyleProt, &defaultProtectedGetFn,
      "Style resolved for the scrollbar thumb specifically -- gets its own hover/active state cascade "
      "(distinct from this control's own hover/active) so a dragged or hovered thumb can look different "
      "from an idle one. Falls back to this control's own 'style' if unset.");

   endGroup("Scroll");

   Parent::initPersistFields();
}

void GuiScrollCtrlNew::setThumbStyle(GuiStyle* style)
{
   if (style == mThumbStyle)
      return;

   bool skipAwaken = false;

   if (mThumbStyle == NULL)
      skipAwaken = true;

   if (mAwake && mThumbStyle)
      mThumbStyle->decLoadCount();

   if (mThumbStyle)
      clearNotify(mThumbStyle);

   mThumbStyle = style;

   if (mAwake && mThumbStyle)
      mThumbStyle->incLoadCount();

   if (mThumbStyle)
      deleteNotify(mThumbStyle);

   setUpdate();

   // Same re-cycle-through-sleep/awaken rationale as
   // GuiControlNew::setStyle(): a live style swap (not the first-ever
   // assignment, which is what skipAwaken guards against) needs every
   // other awake-dependent bit of state -- not just the load count -- to
   // re-derive cleanly, and sleep()/awaken() is the one place that
   // already happens correctly.
   if (mAwake && !skipAwaken)
   {
      sleep();

      if (!Sim::isShuttingDown())
         awaken();
   }
}

//-----------------------------------------------------------------------------

bool GuiScrollCtrlNew::setThumbStyleProt(void* object, const char* index, const char* data)
{
   GuiScrollCtrlNew* ctrl = static_cast<GuiScrollCtrlNew*>(object);
   GuiStyle* style = dynamic_cast<GuiStyle*>(Sim::findObject(data));
   if (style == NULL)
      return false;

   ctrl->setThumbStyle(style);
   return false;
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onRemove()
{
   // mThumbStyle must be released via the real setThumbStyle(NULL) path
   // (not just nulled) so its decLoadCount()/clearNotify() actually run.
   // This runs BEFORE Parent::onRemove() (which is what puts this control
   // to sleep), so mAwake is still true here and setThumbStyle()'s
   // decrement fires correctly. Parent::onRemove()'s subsequent sleep()
   // call reaches this class's own onSleep() override too, but by then
   // mThumbStyle is already NULL, so that decrement is a safe no-op --
   // see onSleep()'s comment.
   if (mThumbStyle)
      setThumbStyle(NULL);

   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onDeleteNotify(SimObject* object)
{
   if (object == mThumbStyle)
   {
      GuiStyle* fallback;
      Sim::findObject("GuiDefaultStyle", fallback);

      if (fallback == mThumbStyle)
         mThumbStyle = NULL;
      else
         setThumbStyle(fallback);
   }

   Parent::onDeleteNotify(object);
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::_updateScrollRangesIfDirty()
{
   if (mScrollRangesDirty)
      _updateScrollRanges();
}

//-----------------------------------------------------------------------------

bool GuiScrollCtrlNew::onWake()
{
   if (!Parent::onWake())
      return false;

   if (mThumbStyle)
      mThumbStyle->incLoadCount();

   _updateScrollRanges();
   return true;
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onSleep()
{
   if (mThumbStyle)
      mThumbStyle->decLoadCount();

   Parent::onSleep();
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onChildAdded(GuiControlNew* child)
{
   Parent::onChildAdded(child);

   child->setAllowOverflow(true);

   ChildAnchor anchor;
   anchor.child = child;
   anchor.authoredPosition = child->getPosition();
   mChildAnchors.push_back(anchor);

   mScrollRangesDirty = true;
}

void GuiScrollCtrlNew::onChildRemoved(GuiControlNew* child)
{
   Parent::onChildRemoved(child);

   for (U32 i = 0; i < mChildAnchors.size(); i++)
   {
      if (mChildAnchors[i].child == child)
      {
         mChildAnchors.erase(i);
         break;
      }
   }

   mScrollRangesDirty = true;
}

//-----------------------------------------------------------------------------

bool GuiScrollCtrlNew::resize(const Point2I& newPosition, const Point2I& newExtent)
{
   const bool result = Parent::resize(newPosition, newExtent);

   mScrollRangesDirty = true;
   return result;
}

//-----------------------------------------------------------------------------

Point2I GuiScrollCtrlNew::getClientExtent() const
{
   // Reserves each enabled axis's scrollbar gutter 
   const Point2I fullExtent = getExtent();

   const S32 reservedV = (mScrollBarMode & GuiScrollBarMode_Vertical) ? mScrollBarThickness : 0;
   const S32 reservedH = (mScrollBarMode & GuiScrollBarMode_Horizontal) ? mScrollBarThickness : 0;

   return Point2I(getMax(fullExtent.x - reservedV, 0), getMax(fullExtent.y - reservedH, 0));
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::_updateScrollRanges()
{
   mScrollRangesDirty = false;

   // Re-derive every child's authored (un-scrolled) anchor FIRST
   {
      const S32 savedScrollOffsetX = mScrollOffsetX;
      const S32 savedScrollOffsetY = mScrollOffsetY;
      mScrollOffsetX = 0;
      mScrollOffsetY = 0;

      for (U32 i = 0; i < mChildAnchors.size(); i++)
      {
         GuiControlNew* child = mChildAnchors[i].child;
         child->markLayoutDirty();
         mChildAnchors[i].authoredPosition = child->getPosition();
      }

      mScrollOffsetX = savedScrollOffsetX;
      mScrollOffsetY = savedScrollOffsetY;
   }

   // Content extent is the UNION of every child's authored rect (see
   // mChildAnchors)
   if (mChildAnchors.empty())
   {
      mContentOrigin = Point2I(0, 0);
      mContentWidth = 0;
      mContentHeight = 0;
   }
   else
   {
      Point2I unionMin = mChildAnchors[0].authoredPosition;
      Point2I unionMax = unionMin + mChildAnchors[0].child->getExtent();

      for (U32 i = 1; i < mChildAnchors.size(); i++)
      {
         const Point2I childMin = mChildAnchors[i].authoredPosition;
         const Point2I childMax = childMin + mChildAnchors[i].child->getExtent();

         unionMin.x = getMin(unionMin.x, childMin.x);
         unionMin.y = getMin(unionMin.y, childMin.y);
         unionMax.x = getMax(unionMax.x, childMax.x);
         unionMax.y = getMax(unionMax.y, childMax.y);
      }

      mContentOrigin = unionMin;
      mContentWidth = unionMax.x - unionMin.x;
      mContentHeight = unionMax.y - unionMin.y;
   }

   const Point2I fullExtent = getExtent();

   const bool wantsV = (mScrollBarMode & GuiScrollBarMode_Vertical) && mContentHeight > fullExtent.y;
   const bool wantsH = (mScrollBarMode & GuiScrollBarMode_Horizontal) && mContentWidth > fullExtent.x;

   const S32 viewportWidth = fullExtent.x - (wantsV ? mScrollBarThickness : 0);
   const S32 viewportHeight = fullExtent.y - (wantsH ? mScrollBarThickness : 0);

   mMaxScrollX = (mScrollBarMode & GuiScrollBarMode_Horizontal) ? getMax(0, mContentWidth - viewportWidth) : 0;
   mMaxScrollY = (mScrollBarMode & GuiScrollBarMode_Vertical) ? getMax(0, mContentHeight - viewportHeight) : 0;

   mScrollOffsetX = mClamp(mScrollOffsetX, 0, mMaxScrollX);
   mScrollOffsetY = mClamp(mScrollOffsetY, 0, mMaxScrollY);

   _applyContentPosition();
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::_applyContentPosition()
{
   for (U32 i = 0; i < mChildAnchors.size(); i++)
   {
      const Point2I newPosition(
         mChildAnchors[i].authoredPosition.x - mContentOrigin.x - mScrollOffsetX,
         mChildAnchors[i].authoredPosition.y - mContentOrigin.y - mScrollOffsetY);

      mChildAnchors[i].child->setPosition(newPosition);
   }
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::setScrollOffset(S32 x, S32 y)
{
   const S32 newX = mClamp(x, 0, mMaxScrollX);
   const S32 newY = mClamp(y, 0, mMaxScrollY);

   if (newX == mScrollOffsetX && newY == mScrollOffsetY)
      return;

   mScrollOffsetX = newX;
   mScrollOffsetY = newY;
   _applyContentPosition();
   setUpdate();
}

//-----------------------------------------------------------------------------
//    Track/thumb geometry
//-----------------------------------------------------------------------------

RectI GuiScrollCtrlNew::_getVTrackDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!shouldShowVBar())
      return RectI(Point2I(0, 0), Point2I(0, 0));

   // Device-pixel thickness -- mScrollBarThickness is authored in
   // logical units like every other layout field
   const Point2I logicalExtent = getExtent();
   const F32 scaleX = logicalExtent.x > 0 ? (F32)ctrlDeviceRect.extent.x / (F32)logicalExtent.x : 1.0f;
   const S32 barThicknessDevice = (S32)(mScrollBarThickness * scaleX);

   return RectI(Point2I(ctrlDeviceRect.point.x + ctrlDeviceRect.extent.x - barThicknessDevice, ctrlDeviceRect.point.y),
      Point2I(barThicknessDevice, ctrlDeviceRect.extent.y - (shouldShowHBar() ? barThicknessDevice : 0)));
}

//-----------------------------------------------------------------------------

RectI GuiScrollCtrlNew::_getHTrackDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!shouldShowHBar())
      return RectI(Point2I(0, 0), Point2I(0, 0));

   const Point2I logicalExtent = getExtent();
   const F32 scaleY = logicalExtent.y > 0 ? (F32)ctrlDeviceRect.extent.y / (F32)logicalExtent.y : 1.0f;
   const S32 barThicknessDevice = (S32)(mScrollBarThickness * scaleY);

   return RectI(Point2I(ctrlDeviceRect.point.x, ctrlDeviceRect.point.y + ctrlDeviceRect.extent.y - barThicknessDevice),
      Point2I(ctrlDeviceRect.extent.x - (shouldShowVBar() ? barThicknessDevice : 0), barThicknessDevice));
}

//-----------------------------------------------------------------------------

RectI GuiScrollCtrlNew::_getVThumbDeviceRect(const RectI& ctrlDeviceRect) const
{
   const RectI track = _getVTrackDeviceRect(ctrlDeviceRect);
   if (track.extent.y <= 0 || mContentHeight <= 0)
      return track;

   // Standard proportional-thumb formula: thumb length is the visible
   // fraction of total content, thumb position is the scrolled fraction
   // of the remaining (content - visible) range. viewportHeight is
   // recovered from mContentHeight - mMaxScrollY rather than re-deriving
   // it from the track rect, since that's the exact same value
   // _updateScrollRanges() used to compute mMaxScrollY in the first
   // place (avoids the two ever silently disagreeing).
   const S32 viewportHeight = mContentHeight - mMaxScrollY;
   const F32 visibleFraction = mClampF((F32)viewportHeight / (F32)mContentHeight, 0.0f, 1.0f);

   S32 thumbLength = (S32)(track.extent.y * visibleFraction);
   thumbLength = getMax(thumbLength, getMin(kMinThumbLength, track.extent.y));

   const S32 thumbTravel = track.extent.y - thumbLength;
   const F32 scrollFraction = mMaxScrollY > 0 ? (F32)mScrollOffsetY / (F32)mMaxScrollY : 0.0f;
   const S32 thumbOffset = (S32)(thumbTravel * scrollFraction);

   return RectI(Point2I(track.point.x, track.point.y + thumbOffset), Point2I(track.extent.x, thumbLength));
}

//-----------------------------------------------------------------------------

RectI GuiScrollCtrlNew::_getHThumbDeviceRect(const RectI& ctrlDeviceRect) const
{
   const RectI track = _getHTrackDeviceRect(ctrlDeviceRect);
   if (track.extent.x <= 0 || mContentWidth <= 0)
      return track;

   const S32 viewportWidth = mContentWidth - mMaxScrollX;
   const F32 visibleFraction = mClampF((F32)viewportWidth / (F32)mContentWidth, 0.0f, 1.0f);

   S32 thumbLength = (S32)(track.extent.x * visibleFraction);
   thumbLength = getMax(thumbLength, getMin(kMinThumbLength, track.extent.x));

   const S32 thumbTravel = track.extent.x - thumbLength;
   const F32 scrollFraction = mMaxScrollX > 0 ? (F32)mScrollOffsetX / (F32)mMaxScrollX : 0.0f;
   const S32 thumbOffset = (S32)(thumbTravel * scrollFraction);

   return RectI(Point2I(track.point.x + thumbOffset, track.point.y), Point2I(thumbLength, track.extent.y));
}

//-----------------------------------------------------------------------------
//    Thumb style-state derivation
//-----------------------------------------------------------------------------

U32 GuiScrollCtrlNew::_getVThumbStyleStateMask(const RectI& ctrlDeviceRect)
{
   U32 mask = 0;

   // mDraggingVThumb IS legitimately consulted here, alongside the
   // on-demand hover hit-test below
   if (mDraggingVThumb)
      mask |= GuiStyle::bit(GuiStyleState::Active);

   GuiCanvasNew* root = getRoot();
   if (root && root->getMouseControl() == this)
   {
      // getCursorPosLocal() is misleadingly named
      const Point2I logicalMouse = globalToLocalCoord(root->deviceToLogicalPoint(root->getCursorPosLocal()));
      const Point2I logicalExtent = getExtent();
      const F32 scaleX = logicalExtent.x > 0 ? (F32)ctrlDeviceRect.extent.x / (F32)logicalExtent.x : 1.0f;
      const F32 scaleY = logicalExtent.y > 0 ? (F32)ctrlDeviceRect.extent.y / (F32)logicalExtent.y : 1.0f;
      const Point2I deviceMouse((S32)(logicalMouse.x * scaleX), (S32)(logicalMouse.y * scaleY));

      if (_getVThumbDeviceRect(ctrlDeviceRect).pointInRect(deviceMouse))
         mask |= GuiStyle::bit(GuiStyleState::Hover);
   }

   return mask;
}

//-----------------------------------------------------------------------------

U32 GuiScrollCtrlNew::_getHThumbStyleStateMask(const RectI& ctrlDeviceRect)
{
   U32 mask = 0;

   if (mDraggingHThumb)
      mask |= GuiStyle::bit(GuiStyleState::Active);

   GuiCanvasNew* root = getRoot();
   if (root && root->getMouseControl() == this)
   {
      // See _getVThumbStyleStateMask()'s equivalent comment on why this
      // goes through deviceToLogicalPoint() rather than using
      // getCursorPosLocal()'s result directly.
      const Point2I logicalMouse = globalToLocalCoord(root->deviceToLogicalPoint(root->getCursorPosLocal()));
      const Point2I logicalExtent = getExtent();
      const F32 scaleX = logicalExtent.x > 0 ? (F32)ctrlDeviceRect.extent.x / (F32)logicalExtent.x : 1.0f;
      const F32 scaleY = logicalExtent.y > 0 ? (F32)ctrlDeviceRect.extent.y / (F32)logicalExtent.y : 1.0f;
      const Point2I deviceMouse((S32)(logicalMouse.x * scaleX), (S32)(logicalMouse.y * scaleY));

      if (_getHThumbDeviceRect(ctrlDeviceRect).pointInRect(deviceMouse))
         mask |= GuiStyle::bit(GuiStyleState::Hover);
   }

   return mask;
}

//-----------------------------------------------------------------------------
//    Input
//-----------------------------------------------------------------------------

bool GuiScrollCtrlNew::onKeyDown(const GuiEvent& event)
{
   _updateScrollRangesIfDirty();

   if (event.keyCode == KEY_UP)
   {
      setScrollOffset(mScrollOffsetX, mScrollOffsetY - mWheelStep);
      return true;
   }
   if (event.keyCode == KEY_DOWN)
   {
      setScrollOffset(mScrollOffsetX, mScrollOffsetY + mWheelStep);
      return true;
   }

   return Parent::onKeyDown(event);
}

//-----------------------------------------------------------------------------

bool GuiScrollCtrlNew::onMouseWheelUp(const GuiEvent& event)
{
   _updateScrollRangesIfDirty();
   setScrollOffset(mScrollOffsetX, mScrollOffsetY - mWheelStep);
   return true;
}

bool GuiScrollCtrlNew::onMouseWheelDown(const GuiEvent& event)
{
   _updateScrollRangesIfDirty();
   setScrollOffset(mScrollOffsetX, mScrollOffsetY + mWheelStep);
   return true;
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onMouseDown(const GuiEvent& event)
{
   _updateScrollRangesIfDirty();

   // event.mousePoint is root/canvas-relative logical space.
   const Point2I logicalLocal = globalToLocalCoord(event.mousePoint);
   const RectI deviceBounds = getDeviceBounds();
   const Point2I logicalExtent = getExtent();
   const F32 scaleX = logicalExtent.x > 0 ? (F32)deviceBounds.extent.x / (F32)logicalExtent.x : 1.0f;
   const F32 scaleY = logicalExtent.y > 0 ? (F32)deviceBounds.extent.y / (F32)logicalExtent.y : 1.0f;
   const Point2I deviceLocal((S32)(logicalLocal.x * scaleX), (S32)(logicalLocal.y * scaleY));
   const RectI localDeviceRect(Point2I(0, 0), deviceBounds.extent);

   const RectI vThumb = _getVThumbDeviceRect(localDeviceRect);
   if (shouldShowVBar() && vThumb.pointInRect(deviceLocal))
   {
      mDraggingVThumb = true;
      mThumbDragGrabOffset = deviceLocal.y - vThumb.point.y;
      mouseLock();
      setUpdate();
      return;
   }

   const RectI hThumb = _getHThumbDeviceRect(localDeviceRect);
   if (shouldShowHBar() && hThumb.pointInRect(deviceLocal))
   {
      mDraggingHThumb = true;
      mThumbDragGrabOffset = deviceLocal.x - hThumb.point.x;
      mouseLock();
      setUpdate();
      return;
   }

   // Track click outside the thumb -- page jump, standard scrollbar
   // behavior (click above/below the thumb pages toward that direction
   // rather than snapping the thumb straight to the click point, which
   // is what dragging is for).
   const RectI vTrack = _getVTrackDeviceRect(localDeviceRect);
   if (shouldShowVBar() && vTrack.pointInRect(deviceLocal))
   {
      const S32 viewportHeight = mContentHeight - mMaxScrollY;
      const S32 pageStep = getMax(mWheelStep, viewportHeight - mWheelStep);
      setScrollOffset(mScrollOffsetX, mScrollOffsetY + (deviceLocal.y < vThumb.point.y ? -pageStep : pageStep));
      return;
   }

   const RectI hTrack = _getHTrackDeviceRect(localDeviceRect);
   if (shouldShowHBar() && hTrack.pointInRect(deviceLocal))
   {
      const S32 viewportWidth = mContentWidth - mMaxScrollX;
      const S32 pageStep = getMax(mWheelStep, viewportWidth - mWheelStep);
      setScrollOffset(mScrollOffsetX + (deviceLocal.x < hThumb.point.x ? -pageStep : pageStep), mScrollOffsetY);
      return;
   }

   Parent::onMouseDown(event);
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onMouseDragged(const GuiEvent& event)
{
   if (!mDraggingVThumb && !mDraggingHThumb)
   {
      Parent::onMouseDragged(event);
      return;
   }

   const RectI deviceBounds = getDeviceBounds();
   const Point2I logicalLocal = globalToLocalCoord(event.mousePoint);
   const Point2I logicalExtent = getExtent();
   const F32 scaleX = logicalExtent.x > 0 ? (F32)deviceBounds.extent.x / (F32)logicalExtent.x : 1.0f;
   const F32 scaleY = logicalExtent.y > 0 ? (F32)deviceBounds.extent.y / (F32)logicalExtent.y : 1.0f;
   const Point2I deviceLocal((S32)(logicalLocal.x * scaleX), (S32)(logicalLocal.y * scaleY));
   const RectI localDeviceRect(Point2I(0, 0), deviceBounds.extent);

   if (mDraggingVThumb)
   {
      const RectI track = _getVTrackDeviceRect(localDeviceRect);
      const RectI thumb = _getVThumbDeviceRect(localDeviceRect);
      const S32 thumbTravel = track.extent.y - thumb.extent.y;
      if (thumbTravel <= 0)
         return;

      const S32 desiredThumbTop = deviceLocal.y - mThumbDragGrabOffset - track.point.y;
      const F32 scrollFraction = mClampF((F32)desiredThumbTop / (F32)thumbTravel, 0.0f, 1.0f);
      setScrollOffset(mScrollOffsetX, (S32)(mMaxScrollY * scrollFraction));
   }
   else // mDraggingHThumb
   {
      const RectI track = _getHTrackDeviceRect(localDeviceRect);
      const RectI thumb = _getHThumbDeviceRect(localDeviceRect);
      const S32 thumbTravel = track.extent.x - thumb.extent.x;
      if (thumbTravel <= 0)
         return;

      const S32 desiredThumbLeft = deviceLocal.x - mThumbDragGrabOffset - track.point.x;
      const F32 scrollFraction = mClampF((F32)desiredThumbLeft / (F32)thumbTravel, 0.0f, 1.0f);
      setScrollOffset((S32)(mMaxScrollX * scrollFraction), mScrollOffsetY);
   }
}

//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onMouseUp(const GuiEvent& event)
{
   if (mDraggingVThumb || mDraggingHThumb)
   {
      mDraggingVThumb = false;
      mDraggingHThumb = false;
      if (isMouseLocked())
         mouseUnlock();
      setUpdate();
      return;
   }

   Parent::onMouseUp(event);
}

//-----------------------------------------------------------------------------
//    Render
//-----------------------------------------------------------------------------

void GuiScrollCtrlNew::onRender(Point2I offset, const RectI& updateRect)
{
   // Must happen before anything below reads mMaxScrollX/Y, mContentWidth/
   // Height, or calls any of the track/thumb rect helpers -- see
   // mScrollRangesDirty's doc comment.
   _updateScrollRangesIfDirty();

   const RectI ctrlRect(offset, getDeviceBounds().extent);
   const GuiStyleProperties style = resolveStyle();

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;
   GuiRenderBatch& batch = root->getRenderBatch();

   if (style.backgroundColor.isSet())
      batch.pushQuad(ctrlRect, style.backgroundColor.mValue, getRenderLayer());

   const S32 vBarDevice = shouldShowVBar() ? _getVTrackDeviceRect(ctrlRect).extent.x : 0;
   const S32 hBarDevice = shouldShowHBar() ? _getHTrackDeviceRect(ctrlRect).extent.y : 0;
   const RectI viewportDeviceRect(ctrlRect.point, Point2I(ctrlRect.extent.x - vBarDevice, ctrlRect.extent.y - hBarDevice));

   RectI clippedUpdateRect = updateRect;
   if (clippedUpdateRect.intersect(viewportDeviceRect))
   {
      const RectI savedClipRect = GFX->getClipRect();
      GFX->setClipRect(clippedUpdateRect);

      batch.pushClipRect(clippedUpdateRect);
      renderChildControls(offset, clippedUpdateRect);
      batch.popClipRect();

      GFX->setClipRect(savedClipRect);
   }

   const S32 barLayer = getRenderLayer() + 2;

   GuiStyle* thumbStyleRef = mThumbStyle ? mThumbStyle : mStyle;

   if (shouldShowVBar())
   {
      const RectI track = _getVTrackDeviceRect(ctrlRect);
      const RectI thumb = _getVThumbDeviceRect(ctrlRect);

      const ColorI trackColor = style.backgroundColor.isSet() ? style.backgroundColor.mValue : kFallbackTrackColor;
      batch.pushQuad(track, trackColor, barLayer);

      if (thumbStyleRef)
      {
         const GuiStyleProperties thumbStyle = thumbStyleRef->resolve(_getVThumbStyleStateMask(ctrlRect));
         const ColorI thumbColor = thumbStyle.backgroundColor.isSet() ? thumbStyle.backgroundColor.mValue : kFallbackThumbColor;
         batch.pushQuad(thumb, thumbColor, barLayer);

         if (thumbStyle.borderWidth.isSet() && thumbStyle.borderWidth.mValue > 0 && thumbStyle.borderColor.isSet())
         {
            const S32 bw = thumbStyle.borderWidth.mValue;
            const ColorI& bc = thumbStyle.borderColor.mValue;
            batch.pushQuad(RectI(thumb.point, Point2I(thumb.extent.x, bw)), bc, barLayer);
            batch.pushQuad(RectI(Point2I(thumb.point.x, thumb.point.y + thumb.extent.y - bw), Point2I(thumb.extent.x, bw)), bc, barLayer);
            batch.pushQuad(RectI(thumb.point, Point2I(bw, thumb.extent.y)), bc, barLayer);
            batch.pushQuad(RectI(Point2I(thumb.point.x + thumb.extent.x - bw, thumb.point.y), Point2I(bw, thumb.extent.y)), bc, barLayer);
         }
      }
      else
      {
         batch.pushQuad(thumb, kFallbackThumbColor, barLayer);
      }
   }

   if (shouldShowHBar())
   {
      const RectI track = _getHTrackDeviceRect(ctrlRect);
      const RectI thumb = _getHThumbDeviceRect(ctrlRect);

      const ColorI trackColor = style.backgroundColor.isSet() ? style.backgroundColor.mValue : kFallbackTrackColor;
      batch.pushQuad(track, trackColor, barLayer);

      if (thumbStyleRef)
      {
         const GuiStyleProperties thumbStyle = thumbStyleRef->resolve(_getHThumbStyleStateMask(ctrlRect));
         const ColorI thumbColor = thumbStyle.backgroundColor.isSet() ? thumbStyle.backgroundColor.mValue : kFallbackThumbColor;
         batch.pushQuad(thumb, thumbColor, barLayer);

         if (thumbStyle.borderWidth.isSet() && thumbStyle.borderWidth.mValue > 0 && thumbStyle.borderColor.isSet())
         {
            const S32 bw = thumbStyle.borderWidth.mValue;
            const ColorI& bc = thumbStyle.borderColor.mValue;
            batch.pushQuad(RectI(thumb.point, Point2I(thumb.extent.x, bw)), bc, barLayer);
            batch.pushQuad(RectI(Point2I(thumb.point.x, thumb.point.y + thumb.extent.y - bw), Point2I(thumb.extent.x, bw)), bc, barLayer);
            batch.pushQuad(RectI(thumb.point, Point2I(bw, thumb.extent.y)), bc, barLayer);
            batch.pushQuad(RectI(Point2I(thumb.point.x + thumb.extent.x - bw, thumb.point.y), Point2I(bw, thumb.extent.y)), bc, barLayer);
         }
      }
      else
      {
         batch.pushQuad(thumb, kFallbackThumbColor, barLayer);
      }
   }

   // NOTE: renderChildControls() already ran above (inside the viewport
   // clip) -- do not call it again here the way a plain GuiControlNew::
   // onRender() would at the end (see e.g. GuiLabelCtrlNew::onRender()'s
   // equivalent trailing call). Doing so would render every content
   // child a second time, unclipped, on top of the scrollbar just drawn.
}
