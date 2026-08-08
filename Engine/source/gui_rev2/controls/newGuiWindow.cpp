//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiWindow.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiWindow.h"
#include "gui_rev2/core/newGuiRenderBatch.h"
#include "gui_rev2/core/newGuiCanvas.h"

IMPLEMENT_CONOBJECT(NewGuiWindow);

IMPLEMENT_CALLBACK(NewGuiWindow, onClose, void, (), (),
   "Called whenever this window is closed - a real click on the close glyph, or a script-driven "
   "performClose() call. Fires just before the window is hidden.");

IMPLEMENT_CALLBACK(NewGuiWindow, onMinimize, void, (), (),
   "Called whenever this window is minimized - a real click on the minimize glyph, or a "
   "script-driven performMinimize() call. Fires just before the window collapses to its titlebar.");

IMPLEMENT_CALLBACK(NewGuiWindow, onMaximize, void, (), (),
   "Called whenever this window is maximized - a real click on the maximize glyph, or a "
   "script-driven performMaximize() call. Fires just before the window snaps to fill its parent.");

IMPLEMENT_CALLBACK(NewGuiWindow, onRestore, void, (), (),
   "Called whenever performRestore() reverts either a minimize or a maximize (or both) - a real "
   "click on the maximize glyph while already maximized, or a script-driven performRestore() call.");

IMPLEMENT_CALLBACK(NewGuiWindow, onWindowMoved, void, (), (),
   "Called whenever a titlebar drag or resize drag completes with the window's bounds actually "
   "having changed. Read left/top/width/height (or getBounds()) from script to persist the result.");

// Fixed pixel constants, same tier of detail as NewGuiScroll's kMinThumbLength/mScrollBarThickness -
// not style-driven, since they're interaction-hitbox geometry rather than visual appearance.
static const F32 kDefaultTitlebarHeight = 18.0f;
static const F32 kDefaultResizeGripSize = 6.0f;
static const F32 kDefaultMinWidth = 80.0f;
static const F32 kDefaultMinHeight = 48.0f;
static const S32 kGlyphMargin = 6;      ///< Device-pixel margin each titlebar glyph is inset by from its neighbor/the titlebar edge.

NewGuiWindow::NewGuiWindow()
   : mCachedFontFamily(NULL),
   mCachedFontSize(0.0f),
   mTitlebarHeight(kDefaultTitlebarHeight),
   mResizeGripSize(kDefaultResizeGripSize),
   mMinWidth(kDefaultMinWidth),
   mMinHeight(kDefaultMinHeight),
   mResizable(true),
   mMovable(true),
   mClosable(true),
   mMinimizable(true),
   mMaximizable(true),
   mDragInProgress(false),
   mDragLastPoint(0, 0),
   mResizeEdges(Edge_None),
   mResizeLastPoint(0, 0),
   mCloseArmed(false),
   mMinimizeArmed(false),
   mMaximizeArmed(false),
   mMinimized(false),
   mPreMinimizeHeight(NewGuiDimension::fromAuto()),
   mMaximized(false),
   mPreMaximizeLeft(NewGuiDimension::fromAuto()),
   mPreMaximizeTop(NewGuiDimension::fromAuto()),
   mPreMaximizeWidth(NewGuiDimension::fromAuto()),
   mPreMaximizeHeight(NewGuiDimension::fromAuto()),
   mHoverResizeEdges(Edge_None),
   mCloseHovered(false),
   mMinimizeHovered(false),
   mMaximizeHovered(false)
{
   mTitleText.setAlignHorizontal(NewGuiTextAlignHorizontal::Left);
   mTitleText.setAlignVertical(NewGuiTextAlignVertical::Middle);
   mTitleText.setOverflow(NewGuiTextOverflow_Ellipsis);
}

NewGuiWindow::~NewGuiWindow()
{
}

bool NewGuiWindow::_setTitleText(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->setTitleText(data);
   return false;
}

bool NewGuiWindow::_setTitlebarHeight(void* obj, const char* index, const char* data)
{
   NewGuiWindow* window = static_cast<NewGuiWindow*>(obj);
   window->mTitlebarHeight = getMax(0.0f, (F32)dAtof(data));
   window->setContentDirty();
   window->setArrangementDirty();
   return false;
}

bool NewGuiWindow::_setResizeGripSize(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->mResizeGripSize = getMax(0.0f, (F32)dAtof(data));
   return false;
}

bool NewGuiWindow::_setResizable(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->setResizable(dAtob(data));
   return false;
}

bool NewGuiWindow::_setMovable(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->setMovable(dAtob(data));
   return false;
}

bool NewGuiWindow::_setClosable(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->setClosable(dAtob(data));
   return false;
}

bool NewGuiWindow::_setMinimizable(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->setMinimizable(dAtob(data));
   return false;
}

bool NewGuiWindow::_setMaximizable(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->setMaximizable(dAtob(data));
   return false;
}

bool NewGuiWindow::_setMinWidth(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->mMinWidth = getMax(1.0f, (F32)dAtof(data));
   return false;
}

bool NewGuiWindow::_setMinHeight(void* obj, const char* index, const char* data)
{
   static_cast<NewGuiWindow*>(obj)->mMinHeight = getMax(1.0f, (F32)dAtof(data));
   return false;
}

void NewGuiWindow::setTitleText(const char* text)
{
   mTitleText.setText(text ? text : "");
   setContentDirty();
   setArrangementDirty();
}

