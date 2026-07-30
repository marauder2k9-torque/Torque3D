//-----------------------------------------------------------------------------
// guiWindowCtrlNew.cpp
//-----------------------------------------------------------------------------

#include "gui_refactor/controls/containers/guiWindowCtrlNew.h"
#include "gui_refactor/core/guiCanvasNew.h"
#include "gui_refactor/core/guiRenderBatch.h"
#include "console/consoleTypes.h"

IMPLEMENT_CONOBJECT(GuiWindowCtrlNew);

//-----------------------------------------------------------------------------

IMPLEMENT_CALLBACK(GuiWindowCtrlNew, onWindowClose, void, (), (),
   "Called when the window's close button is clicked, before the default "
   "hide-on-close behavior runs. Return value is ignored -- to suppress the "
   "default hide, override onWindowClose() in a C++ subclass instead; the "
   "script callback is notification-only, same as onAction() elsewhere.");

IMPLEMENT_CALLBACK(GuiWindowCtrlNew, onWindowMinimize, void, (), (),
   "Called after the window collapses to just its title bar via the minimize button "
   "or a setMinimized(true) call.");

IMPLEMENT_CALLBACK(GuiWindowCtrlNew, onWindowMaximize, void, (), (),
   "Called after the window grows to fill its parent via the maximize button "
   "or a setMaximized(true) call.");

IMPLEMENT_CALLBACK(GuiWindowCtrlNew, onWindowRestore, void, (), (),
   "Called after the window returns to its pre-minimize/pre-maximize bounds, "
   "whichever state it's leaving.");

//-----------------------------------------------------------------------------

namespace
{
   // Fallback ONLY -- used if neither mTitleBarStyle nor mStyle (its own
   // fallback) resolve a backgroundColor/textColor at all. Real authored
   // appearances come from the style system -- see mTitleBarStyle's doc
   // comment.
   const ColorI kFallbackTitleBarColor(45, 45, 52, 255);
   const ColorI kFallbackCloseButtonColor(90, 45, 45, 255);
   const ColorI kFallbackCloseGlyphColor(230, 230, 230, 255);
   const ColorI kFallbackBackgroundColor(30, 30, 34, 235);
   const ColorI kFallbackChromeButtonColor(55, 55, 62, 255); ///< Minimize/maximize buttons -- less alarming than the close button's reddish tint since they're non-destructive.

   const S32 kDefaultTitleBarHeight = 24;
   const S32 kCloseButtonMargin = 4; ///< Gap between the close button and the title bar's top/right/bottom edges.
   const S32 kResizeGripLogicalSize = 14; ///< Logical-unit side length of the bottom-right resize grip hit region.
   const ColorI kFallbackResizeGripColor(120, 120, 130, 200);
   const ColorI kFallbackButtonGlyphColor(230, 230, 230, 255); ///< Used for minimize/maximize glyphs -- close's own glyph keeps its existing kFallbackCloseGlyphColor name unchanged below.
}

//-----------------------------------------------------------------------------