void NewGuiWindow::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Window");

   ADD_FIELD("titleText", TypeString, 0)
      .onSet(_setTitleText)
      .doc("Text drawn in the titlebar.");

   ADD_FIELD("titlebarHeight", TypeF32, Offset(mTitlebarHeight, NewGuiWindow))
      .onSet(_setTitlebarHeight)
      .doc("Titlebar chrome height, in DESIGN units (converted to device pixels via the resolved UI scale, same as width/height/left/top). Default 28.");

   ADD_FIELD("resizeGripSize", TypeF32, Offset(mResizeGripSize, NewGuiWindow))
      .onSet(_setResizeGripSize)
      .doc("Thickness of the edge/corner resize hit region, in DESIGN units. Default 6.");

   ADD_FIELD("resizable", TypeBool, Offset(mResizable, NewGuiWindow))
      .onSet(_setResizable)
      .doc("Whether the edges/corners can be dragged to resize the window. Default true.");

   ADD_FIELD("movable", TypeBool, Offset(mMovable, NewGuiWindow))
      .onSet(_setMovable)
      .doc("Whether the titlebar can be dragged to move the window. Default true.");

   ADD_FIELD("closable", TypeBool, Offset(mClosable, NewGuiWindow))
      .onSet(_setClosable)
      .doc("Whether a close glyph is drawn/hit-testable in the titlebar. Default true.");

   ADD_FIELD("minimizable", TypeBool, Offset(mMinimizable, NewGuiWindow))
      .onSet(_setMinimizable)
      .doc("Whether a minimize glyph is drawn/hit-testable in the titlebar. Default true.");

   ADD_FIELD("maximizable", TypeBool, Offset(mMaximizable, NewGuiWindow))
      .onSet(_setMaximizable)
      .doc("Whether a maximize/restore glyph is drawn/hit-testable in the titlebar. Default true.");

   ADD_FIELD("minWidth", TypeF32, Offset(mMinWidth, NewGuiWindow))
      .onSet(_setMinWidth)
      .doc("Floor applied to a resize drag's resulting width, in DESIGN units. Default 80. Does not clamp an authored width/percent set outside a resize drag.");

   ADD_FIELD("minHeight", TypeF32, Offset(mMinHeight, NewGuiWindow))
      .onSet(_setMinHeight)
      .doc("Floor applied to a resize drag's resulting height, in DESIGN units. Default 48. Does not clamp an authored height/percent set outside a resize drag.");

   GROUP_END("Window");
}

void NewGuiWindow::resolveFont()
{
   const NewGuiResolvedStyle& style = getResolvedStyle();

   if (mTitleFont != NULL && style.fontFamily == mCachedFontFamily && style.fontSize == mCachedFontSize)
      return;

   const char* faceName = style.fontFamily ? style.fontFamily : "Arial";
   U32 size = (U32)(style.fontSize > 0.0f ? style.fontSize : 14.0f);

   mTitleFont = GFont::create(faceName, size);
   mCachedFontFamily = style.fontFamily;
   mCachedFontSize = style.fontSize;

   mTitleText.setFont(mTitleFont);
}

// Local space (0,0 = mBounds.point), device pixels - mTitlebarHeight is authored in design
// units, so it's converted here via mResolvedUIScaleY, same as GetClientRect() below and
// exactly the pattern padding conversion already uses (see NewGuiControl::GetClientRect()).
RectI NewGuiWindow::getTitlebarRect() const
{
   S32 heightPx = (S32)(mTitlebarHeight * mResolvedUIScaleY);
   heightPx = getMin(heightPx, mBounds.extent.y);
   if (heightPx <= 0)
      return RectI(0, 0, 0, 0);

   return RectI(Point2I(0, 0), Point2I(mBounds.extent.x, heightPx));
}

RectI NewGuiWindow::getCloseGlyphRect() const
{
   if (!mClosable)
      return RectI(0, 0, 0, 0);

   RectI titlebar = getTitlebarRect();
   if (titlebar.extent.y <= 0)
      return RectI(0, 0, 0, 0);

   S32 side = getMax(0, titlebar.extent.y - kGlyphMargin * 2);
   S32 x = titlebar.extent.x - kGlyphMargin - side;
   S32 y = kGlyphMargin;

   if (x < 0 || side <= 0)
      return RectI(0, 0, 0, 0);

   return RectI(Point2I(x, y), Point2I(side, side));
}

// Sits immediately left of the close glyph (or where it would be, if !mClosable) - same square
// side/margin math as getCloseGlyphRect(), just anchored off that rect's own left edge instead
// of the titlebar's, so the three glyphs chain together with no gap regardless of which are enabled.
RectI NewGuiWindow::getMaximizeGlyphRect() const
{
   if (!mMaximizable)
      return RectI(0, 0, 0, 0);

   RectI titlebar = getTitlebarRect();
   if (titlebar.extent.y <= 0)
      return RectI(0, 0, 0, 0);

   S32 side = getMax(0, titlebar.extent.y - kGlyphMargin * 2);
   if (side <= 0)
      return RectI(0, 0, 0, 0);

   RectI closeGlyph = getCloseGlyphRect();
   S32 rightEdge = (closeGlyph.extent.x > 0) ? closeGlyph.point.x : (titlebar.extent.x - kGlyphMargin);
   S32 x = rightEdge - kGlyphMargin - side;
   S32 y = kGlyphMargin;

   if (x < 0)
      return RectI(0, 0, 0, 0);

   return RectI(Point2I(x, y), Point2I(side, side));
}

// Sits immediately left of the maximize glyph (or where it would be, if !mMaximizable) - same
// chaining approach as getMaximizeGlyphRect() itself.
RectI NewGuiWindow::getMinimizeGlyphRect() const
{
   if (!mMinimizable)
      return RectI(0, 0, 0, 0);

   RectI titlebar = getTitlebarRect();
   if (titlebar.extent.y <= 0)
      return RectI(0, 0, 0, 0);

   S32 side = getMax(0, titlebar.extent.y - kGlyphMargin * 2);
   if (side <= 0)
      return RectI(0, 0, 0, 0);

   RectI maximizeGlyph = getMaximizeGlyphRect();
   S32 rightEdge;
   if (maximizeGlyph.extent.x > 0)
      rightEdge = maximizeGlyph.point.x;
   else
   {
      RectI closeGlyph = getCloseGlyphRect();
      rightEdge = (closeGlyph.extent.x > 0) ? closeGlyph.point.x : (titlebar.extent.x - kGlyphMargin);
   }

   S32 x = rightEdge - kGlyphMargin - side;
   S32 y = kGlyphMargin;

   if (x < 0)
      return RectI(0, 0, 0, 0);

   return RectI(Point2I(x, y), Point2I(side, side));
}

// Inset by the titlebar (device pixels) then Parent's own padding inset, same layering order as
// NewGuiScroll::GetClientRect() (Parent's padding first, then this control's own chrome) - here
// reversed is equally valid since the two insets are independent axes-of-different-edges, but
// titlebar-first keeps the "chrome closest to the outer edge subtracts first" reading consistent
// with the titlebar being drawn flush against mBounds' own top edge.
//
// No separate mMinimized branch needed: performMinimize() already sets mBounds.extent.y down to
// exactly the titlebar's own height (see that method), so "mBounds.extent.y - titlebar.extent.y"
// below is already zero or clamped to zero by the getMax() calls - the same math naturally
// yields zero client area without a special case.
RectI NewGuiWindow::GetClientRect() const
{
   RectI titlebar = getTitlebarRect();

   RectI belowTitlebar(
      Point2I(mBounds.point.x, mBounds.point.y + titlebar.extent.y),
      Point2I(mBounds.extent.x, getMax(0, mBounds.extent.y - titlebar.extent.y)));

   const NewGuiEdgeInsets& padding = mResolvedStyle.padding;
   S32 padLeft = (S32)(padding.left * mResolvedUIScaleX);
   S32 padRight = (S32)(padding.right * mResolvedUIScaleX);
   S32 padTop = (S32)(padding.top * mResolvedUIScaleY);
   S32 padBottom = (S32)(padding.bottom * mResolvedUIScaleY);

   return RectI(
      Point2I(belowTitlebar.point.x + padLeft, belowTitlebar.point.y + padTop),
      Point2I(getMax(0, belowTitlebar.extent.x - padLeft - padRight),
         getMax(0, belowTitlebar.extent.y - padTop - padBottom)));
}

void NewGuiWindow::renderChildControls(NewGuiRenderBatch* batch)
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

// Titlebar's own preferred contribution (title text width + every enabled glyph's own
// allowance + titlebar height) unioned against Parent's normal children-preferred-size default,
// so an auto-sized window never clips its own title/chrome even if its content child is narrower.
Point2I NewGuiWindow::ComputePreferredSize()
{
   resolveFont();

   Point2I base = Parent::ComputePreferredSize();

   mTitleText.setBoxExtent(Point2I(0, 0));
   const NewGuiTextLayoutResult& result = mTitleText.layout();

   S32 titleTextWidth = result.lines.empty() ? 0 : result.blockBounds.extent.x;

   // One (titlebarHeight-ish square + margin) allowance per enabled glyph - mirrors each
   // getXGlyphRect()'s own side = titlebar.extent.y - kGlyphMargin*2 sizing, just in
   // titlebar-height terms rather than resolved device pixels (see the fallbackScale note below
   // for why device pixels aren't available yet here).
   S32 enabledGlyphCount = (mClosable ? 1 : 0) + (mMaximizable ? 1 : 0) + (mMinimizable ? 1 : 0);
   S32 glyphAllowance = enabledGlyphCount * ((S32)mTitlebarHeight + kGlyphMargin);

   // mResolvedUIScaleX/Y aren't necessarily valid yet at MeasurePass time on a first frame (see
   // ArrangePass()'s own doc comment on when scale is resolved) - fall back to a 1:1 assumption
   // for this preferred-size contribution, same "one frame of latency is acceptable" tradeoff the
   // design doc calls out for measure/arrange ordering in general.
   F32 fallbackScale = (mResolvedUIScaleX > 0.0f) ? mResolvedUIScaleX : 1.0f;
   S32 titlebarMinWidth = (S32)((titleTextWidth / fallbackScale)) + glyphAllowance + kGlyphMargin + (S32)mResolvedStyle.padding.horizontal();
   S32 titlebarHeightContribution = (S32)mTitlebarHeight;

   return Point2I(
      getMax(base.x, titlebarMinWidth),
      base.y + titlebarHeightContribution);
}