GuiWindowCtrlNew::GuiWindowCtrlNew()
   : mShowTitleBar(true),
   mTitleBarHeight(kDefaultTitleBarHeight),
   mMovable(true),
   mShowCloseButton(true),
   mTitleBarStyle(NULL),
   mDraggingTitleBar(false),
   mDragGrabOffsetDevice(0, 0),
   mMinimizable(true),
   mMaximizable(true),
   mResizable(true),
   mMinimized(false),
   mMaximized(false),
   mRestoreBounds(0, 0, 0, 0),
   mDraggingResize(false),
   mResizeGrabOffsetDevice(0, 0)
{
   setCanHit(true);
   setCapturesInput(true); // clicks on the title bar/chrome shouldn't fall through to whatever's behind this window

   mGuiText.mAlignH = GuiTextAlignHorizontal_Left;
   mGuiText.mAlignV = GuiTextAlignVertical_Middle;
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::initPersistFields()
{
   docsURL;
   addGroup("Window");

   addField("showTitleBar", TypeBool, Offset(mShowTitleBar, GuiWindowCtrlNew),
      "If false, no title bar is drawn -- just a bordered/filled box around the children. "
      "Default true.");

   addProtectedField("text", TypeRealString, Offset(mText, GuiWindowCtrlNew), &setTextProt, &defaultProtectedGetFn,
      "Caption text shown in the title bar. Meaningless if showTitleBar is false.");

   addField("titleBarHeight", TypeS32, Offset(mTitleBarHeight, GuiWindowCtrlNew),
      "Fixed height of the title bar, logical pixels. Default 24.");

   addField("movable", TypeBool, Offset(mMovable, GuiWindowCtrlNew),
      "If true (and showTitleBar), dragging the title bar moves the window. Default true.");

   addField("showCloseButton", TypeBool, Offset(mShowCloseButton, GuiWindowCtrlNew),
      "If true (and showTitleBar), a close button is drawn in the title bar. Default true.");

   addField("minimizable", TypeBool, Offset(mMinimizable, GuiWindowCtrlNew),
      "If true (and showTitleBar), a minimize button is drawn in the title bar. Default true.");

   addField("maximizable", TypeBool, Offset(mMaximizable, GuiWindowCtrlNew),
      "If true (and showTitleBar), a maximize button is drawn in the title bar. Default true.");

   addField("resizable", TypeBool, Offset(mResizable, GuiWindowCtrlNew),
      "If true, a grip in the bottom-right corner can be dragged to resize the window. "
      "Default true.");

   addProtectedField("titleBarStyle", TYPEID< GuiStyle >(), Offset(mTitleBarStyle, GuiWindowCtrlNew), &setTitleBarStyleProt, &defaultProtectedGetFn,
      "Style resolved for the title bar fill/text and close button -- gets its own Active state "
      "while being dragged (distinct from this control's own state cascade). Falls back to this "
      "control's own 'style' if unset.");

   endGroup("Window");

   Parent::initPersistFields();
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::setTitleBarStyle(GuiStyle* style)
{
   if (style == mTitleBarStyle)
      return;

   bool skipAwaken = false;

   if (mTitleBarStyle == NULL)
      skipAwaken = true;

   if (mAwake && mTitleBarStyle)
      mTitleBarStyle->decLoadCount();

   if (mTitleBarStyle)
      clearNotify(mTitleBarStyle);

   mTitleBarStyle = style;

   if (mAwake && mTitleBarStyle)
      mTitleBarStyle->incLoadCount();

   if (mTitleBarStyle)
      deleteNotify(mTitleBarStyle);

   setUpdate();

   // Same re-cycle-through-sleep/awaken rationale as GuiScrollCtrlNew::
   // setThumbStyle() -- a live style swap needs every other
   // awake-dependent bit of state to re-derive cleanly.
   if (mAwake && !skipAwaken)
   {
      sleep();

      if (!Sim::isShuttingDown())
         awaken();
   }
}

//-----------------------------------------------------------------------------

bool GuiWindowCtrlNew::setTitleBarStyleProt(void* object, const char* index, const char* data)
{
   GuiWindowCtrlNew* ctrl = static_cast<GuiWindowCtrlNew*>(object);
   GuiStyle* style = dynamic_cast<GuiStyle*>(Sim::findObject(data));
   if (style == NULL)
      return false;

   ctrl->setTitleBarStyle(style);
   return false;
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::setText(const String& text)
{
   if (String::compare(mText.c_str(), text.c_str()) == 0)
      return;

   mText = text;
   mGuiText.setText(mText);
   setUpdate();
}

//-----------------------------------------------------------------------------

bool GuiWindowCtrlNew::setTextProt(void* object, const char* index, const char* data)
{
   static_cast<GuiWindowCtrlNew*>(object)->setText(data);
   return false;
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onRemove()
{
   // mTitleBarStyle must be released via the real setTitleBarStyle(NULL)
   // path -- same rationale as GuiScrollCtrlNew::onRemove().
   if (mTitleBarStyle)
      setTitleBarStyle(NULL);

   Parent::onRemove();
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onDeleteNotify(SimObject* object)
{
   if (object == mTitleBarStyle)
   {
      GuiStyle* fallback;
      Sim::findObject("GuiDefaultStyle", fallback);

      if (fallback == mTitleBarStyle)
         mTitleBarStyle = NULL;
      else
         setTitleBarStyle(fallback);
   }

   Parent::onDeleteNotify(object);
}

//-----------------------------------------------------------------------------

bool GuiWindowCtrlNew::onWake()
{
   if (!Parent::onWake())
      return false;

   if (mTitleBarStyle)
      mTitleBarStyle->incLoadCount();

   mGuiText.setText(mText);

   return true;
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onSleep()
{
   if (mTitleBarStyle)
      mTitleBarStyle->decLoadCount();

   Parent::onSleep();
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onWindowClose()
{
   setVisible(false);
   onWindowClose_callback();
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::setMinimized(bool minimized)
{
   if (minimized == mMinimized)
      return;

   if (minimized)
   {
      // If currently maximized, restore that first
      {
         mMaximized = false;
         resize(mRestoreBounds.point, mRestoreBounds.extent);
      }

      mRestoreBounds = RectI(getPosition(), getExtent());
      mMinimized = true;

      // Collapse to exactly _getMinExtentWithChrome()'s title-bar-only
      // floor
      const Point2I minExtent = _getMinExtentWithChrome();
      resize(getPosition(), Point2I(getExtent().x, minExtent.y));

      onWindowMinimize_callback();
   }
   else
   {
      mMinimized = false;
      resize(mRestoreBounds.point, mRestoreBounds.extent);
      onWindowRestore_callback();
   }

   setUpdate();
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::setMaximized(bool maximized)
{
   if (maximized == mMaximized)
      return;

   GuiControlNew* parent = getParent();
   if (maximized && !parent)
      return; // nothing to fill

   if (maximized)
   {
      // Same "restore the true prior size first" rationale as
      // setMinimized()
      if (mMinimized)
      {
         mMinimized = false;
         resize(mRestoreBounds.point, mRestoreBounds.extent);
      }

      mRestoreBounds = RectI(getPosition(), getExtent());
      mMaximized = true;

      resize(Point2I(0, 0), parent->getExtent());

      onWindowMaximize_callback();
   }
   else
   {
      mMaximized = false;
      resize(mRestoreBounds.point, mRestoreBounds.extent);
      onWindowRestore_callback();
   }

   setUpdate();
}

//-----------------------------------------------------------------------------
//    Layout helpers
//-----------------------------------------------------------------------------

RectI GuiWindowCtrlNew::_getTitleBarDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!mShowTitleBar)
      return RectI(Point2I(0, 0), Point2I(0, 0));

   // Device-pixel height -- mTitleBarHeight is authored in
   // logical units like every other layout field
   const Point2I logicalExtent = getExtent();
   const F32 scaleY = logicalExtent.y > 0 ? (F32)ctrlDeviceRect.extent.y / (F32)logicalExtent.y : 1.0f;
   const S32 barThicknessDevice = (S32)(mTitleBarHeight * scaleY);

   return RectI(Point2I(ctrlDeviceRect.point.x, ctrlDeviceRect.point.y),
      Point2I(ctrlDeviceRect.extent.x, barThicknessDevice));
}

//-----------------------------------------------------------------------------

RectI GuiWindowCtrlNew::_getCloseButtonDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!mShowTitleBar || !mShowCloseButton)
      return RectI(ctrlDeviceRect.point, Point2I(0, 0));

   const RectI titleBar = _getTitleBarDeviceRect(ctrlDeviceRect);

   GuiCanvasNew* root = getRoot();
   const F32 scaleX = root ? root->getEffectiveScaleX() : 1.0f;
   const S32 marginDevice = (S32)(kCloseButtonMargin * scaleX);

   // Square button, inset from the title bar's top/bottom by the margin,
   // right-aligned within the title bar (also inset by the margin).
   const S32 side = getMax(0, titleBar.extent.y - marginDevice * 2);
   const Point2I pos(
      titleBar.point.x + titleBar.extent.x - marginDevice - side,
      titleBar.point.y + marginDevice);

   return RectI(pos, Point2I(side, side));
}

//-----------------------------------------------------------------------------

RectI GuiWindowCtrlNew::_getMaximizeButtonDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!mShowTitleBar || !mMaximizable)
      return RectI(ctrlDeviceRect.point, Point2I(0, 0));

   const RectI titleBar = _getTitleBarDeviceRect(ctrlDeviceRect);

   GuiCanvasNew* root = getRoot();
   const F32 scaleX = root ? root->getEffectiveScaleX() : 1.0f;
   const S32 marginDevice = (S32)(kCloseButtonMargin * scaleX);
   const S32 side = getMax(0, titleBar.extent.y - marginDevice * 2);

   // Reserve the close button's slot even when showCloseButton is false
   const S32 closeSlot = side + marginDevice;

   const Point2I pos(
      titleBar.point.x + titleBar.extent.x - marginDevice - closeSlot - side,
      titleBar.point.y + marginDevice);

   return RectI(pos, Point2I(side, side));
}

//-----------------------------------------------------------------------------

RectI GuiWindowCtrlNew::_getMinimizeButtonDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!mShowTitleBar || !mMinimizable)
      return RectI(ctrlDeviceRect.point, Point2I(0, 0));

   const RectI titleBar = _getTitleBarDeviceRect(ctrlDeviceRect);

   GuiCanvasNew* root = getRoot();
   const F32 scaleX = root ? root->getEffectiveScaleX() : 1.0f;
   const S32 marginDevice = (S32)(kCloseButtonMargin * scaleX);
   const S32 side = getMax(0, titleBar.extent.y - marginDevice * 2);

   // Reserve BOTH the close and maximize slots (stable regardless of
   // showCloseButton/mMaximizable)
   const S32 closeSlot = side + marginDevice;
   const S32 maximizeSlot = side + marginDevice;

   const Point2I pos(
      titleBar.point.x + titleBar.extent.x - marginDevice - closeSlot - maximizeSlot - side,
      titleBar.point.y + marginDevice);

   return RectI(pos, Point2I(side, side));
}

//-----------------------------------------------------------------------------

RectI GuiWindowCtrlNew::_getResizeGripDeviceRect(const RectI& ctrlDeviceRect) const
{
   if (!mResizable)
      return RectI(ctrlDeviceRect.point, Point2I(0, 0));

   GuiCanvasNew* root = getRoot();
   const F32 scaleX = root ? root->getEffectiveScaleX() : 1.0f;
   const F32 scaleY = root ? root->getEffectiveScaleY() : 1.0f;

   // Same logical grip size regardless of window size
   const S32 gripW = getMax(1, (S32)(kResizeGripLogicalSize * scaleX));
   const S32 gripH = getMax(1, (S32)(kResizeGripLogicalSize * scaleY));

   const Point2I pos(
      ctrlDeviceRect.point.x + ctrlDeviceRect.extent.x - gripW,
      ctrlDeviceRect.point.y + ctrlDeviceRect.extent.y - gripH);

   return RectI(pos, Point2I(gripW, gripH));
}

//-----------------------------------------------------------------------------

Point2I GuiWindowCtrlNew::_getMinExtentWithChrome() const
{
   Point2I result = getMinExtent();

   if (mShowTitleBar)
   {
      // Enough width for the title bar's own buttons (however many are
      // actually enabled)
      S32 buttonCount = 0;
      if (mShowCloseButton) buttonCount++;
      if (mMaximizable) buttonCount++;
      if (mMinimizable) buttonCount++;

      const S32 minWidthForButtons = buttonCount > 0
         ? (buttonCount * (mTitleBarHeight - kCloseButtonMargin * 2) + (buttonCount + 1) * kCloseButtonMargin)
         : 0;

      result.x = getMax(result.x, minWidthForButtons);

      // Height must at least fit the title bar itself, whether or not
      // there's any client area left below it (a fully-minimized window
      // legitimately wants exactly this height -- see setMinimized()).
      result.y = getMax(result.y, mTitleBarHeight);
   }

   return result;
}

//-----------------------------------------------------------------------------

U32 GuiWindowCtrlNew::_getTitleBarStyleStateMask() const
{
   U32 mask = 0;

   if (mDraggingTitleBar)
      mask |= GuiStyle::bit(GuiStyleState::Active);

   return mask;
}

//-----------------------------------------------------------------------------
//    Input
//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onMouseDown(const GuiEvent& event)
{
   // A click anywhere in the window brings it forward, same as any real
   // OS window manager -- happens regardless of whether the click also
   // starts a title bar drag or lands on ordinary content.
   bringToFront();

   const RectI deviceBounds = getDeviceBounds();
   const RectI localDeviceRect(Point2I(0, 0), deviceBounds.extent);

   // event.mousePoint is root/canvas-relative logical space.
   const Point2I logicalLocal = globalToLocalCoord(event.mousePoint);
   const Point2I logicalExtent = getExtent();
   const F32 scaleX = logicalExtent.x > 0 ? (F32)deviceBounds.extent.x / (F32)logicalExtent.x : 1.0f;
   const F32 scaleY = logicalExtent.y > 0 ? (F32)deviceBounds.extent.y / (F32)logicalExtent.y : 1.0f;
   const Point2I deviceLocal((S32)(logicalLocal.x * scaleX), (S32)(logicalLocal.y * scaleY));

   // Resize grip checked before any title bar hit-testing below -- it's
   // legitimately reachable even when !mShowTitleBar (a window with no
   // title bar can still be resizable)
   if (mResizable && !mMaximized)
   {
      const RectI resizeGrip = _getResizeGripDeviceRect(localDeviceRect);
      if (resizeGrip.pointInRect(deviceLocal))
      {
         mDraggingResize = true;
         // Offset from the window's BOTTOM-RIGHT corner, mirroring
         // mDragGrabOffsetDevice's "offset from top-left" for the move
         // drag -- see mResizeGrabOffsetDevice's doc comment.
         mResizeGrabOffsetDevice = deviceLocal - deviceBounds.extent;
         mouseLock();
         setFirstResponder();
         setUpdate();
         return;
      }
   }

   if (mShowTitleBar)
   {
      const RectI closeButton = _getCloseButtonDeviceRect(localDeviceRect);
      if (mShowCloseButton && closeButton.pointInRect(deviceLocal))
      {
         onWindowClose();
         return;
      }

      const RectI maximizeButton = _getMaximizeButtonDeviceRect(localDeviceRect);
      if (mMaximizable && maximizeButton.pointInRect(deviceLocal))
      {
         setMaximized(!mMaximized);
         return;
      }

      const RectI minimizeButton = _getMinimizeButtonDeviceRect(localDeviceRect);
      if (mMinimizable && minimizeButton.pointInRect(deviceLocal))
      {
         setMinimized(!mMinimized);
         return;
      }

      const RectI titleBar = _getTitleBarDeviceRect(localDeviceRect);

      // A maximized window doesn't drag
      if (mMovable && !mMaximized && titleBar.pointInRect(deviceLocal))
      {
         mDraggingTitleBar = true;
         mDragGrabOffsetDevice = deviceLocal; // offset from the window's own top-left (0,0), not the title bar's
         mouseLock();
         setFirstResponder();
         setUpdate();
         return;
      }
   }

   Parent::onMouseDown(event);
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onMouseDragged(const GuiEvent& event)
{
   if (mDraggingResize)
   {
      const RectI deviceBounds = getDeviceBounds();
      const Point2I logicalExtent = getExtent();
      const F32 scaleX = logicalExtent.x > 0 ? (F32)deviceBounds.extent.x / (F32)logicalExtent.x : 1.0f;
      const F32 scaleY = logicalExtent.y > 0 ? (F32)deviceBounds.extent.y / (F32)logicalExtent.y : 1.0f;

      const Point2I mouseLocal = globalToLocalCoord(event.mousePoint);
      const Point2I mouseLocalDevice(
         (S32)(mouseLocal.x * scaleX),
         (S32)(mouseLocal.y * scaleY));

      // Desired bottom-right corner in device space
      const Point2I desiredBottomRightDevice = mouseLocalDevice - mResizeGrabOffsetDevice;
      const Point2I desiredExtentLogical(
         (S32)(desiredBottomRightDevice.x / getMax(scaleX, 0.0001f)),
         (S32)(desiredBottomRightDevice.y / getMax(scaleY, 0.0001f)));

      const Point2I minExtent = _getMinExtentWithChrome();
      const Point2I clampedExtent(
         getMax(desiredExtentLogical.x, minExtent.x),
         getMax(desiredExtentLogical.y, minExtent.y));

      resize(getPosition(), clampedExtent);
      return;
   }

   if (!mDraggingTitleBar)
   {
      Parent::onMouseDragged(event);
      return;
   }

   GuiControlNew* parent = getParent();
   if (!parent)
      return;

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;

   const Point2I logicalExtent = getExtent();
   const RectI deviceBounds = getDeviceBounds();
   const F32 scaleX = logicalExtent.x > 0 ? (F32)deviceBounds.extent.x / (F32)logicalExtent.x : 1.0f;
   const F32 scaleY = logicalExtent.y > 0 ? (F32)deviceBounds.extent.y / (F32)logicalExtent.y : 1.0f;

   const Point2I grabOffsetLogical(
      (S32)(mDragGrabOffsetDevice.x / getMax(scaleX, 0.0001f)),
      (S32)(mDragGrabOffsetDevice.y / getMax(scaleY, 0.0001f)));

   // event.mousePoint is root/canvas-relative logical space; re-express
   // relative to the parent (what setPosition() is in terms of) via the
   // parent's OWN globalToLocalCoord() -- same helper this control's
   // onMouseDown() above uses on itself, just called on the parent
   // instead since setPosition() is parent-relative.
   const Point2I mouseInParent = parent->globalToLocalCoord(event.mousePoint);

   setPosition(mouseInParent - grabOffsetLogical);
}

//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onMouseUp(const GuiEvent& event)
{
   if (mDraggingTitleBar)
   {
      mDraggingTitleBar = false;
      if (isMouseLocked())
         mouseUnlock();
      setUpdate();
      return;
   }

   if (mDraggingResize)
   {
      mDraggingResize = false;
      if (isMouseLocked())
         mouseUnlock();
      setUpdate();
      return;
   }

   Parent::onMouseUp(event);
}

//-----------------------------------------------------------------------------

bool GuiWindowCtrlNew::resize(const Point2I& newPosition, const Point2I& newExtent)
{
   // Single choke point for the "never smaller than the title bar's own
   // buttons need" floor -- covers the resize-grip drag (which already
   // pre-clamps, so this is a no-op there) AND any script-authored
   // resize()/setExtent() call, which wouldn't otherwise know about
   // _getMinExtentWithChrome() at all.
   const Point2I minExtent = _getMinExtentWithChrome();
   const Point2I clampedExtent(
      getMax(newExtent.x, minExtent.x),
      getMax(newExtent.y, minExtent.y));

   return Parent::resize(newPosition, clampedExtent);
}

//-----------------------------------------------------------------------------
//    Render
//-----------------------------------------------------------------------------

void GuiWindowCtrlNew::onRender(Point2I offset, const RectI& updateRect)
{
   const RectI ctrlRect(offset, getDeviceBounds().extent);
   const GuiStyleProperties style = resolveStyle();

   GuiCanvasNew* root = getRoot();
   if (!root)
      return;
   GuiRenderBatch& batch = root->getRenderBatch();

   const S32 titleBarDevice = mShowTitleBar ? _getTitleBarDeviceRect(ctrlRect).extent.y : 0;
   const ColorI backgroundColor = style.backgroundColor.isSet() ? style.backgroundColor.mValue : kFallbackBackgroundColor;
   batch.pushQuad(ctrlRect, backgroundColor, getRenderLayer());

   const RectI clientDeviceRect(
      Point2I(ctrlRect.point.x, ctrlRect.point.y + titleBarDevice),
      Point2I(ctrlRect.extent.x, ctrlRect.extent.y - titleBarDevice));

   const Point2I clientOffset(offset.x, offset.y + titleBarDevice);

   RectI clippedUpdateRect = updateRect;
   if (clippedUpdateRect.intersect(clientDeviceRect))
   {
      const RectI savedClipRect = GFX->getClipRect();
      GFX->setClipRect(clippedUpdateRect);
      batch.pushClipRect(clippedUpdateRect);
      renderChildControls(clientOffset, clippedUpdateRect);
      batch.popClipRect();

      GFX->setClipRect(savedClipRect);
   }

   if (style.borderWidth.isSet() && style.borderWidth.mValue > 0 && style.borderColor.isSet())
   {
      const S32 bw = style.borderWidth.mValue;
      const ColorI& bc = style.borderColor.mValue;
      batch.pushQuad(RectI(ctrlRect.point, Point2I(ctrlRect.extent.x, bw)), bc, getRenderLayer());
      batch.pushQuad(RectI(Point2I(ctrlRect.point.x, ctrlRect.point.y + ctrlRect.extent.y - bw), Point2I(ctrlRect.extent.x, bw)), bc, getRenderLayer());
      batch.pushQuad(RectI(ctrlRect.point, Point2I(bw, ctrlRect.extent.y)), bc, getRenderLayer());
      batch.pushQuad(RectI(Point2I(ctrlRect.point.x + ctrlRect.extent.x - bw, ctrlRect.point.y), Point2I(bw, ctrlRect.extent.y)), bc, getRenderLayer());
   }

   const S32 chromeLayer = getRenderLayer();


   if (mShowTitleBar)
   {
      RectI titleBarDeviceRect = _getTitleBarDeviceRect(ctrlRect);
      GuiStyle* titleStyleRef = mTitleBarStyle ? mTitleBarStyle : mStyle;
      GuiStyleProperties titleStyle;
      if (titleStyleRef)
         titleStyle = titleStyleRef->resolve(_getTitleBarStyleStateMask());

      const S32 marginDevice = (S32)(kCloseButtonMargin * getEffectiveScaleX());
      const ColorI titleBarColor = titleStyle.backgroundColor.isSet() ? titleStyle.backgroundColor.mValue : kFallbackTitleBarColor;
      const Point2I textOffset(titleBarDeviceRect.point.x + marginDevice, titleBarDeviceRect.point.y);

      // Reserve room for every enabled button's slot, not just close
      const S32 buttonSlotWidth = titleBarDeviceRect.extent.y - marginDevice; // one button's (side + margin), reused per enabled button
      S32 reservedForButtons = marginDevice;
      if (mShowCloseButton) reservedForButtons += buttonSlotWidth;
      if (mMaximizable) reservedForButtons += buttonSlotWidth;
      if (mMinimizable) reservedForButtons += buttonSlotWidth;

      const Point2I textExtent(getMax(0, titleBarDeviceRect.extent.x - marginDevice - reservedForButtons), titleBarDeviceRect.extent.y);

      batch.pushQuad(titleBarDeviceRect, titleBarColor, chromeLayer);

      if (mStyle && mText.length() > 0)
      {
         mGuiText.setAlignHorizontal(GuiTextAlignHorizontal_Left);
         mGuiText.setAlignVertical(GuiTextAlignVertical_Middle);

         renderText(mGuiText, textOffset, textExtent);
      }

      if (mShowCloseButton)
      {
         const RectI closeButton = _getCloseButtonDeviceRect(ctrlRect);
         const ColorI closeButtonColor = kFallbackCloseButtonColor;
         batch.pushQuad(closeButton, closeButtonColor, chromeLayer);

         const S32 inset = getMax(1, closeButton.extent.x / 4);
         const S32 thickness = getMax(1, closeButton.extent.x / 8);
         const Point2I center(closeButton.point.x + closeButton.extent.x / 2, closeButton.point.y + closeButton.extent.y / 2);
         const S32 armLen = closeButton.extent.x / 2 - inset;

         if (armLen > 0)
         {
            batch.pushQuad(RectI(Point2I(center.x - armLen, center.y - thickness / 2), Point2I(armLen * 2, thickness)), kFallbackCloseGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(Point2I(center.x - thickness / 2, center.y - armLen), Point2I(thickness, armLen * 2)), kFallbackCloseGlyphColor, chromeLayer + 1);
         }
      }

      if (mMaximizable)
      {
         const RectI maximizeButton = _getMaximizeButtonDeviceRect(ctrlRect);
         batch.pushQuad(maximizeButton, kFallbackChromeButtonColor, chromeLayer);

         const S32 inset = getMax(1, maximizeButton.extent.x / 4);
         const S32 thickness = getMax(1, maximizeButton.extent.x / 8);

         if (mMaximized)
         {
            // Restore glyph: two overlapping small squares (outlines)
            const S32 side = maximizeButton.extent.x - inset * 2;
            const S32 smallSide = getMax(1, (S32)(side * 0.7f));
            const Point2I backPos(maximizeButton.point.x + inset + (side - smallSide), maximizeButton.point.y + inset);
            const Point2I frontPos(maximizeButton.point.x + inset, maximizeButton.point.y + inset + (side - smallSide));

            // Each "square" drawn as a 4-sided outline (thin quads)
            batch.pushQuad(RectI(backPos, Point2I(smallSide, thickness)), kFallbackButtonGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(Point2I(backPos.x, backPos.y + smallSide - thickness), Point2I(smallSide, thickness)), kFallbackButtonGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(backPos, Point2I(thickness, smallSide)), kFallbackButtonGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(Point2I(backPos.x + smallSide - thickness, backPos.y), Point2I(thickness, smallSide)), kFallbackButtonGlyphColor, chromeLayer + 1);

            batch.pushQuad(RectI(frontPos, Point2I(smallSide, thickness)), kFallbackButtonGlyphColor, chromeLayer + 2);
            batch.pushQuad(RectI(Point2I(frontPos.x, frontPos.y + smallSide - thickness), Point2I(smallSide, thickness)), kFallbackButtonGlyphColor, chromeLayer + 2);
            batch.pushQuad(RectI(frontPos, Point2I(thickness, smallSide)), kFallbackButtonGlyphColor, chromeLayer + 2);
            batch.pushQuad(RectI(Point2I(frontPos.x + smallSide - thickness, frontPos.y), Point2I(thickness, smallSide)), kFallbackButtonGlyphColor, chromeLayer + 2);
         }
         else
         {
            // Maximize glyph: single square outline.
            const S32 side = maximizeButton.extent.x - inset * 2;
            const Point2I pos(maximizeButton.point.x + inset, maximizeButton.point.y + inset);

            batch.pushQuad(RectI(pos, Point2I(side, thickness)), kFallbackButtonGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(Point2I(pos.x, pos.y + side - thickness), Point2I(side, thickness)), kFallbackButtonGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(pos, Point2I(thickness, side)), kFallbackButtonGlyphColor, chromeLayer + 1);
            batch.pushQuad(RectI(Point2I(pos.x + side - thickness, pos.y), Point2I(thickness, side)), kFallbackButtonGlyphColor, chromeLayer + 1);
         }
      }

      if (mMinimizable)
      {
         const RectI minimizeButton = _getMinimizeButtonDeviceRect(ctrlRect);
         batch.pushQuad(minimizeButton, kFallbackChromeButtonColor, chromeLayer);

         // Minimize glyph: single horizontal bar near the bottom of the
         // button -- conventional "minimize" icon.
         const S32 inset = getMax(1, minimizeButton.extent.x / 4);
         const S32 thickness = getMax(1, minimizeButton.extent.x / 8);
         const S32 barWidth = minimizeButton.extent.x - inset * 2;
         const Point2I barPos(
            minimizeButton.point.x + inset,
            minimizeButton.point.y + minimizeButton.extent.y - inset - thickness);

         if (barWidth > 0)
            batch.pushQuad(RectI(barPos, Point2I(barWidth, thickness)), kFallbackButtonGlyphColor, chromeLayer + 1);
      }
   }

   if (mResizable && !mMaximized)
   {
      const RectI resizeGrip = _getResizeGripDeviceRect(ctrlRect);

      // Three short diagonal strokes near the corner
      const S32 dotSize = getMax(1, resizeGrip.extent.x / 8);
      const S32 spacing = getMax(dotSize + 1, resizeGrip.extent.x / 4);

      for (S32 row = 0; row < 3; row++)
      {
         for (S32 col = 0; col <= row; col++)
         {
            const Point2I dotPos(
               resizeGrip.point.x + resizeGrip.extent.x - spacing * (row - col) - spacing,
               resizeGrip.point.y + resizeGrip.extent.y - spacing * col - spacing);

            batch.pushQuad(RectI(dotPos, Point2I(dotSize, dotSize)), kFallbackResizeGripColor, chromeLayer + 1);
         }
      }
   }
}