void NewGuiWindow::EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer)
{
   Parent::EmitDrawCommands(batch, bounds, style, layer);

   if (!batch || style.opacity <= 0.0f)
      return;

   RectI titlebar = getTitlebarRect();
   if (titlebar.extent.y <= 0)
      return;

   RectI screenTitlebar(bounds.point + titlebar.point, titlebar.extent);

   // Titlebar fill: a fixed darkening of the resolved background, same "derive chrome from the
   // resolved style rather than authoring a second color" approach as nothing else in this system
   // currently needs a second background color field for one piece of chrome.
   ColorI titlebarColor(
      (U8)(style.backgroundColor.red * 0.75f),
      (U8)(style.backgroundColor.green * 0.75f),
      (U8)(style.backgroundColor.blue * 0.75f),
      (U8)((F32)style.backgroundColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));
   batch->pushQuad(screenTitlebar, titlebarColor, layer);

   if (style.borderWidth > 0.0f && style.borderColor.alpha > 0)
   {
      ColorI borderColor(
         style.borderColor.red, style.borderColor.green, style.borderColor.blue,
         (U8)((F32)style.borderColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));
      Point2I bl(screenTitlebar.point.x, screenTitlebar.point.y + screenTitlebar.extent.y);
      Point2I br(screenTitlebar.point.x + screenTitlebar.extent.x, screenTitlebar.point.y + screenTitlebar.extent.y);
      batch->pushLine(bl, br, borderColor, 1.0f, layer);
   }

   RectI closeGlyph = getCloseGlyphRect();
   RectI maximizeGlyph = getMaximizeGlyphRect();
   RectI minimizeGlyph = getMinimizeGlyphRect();

   // Leftmost edge among whatever glyphs are actually enabled - the title text is inset to stop
   // there, regardless of which subset of the three glyphs is present.
   S32 leftmostGlyphX = titlebar.extent.x;
   if (closeGlyph.extent.x > 0) leftmostGlyphX = getMin(leftmostGlyphX, closeGlyph.point.x);
   if (maximizeGlyph.extent.x > 0) leftmostGlyphX = getMin(leftmostGlyphX, maximizeGlyph.point.x);
   if (minimizeGlyph.extent.x > 0) leftmostGlyphX = getMin(leftmostGlyphX, minimizeGlyph.point.x);

   if (mTitleFont != NULL)
   {
      S32 textRightInset = (leftmostGlyphX < titlebar.extent.x) ? (titlebar.extent.x - leftmostGlyphX + kGlyphMargin) : kGlyphMargin;

      RectI textRect(
         Point2I(screenTitlebar.point.x + kGlyphMargin, screenTitlebar.point.y),
         Point2I(getMax(0, screenTitlebar.extent.x - kGlyphMargin - textRightInset), screenTitlebar.extent.y));

      ColorI textColor(
         style.textColor.red, style.textColor.green, style.textColor.blue,
         (U8)((F32)style.textColor.alpha * mClampF(style.opacity, 0.0f, 1.0f)));

      mTitleText.setBoxExtent(textRect.extent);
      mTitleText.submit(*batch, textRect.point, textColor, layer);
   }

   if (mMinimizable && minimizeGlyph.extent.x > 0)
   {
      RectI screenMinimize(bounds.point + minimizeGlyph.point, minimizeGlyph.extent);

      ColorI minimizeColor = mMinimizeHovered
         ? style.borderColor
         : ColorI(style.textColor.red, style.textColor.green, style.textColor.blue, style.textColor.alpha);

      // Minimize glyph: a single horizontal line flush against the bottom of the glyph box -
      // same "line-based glyph, no font dependency" approach as the close glyph's X and
      // NewGuiButton's checkbox check mark.
      Point2I left(screenMinimize.point.x, screenMinimize.point.y + screenMinimize.extent.y - 1);
      Point2I right(screenMinimize.point.x + screenMinimize.extent.x, left.y);
      batch->pushLine(left, right, minimizeColor, 2.0f, layer);
   }

   if (mMaximizable && maximizeGlyph.extent.x > 0)
   {
      RectI screenMaximize(bounds.point + maximizeGlyph.point, maximizeGlyph.extent);

      ColorI maximizeColor = mMaximizeHovered
         ? style.borderColor
         : ColorI(style.textColor.red, style.textColor.green, style.textColor.blue, style.textColor.alpha);

      if (mMaximized)
      {
         // Restore glyph: two overlapping square outlines (the conventional "restore" icon),
         // each a bit smaller than the full glyph box so they read distinctly at small sizes.
         S32 backInset = getMax(2, screenMaximize.extent.x / 4);
         RectI backSquare(
            Point2I(screenMaximize.point.x + backInset, screenMaximize.point.y),
            Point2I(screenMaximize.extent.x - backInset, screenMaximize.extent.y - backInset));
         RectI frontSquare(
            Point2I(screenMaximize.point.x, screenMaximize.point.y + backInset),
            Point2I(screenMaximize.extent.x - backInset, screenMaximize.extent.y - backInset));

         auto pushSquareOutline = [&](const RectI& r)
         {
            Point2I tl = r.point;
            Point2I tr(r.point.x + r.extent.x, r.point.y);
            Point2I bl(r.point.x, r.point.y + r.extent.y);
            Point2I br(r.point.x + r.extent.x, r.point.y + r.extent.y);
            batch->pushLine(tl, tr, maximizeColor, 1.5f, layer);
            batch->pushLine(tr, br, maximizeColor, 1.5f, layer);
            batch->pushLine(br, bl, maximizeColor, 1.5f, layer);
            batch->pushLine(bl, tl, maximizeColor, 1.5f, layer);
         };

         pushSquareOutline(backSquare);
         pushSquareOutline(frontSquare);
      }
      else
      {
         // Maximize glyph: one square outline.
         Point2I tl = screenMaximize.point;
         Point2I tr(screenMaximize.point.x + screenMaximize.extent.x, screenMaximize.point.y);
         Point2I bl(screenMaximize.point.x, screenMaximize.point.y + screenMaximize.extent.y);
         Point2I br(screenMaximize.point.x + screenMaximize.extent.x, screenMaximize.point.y + screenMaximize.extent.y);
         batch->pushLine(tl, tr, maximizeColor, 1.5f, layer);
         batch->pushLine(tr, br, maximizeColor, 1.5f, layer);
         batch->pushLine(br, bl, maximizeColor, 1.5f, layer);
         batch->pushLine(bl, tl, maximizeColor, 1.5f, layer);
      }
   }

   if (mClosable && closeGlyph.extent.x > 0)
   {
      RectI screenClose(bounds.point + closeGlyph.point, closeGlyph.extent);

      ColorI closeColor = mCloseHovered
         ? ColorI(220, 80, 80, style.borderColor.alpha)
         : ColorI(style.textColor.red, style.textColor.green, style.textColor.blue, style.textColor.alpha);

      // X glyph as two diagonal lines - same "line-based glyph, no font dependency" approach
      // as NewGuiButton's checkbox check mark.
      Point2I tl = screenClose.point;
      Point2I br(screenClose.point.x + screenClose.extent.x, screenClose.point.y + screenClose.extent.y);
      Point2I tr(br.x, tl.y);
      Point2I bl(tl.x, br.y);
      batch->pushLine(tl, br, closeColor, 2.0f, layer);
      batch->pushLine(tr, bl, closeColor, 2.0f, layer);
   }
}

// Local-space (0,0 = mBounds.point) edge/corner hit-test against mResizeGripSize, converted to
// device pixels the same way every other authored design-unit distance in this control is.
U8 NewGuiWindow::hitTestResizeEdges(const Point2I& localPoint) const
{
   if (!mResizable || mMaximized || mMinimized)
      return Edge_None;

   S32 gripPx = (S32)(mResizeGripSize * getMin(mResolvedUIScaleX, mResolvedUIScaleY));
   if (gripPx <= 0)
      return Edge_None;

   const RectI localBounds(Point2I(0, 0), mBounds.extent);
   if (!localBounds.pointInRect(localPoint))
      return Edge_None;

   U8 edges = Edge_None;
   if (localPoint.x < gripPx)
      edges |= Edge_Left;
   else if (localPoint.x >= mBounds.extent.x - gripPx)
      edges |= Edge_Right;

   if (localPoint.y < gripPx)
      edges |= Edge_Top;
   else if (localPoint.y >= mBounds.extent.y - gripPx)
      edges |= Edge_Bottom;

   return edges;
}

// See the doc comment on NewGuiWindow.h - the actual fix for "can't resize past a child flush
// against the edge". Deliberately does NOT call Parent::findHitControl() first and compare -
// the grip band must win outright, even over a child that's visible/hit-testable/on top, or the
// original bug (child claims the point, window's own onMouseDown() never runs) just reappears
// for whichever children happen to be big enough to fully cover the band.
//
// mHitTestable == false falls through to Parent:: rather than returning NULL outright, matching
// the base class's own contract for that flag (see NewGuiControl::findHitControl()'s doc
// comment: false lets pointer events pass through to whatever's behind THIS control, children
// are still searched regardless) - a non-hit-testable window claiming the grip band anyway would
// silently defeat that contract for the one case a child does need to be reachable underneath it.
NewGuiControl* NewGuiWindow::findHitControl(const Point2I& point)
{
   if (!mVisible || !mActive)
      return NULL;

   if (!mBounds.pointInRect(point))
      return NULL;

   if (mHitTestable && hitTestResizeEdges(point - mBounds.point) != Edge_None)
      return this;

   return Parent::findHitControl(point);
}

NewGuiCursorShape NewGuiWindow::resizeEdgesToCursorShape(U8 edges)
{
   bool horiz = (edges & (Edge_Left | Edge_Right)) != 0;
   bool vert = (edges & (Edge_Top | Edge_Bottom)) != 0;

   if (horiz && vert)
   {
      // NESW: top+right or bottom+left. NWSE: top+left or bottom+right.
      bool topRight = (edges & Edge_Top) && (edges & Edge_Right);
      bool bottomLeft = (edges & Edge_Bottom) && (edges & Edge_Left);
      return (topRight || bottomLeft) ? NewGuiCursorShape::ResizeDiagonalNESW : NewGuiCursorShape::ResizeDiagonalNWSE;
   }

   if (horiz)
      return NewGuiCursorShape::ResizeHorizontal;

   return NewGuiCursorShape::ResizeVertical;   // vert, or nothing (caller already checked Edge_None).
}

void NewGuiWindow::updateHoverState(const Point2I& localPoint)
{
   if (mDragInProgress || mResizeEdges != Edge_None)
      return;   // A drag/resize already owns the cursor for its own duration - don't fight it.

   RectI closeGlyph = getCloseGlyphRect();
   bool newCloseHovered = closeGlyph.extent.x > 0 && closeGlyph.pointInRect(localPoint);

   RectI maximizeGlyph = getMaximizeGlyphRect();
   bool newMaximizeHovered = !newCloseHovered && maximizeGlyph.extent.x > 0 && maximizeGlyph.pointInRect(localPoint);

   RectI minimizeGlyph = getMinimizeGlyphRect();
   bool newMinimizeHovered = !newCloseHovered && !newMaximizeHovered && minimizeGlyph.extent.x > 0 && minimizeGlyph.pointInRect(localPoint);

   U8 newResizeHover = (newCloseHovered || newMaximizeHovered || newMinimizeHovered) ? Edge_None : hitTestResizeEdges(localPoint);

   if (newCloseHovered == mCloseHovered && newMaximizeHovered == mMaximizeHovered
      && newMinimizeHovered == mMinimizeHovered && newResizeHover == mHoverResizeEdges)
      return;   // No transition.

   if (mCloseHovered || mMaximizeHovered || mMinimizeHovered || mHoverResizeEdges != Edge_None)
   {
      popCursor();
      if (mCloseHovered || mMaximizeHovered || mMinimizeHovered)
         setStyleDirty();   // Glyph hover tints are drawn directly, not via the style cascade - need their own repaint trigger.
   }

   if (newCloseHovered || newMaximizeHovered || newMinimizeHovered)
   {
      pushCursor(NewGuiCursorShape::Pointer);
      setStyleDirty();
   }
   else if (newResizeHover != Edge_None)
   {
      pushCursor(resizeEdgesToCursorShape(newResizeHover));
   }

   mCloseHovered = newCloseHovered;
   mMaximizeHovered = newMaximizeHovered;
   mMinimizeHovered = newMinimizeHovered;
   mHoverResizeEdges = newResizeHover;
}

// Divides the device-pixel deviceDelta by the resolved per-axis scale to recover a design-unit
// delta before writing into mWidth/mHeight/mLeft/mTop - those authored fields are design-unit
// values (see resolveAxis()'s doc comment), so a raw device-pixel delta would drift on any
// display whose resolved scale isn't exactly 1.0.
void NewGuiWindow::applyResizeDelta(U8 edges, const Point2I& deviceDelta)
{
   if (edges == Edge_None)
      return;

   F32 scaleX = (mResolvedUIScaleX > 0.0f) ? mResolvedUIScaleX : 1.0f;
   F32 scaleY = (mResolvedUIScaleY > 0.0f) ? mResolvedUIScaleY : 1.0f;

   F32 deltaXDesign = (F32)deviceDelta.x / scaleX;
   F32 deltaYDesign = (F32)deviceDelta.y / scaleY;

   // Current authored values as design-unit floats - only meaningful/used when the corresponding
   // dimension is authored in Pixels, which is the only mode a live resize drag makes sense
   // against (Percent/Auto are re-derived every arrange from their own rule, not draggable).
   F32 curWidth = mWidth.isPixels() ? mWidth.value : (F32)mBounds.extent.x / scaleX;
   F32 curHeight = mHeight.isPixels() ? mHeight.value : (F32)mBounds.extent.y / scaleY;
   F32 curLeft = mLeft.isPixels() ? mLeft.value : (F32)mBounds.point.x / scaleX;
   F32 curTop = mTop.isPixels() ? mTop.value : (F32)mBounds.point.y / scaleY;

   F32 newWidth = curWidth;
   F32 newLeft = curLeft;
   if (edges & Edge_Right)
      newWidth = getMax(mMinWidth, curWidth + deltaXDesign);
   else if (edges & Edge_Left)
   {
      newWidth = getMax(mMinWidth, curWidth - deltaXDesign);
      // Clamp so the RIGHT edge stays fixed even once width has hit the floor - avoids the
      // window silently sliding right once the drag has pushed past the minimum width.
      F32 actualDeltaX = curWidth - newWidth;
      newLeft = curLeft + actualDeltaX;
   }

   F32 newHeight = curHeight;
   F32 newTop = curTop;
   if (edges & Edge_Bottom)
      newHeight = getMax(mMinHeight, curHeight + deltaYDesign);
   else if (edges & Edge_Top)
   {
      newHeight = getMax(mMinHeight, curHeight - deltaYDesign);
      F32 actualDeltaY = curHeight - newHeight;
      newTop = curTop + actualDeltaY;
   }

   mWidth = NewGuiDimension::fromPixels(newWidth);
   mHeight = NewGuiDimension::fromPixels(newHeight);
   if (edges & Edge_Left)
      mLeft = NewGuiDimension::fromPixels(newLeft);
   if (edges & Edge_Top)
      mTop = NewGuiDimension::fromPixels(newTop);

   setContentDirty();
   setArrangementDirty();
}

void NewGuiWindow::bringToFront()
{
   // Paint order: elevateToFront() now stamps this window's ENTIRE content subtree - not just
   // this window's own mRenderLayer - to one shared, freshly-issued override in a single
   // recursive call (see NewGuiControl::elevateToFront()'s own doc comment). No per-child
   // plumbing needed here, and no separate correction pass required on ArrangePass() either,
   // since every descendant now resolves its OWN override normally next arrange, same as any
   // other authored renderLayerOverride.
   elevateToFront();

   SimGroup* parent = getGroup();
   if (!parent)
      return;

   // Hit-test order: pushes this window to the END of the parent's child list. Despite the
   // name, SimSet's own pushObjectToBack(obj) == reOrder(obj, NULL) does exactly that (see
   // SimSet::reOrder()'s "no target -> push_back" branch) - and LAST in tree order is what
   // hit-tests on TOP per NewGuiControl::findHitControl()'s back-to-front walk.
   // bringObjectToFront()/front() would do the opposite here. Kept alongside elevateToFront()
   // (rather than relying on paint order alone) so a click that lands on the newly-overlapping
   // region of two windows resolves to whichever one visually just came to front, not whichever
   // happens to have a numerically higher mRenderLayer for reasons unrelated to this press.
   parent->reOrder(this, NULL);
}

// Priority order for a titlebar-area press: close glyph, then maximize glyph, then minimize
// glyph, then resize edges, then drag. Each glyph check is scoped to ITS OWN rect only - a press
// between two adjacent glyphs (in the margin) falls through to the next check rather than being
// claimed by whichever glyph happens to be checked first.
void NewGuiWindow::onMouseDown(NewGuiInputEvent& event)
{
   bringToFront();

   RectI closeGlyph = getCloseGlyphRect();
   if (closeGlyph.extent.x > 0 && closeGlyph.pointInRect(event.localPoint))
   {
      mCloseArmed = true;
      event.handled = true;
      return;
   }

   RectI maximizeGlyph = getMaximizeGlyphRect();
   if (maximizeGlyph.extent.x > 0 && maximizeGlyph.pointInRect(event.localPoint))
   {
      mMaximizeArmed = true;
      event.handled = true;
      return;
   }

   RectI minimizeGlyph = getMinimizeGlyphRect();
   if (minimizeGlyph.extent.x > 0 && minimizeGlyph.pointInRect(event.localPoint))
   {
      mMinimizeArmed = true;
      event.handled = true;
      return;
   }

   U8 edges = hitTestResizeEdges(event.localPoint);
   if (edges != Edge_None)
   {
      mResizeEdges = edges;
      mResizeLastPoint = event.screenPoint;
      pushCursor(resizeEdgesToCursorShape(edges));   // Popped in onMouseUp() when mResizeEdges clears.
      event.handled = true;
      return;
   }

   RectI titlebar = getTitlebarRect();
   if (mMovable && !mMaximized && titlebar.extent.y > 0 && titlebar.pointInRect(event.localPoint))
   {
      mDragInProgress = true;
      mDragLastPoint = event.screenPoint;
      event.handled = true;
      return;
   }

   // Landed on the body below the titlebar - deliberately left unhandled so it can keep bubbling
   // to a normal child in the content area, same as NewGuiScroll's content-area press.
   Parent::onMouseDown(event);
}

void NewGuiWindow::onMouseUp(NewGuiInputEvent& event)
{
   if (mCloseArmed)
   {
      mCloseArmed = false;

      RectI closeGlyph = getCloseGlyphRect();
      if (closeGlyph.extent.x > 0 && closeGlyph.pointInRect(event.localPoint))
         performClose();

      event.handled = true;
      return;
   }

   if (mMaximizeArmed)
   {
      mMaximizeArmed = false;

      RectI maximizeGlyph = getMaximizeGlyphRect();
      if (maximizeGlyph.extent.x > 0 && maximizeGlyph.pointInRect(event.localPoint))
      {
         if (mMaximized)
            performRestore();
         else
            performMaximize();
      }

      event.handled = true;
      return;
   }

   if (mMinimizeArmed)
   {
      mMinimizeArmed = false;

      RectI minimizeGlyph = getMinimizeGlyphRect();
      if (minimizeGlyph.extent.x > 0 && minimizeGlyph.pointInRect(event.localPoint))
      {
         if (mMinimized)
            performRestore();
         else
            performMinimize();
      }

      event.handled = true;
      return;
   }

   if (mResizeEdges != Edge_None)
   {
      mResizeEdges = Edge_None;
      popCursor();
      updateHoverState(event.localPoint);
      onWindowMoved_callback();
      event.handled = true;
      return;
   }

   if (mDragInProgress)
   {
      mDragInProgress = false;
      onWindowMoved_callback();
      event.handled = true;
      return;
   }

   Parent::onMouseUp(event);
}

void NewGuiWindow::onMouseLeave(NewGuiInputEvent& event)
{
   if (mCloseHovered || mMaximizeHovered || mMinimizeHovered || mHoverResizeEdges != Edge_None)
   {
      popCursor();
      mCloseHovered = false;
      mMaximizeHovered = false;
      mMinimizeHovered = false;
      mHoverResizeEdges = Edge_None;
      setStyleDirty();
   }

   Parent::onMouseLeave(event);
}

void NewGuiWindow::onInputEvent(NewGuiInputEvent& event)
{
   if (event.action != NewGuiInputAction::Move)
      return;

   if (mResizeEdges != Edge_None)
   {
      Point2I delta = event.screenPoint - mResizeLastPoint;
      mResizeLastPoint = event.screenPoint;
      applyResizeDelta(mResizeEdges, delta);
      event.handled = true;
      return;
   }

   if (mDragInProgress)
   {
      Point2I deviceDelta = event.screenPoint - mDragLastPoint;
      mDragLastPoint = event.screenPoint;

      F32 scaleX = (mResolvedUIScaleX > 0.0f) ? mResolvedUIScaleX : 1.0f;
      F32 scaleY = (mResolvedUIScaleY > 0.0f) ? mResolvedUIScaleY : 1.0f;

      // Same design-unit conversion as applyResizeDelta() - mLeft/mTop are authored design-unit
      // values, dragging must accumulate in that same space rather than device pixels.
      F32 curLeft = mLeft.isPixels() ? mLeft.value : (F32)mBounds.point.x / scaleX;
      F32 curTop = mTop.isPixels() ? mTop.value : (F32)mBounds.point.y / scaleY;

      mLeft = NewGuiDimension::fromPixels(curLeft + (F32)deviceDelta.x / scaleX);
      mTop = NewGuiDimension::fromPixels(curTop + (F32)deviceDelta.y / scaleY);

      setArrangementDirty();
      event.handled = true;
      return;
   }

   updateHoverState(event.localPoint);
}

// Resolves normally via Parent:: UNLESS mMaximized, in which case mBounds is set directly to
// slotRect (the parent's own offered rect) and left/top/width/height resolution is skipped
// entirely - see the doc comment on NewGuiWindow.h for why this is re-applied fresh every
// arrange rather than captured once in performMaximize().
//
// Still needs mResolvedUIScaleX/Y, mRenderLayer, GetClientRect()/child arrangement to happen
// exactly the way Parent::ArrangePass() already does them - only the bounds-resolution step
// itself is replaced, so this mirrors Parent::ArrangePass()'s own body rather than calling it
// and then overwriting mBounds after the fact (which would arrange children against the WRONG
// slot on the one frame maximize/restore actually changes size).
void NewGuiWindow::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   if (!mMaximized)
   {
      Parent::ArrangePass(slotRect, parentRenderLayer, uiScaleX, uiScaleY);
      return;
   }

   mResolvedUIScaleX = uiScaleX;
   mResolvedUIScaleY = uiScaleY;

   mBounds = slotRect;
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

void NewGuiWindow::performClose()
{
   onClose_callback();
   setVisible(false);
}

// Captures the current authored height, then sets height to exactly the titlebar's own height
// (design units) so the window collapses to just its titlebar - see GetClientRect()'s own note
// on why no separate zero-client-area special case is needed once mBounds.extent.y shrinks that far.
void NewGuiWindow::performMinimize()
{
   if (mMinimized)
      return;

   mPreMinimizeHeight = mHeight;
   mHeight = NewGuiDimension::fromPixels(mTitlebarHeight);
   mMinimized = true;

   onMinimize_callback();

   setContentDirty();
   setArrangementDirty();
}

// Captures the current authored left/top/width/height, then sets mMaximized so ArrangePass()
// re-derives the fill-parent rect fresh every arrange (see that override's own doc comment).
// Also implicitly disables drag (onMouseDown() checks !mMaximized) and resize
// (hitTestResizeEdges() checks !mMaximized) for the duration, per this control's documented
// standard-OS-window behavior.
void NewGuiWindow::performMaximize()
{
   if (mMaximized)
      return;

   mPreMaximizeLeft = mLeft;
   mPreMaximizeTop = mTop;
   mPreMaximizeWidth = mWidth;
   mPreMaximizeHeight = mHeight;
   mMaximized = true;

   onMaximize_callback();

   setContentDirty();
   setArrangementDirty();
}

// Reverts whichever of mMinimized/mMaximized is currently set (independently - see mMaximized's
// own doc comment on NewGuiWindow.h for why a window can conceivably have both set at once),
// restoring the corresponding captured pre-* dimension(s). No-op (no callback fired) if neither
// flag is set.
void NewGuiWindow::performRestore()
{
   if (!mMinimized && !mMaximized)
      return;

   if (mMinimized)
   {
      mHeight = mPreMinimizeHeight;
      mMinimized = false;
   }

   if (mMaximized)
   {
      mLeft = mPreMaximizeLeft;
      mTop = mPreMaximizeTop;
      mWidth = mPreMaximizeWidth;
      mHeight = mPreMaximizeHeight;
      mMaximized = false;
   }

   onRestore_callback();

   setContentDirty();
   setArrangementDirty();
}

DefineEngineMethod(NewGuiWindow, bringToFront, void, (), ,
   "Reparents this window to the end of its parent's child list, so it paints and hit-tests above every sibling.")
{
   object->bringToFront();
}

DefineEngineMethod(NewGuiWindow, performClose, void, (), ,
   "Runs the same close path a real click on the close glyph would: fires onClose(), then hides the window.")
{
   object->performClose();
}

DefineEngineMethod(NewGuiWindow, performMinimize, void, (), ,
   "Collapses this window to just its titlebar, capturing the current height to restore later. No-op if already minimized.")
{
   object->performMinimize();
}

DefineEngineMethod(NewGuiWindow, performMaximize, void, (), ,
   "Snaps this window to fill its parent's client rect, capturing the current bounds to restore later, and disables drag/resize until restored. No-op if already maximized.")
{
   object->performMaximize();
}

DefineEngineMethod(NewGuiWindow, performRestore, void, (), ,
   "Reverts a minimize and/or maximize, restoring the captured pre-minimize/pre-maximize bounds. No-op if neither is active.")
{
   object->performRestore();
}

DefineEngineMethod(NewGuiWindow, isMinimized, bool, (), ,
   "True if this window is currently minimized.")
{
   return object->isMinimized();
}

DefineEngineMethod(NewGuiWindow, isMaximized, bool, (), ,
   "True if this window is currently maximized.")
{
   return object->isMaximized();
}

DefineEngineMethod(NewGuiWindow, setTitleText, void, (const char* text), ,
   "Sets the titlebar's label text.")
{
   object->setTitleText(text);
}
