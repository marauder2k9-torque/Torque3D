//-----------------------------------------------------------------------------
// guiScrollCtrlNew.h
//
// GuiScrollCtrlNew -- vertically (and optionally horizontally) scrollable
// viewport around one or more content children, with a drawn scrollbar
// (track + draggable thumb, no separate GuiButtonCtrlNew children
// involved).
// Thumb appearance (including hover/active variance) comes from a
// dedicated 'thumbStyle' GuiStyle reference, resolved against a
// thumb-specific interaction-state mask -- see mThumbStyle's doc comment
// and _getVThumbStyleStateMask()/_getHThumbStyleStateMask(). Track color
// comes from this control's own ordinary 'style'.
//-----------------------------------------------------------------------------

#ifndef _GUISCROLLCTRLNEW_H_
#define _GUISCROLLCTRLNEW_H_

#ifndef _GUICONTROLNEW_H_
#include "gui_refactor/core/guiControlNew.h"
#endif

class GuiRenderBatch;

/// Which axes this control allows scrolling on. Bitmask so vertical-only
/// (the common case -- text logs, lists) doesn't pay for horizontal
/// scrollbar-hit-testing/rendering it'll never use.
enum GuiScrollBarMode : U8
{
   GuiScrollBarMode_None = 0,
   GuiScrollBarMode_Vertical = 1 << 0,
   GuiScrollBarMode_Horizontal = 1 << 1,
   GuiScrollBarMode_Both = GuiScrollBarMode_Vertical | GuiScrollBarMode_Horizontal
};

DefineEnumType(GuiScrollBarMode);

class GuiScrollCtrlNew : public GuiControlNew
{
public:

   typedef GuiControlNew Parent;

protected:

   GuiScrollBarMode mScrollBarMode;

   /// Style consulted for the thumb's fill/border, resolved with a
   /// SEPARATE state mask from this control's own getCurrentStyleStateMask()
   /// -- see _getThumbStyleStateMask().
   GuiStyle* mThumbStyle;

   /// Current scroll offset, logical units, always >= 0 and clamped to
   /// [0, maxScroll] by _updateScrollRanges() -- see that function for
   /// how maxScroll itself is derived from content vs. viewport size.
   S32 mScrollOffsetX;
   S32 mScrollOffsetY;

   /// Cached each _updateScrollRanges() call (content resize, this
   /// control's own resize, or child add/remove)
   S32 mMaxScrollX;
   S32 mMaxScrollY;
   S32 mContentWidth;
   S32 mContentHeight;

   /// Top-left corner of the union of every child's authored rect (see
   /// mChildAnchors)
   Point2I mContentOrigin;

   /// One entry per current child
   struct ChildAnchor
   {
      GuiControlNew* child;
      Point2I authoredPosition;
   };
   Vector< ChildAnchor > mChildAnchors;

   /// Fixed thickness of the scrollbar track, logical units.
   S32 mScrollBarThickness;

   /// Pixels scrolled per wheel notch / arrow-key press.
   S32 mWheelStep;

   /// True while the user has the vertical/horizontal thumb actively
   /// grabbed
   bool mDraggingVThumb;
   bool mDraggingHThumb;

   S32 mThumbDragGrabOffset;
   bool mScrollRangesDirty;

   /// Recomputes mContentWidth/Height and mMaxScrollX/Y from the UNION of
   /// every current child's authored rect (see mChildAnchors)
   void _updateScrollRanges();

   /// Lazy wrapper -- see mScrollRangesDirty's doc comment. Every
   /// render/input path that reads mMaxScrollX/Y, mContentWidth/Height,
   /// or any of the track/thumb rect helpers below calls this FIRST.
   void _updateScrollRangesIfDirty();

   /// Applies mScrollOffsetX/Y to every child's actual position
   void _applyContentPosition();

   /// Device-pixel rect of the vertical/horizontal scrollbar track,
   /// relative to this control's own onRender() offset -- used by both
   /// onRender() (to draw it) and onMouseDown()/hit-testing (to know
   /// what was clicked). Returns an empty RectI if that axis isn't
   /// showing a bar (mode disabled, or content doesn't overflow that
   /// axis -- see shouldShowVBar()/shouldShowHBar()).
   RectI _getVTrackDeviceRect(const RectI& ctrlDeviceRect) const;
   RectI _getHTrackDeviceRect(const RectI& ctrlDeviceRect) const;

   /// Device-pixel rect of the thumb itself within its track --
   /// position/size both derived from mScrollOffsetY/mMaxScrollY (or
   /// X/maxScrollX) and mContentHeight/Width vs. the viewport, the
   /// standard "thumb size proportional to visible fraction" formula.
   RectI _getVThumbDeviceRect(const RectI& ctrlDeviceRect) const;
   RectI _getHThumbDeviceRect(const RectI& ctrlDeviceRect) const;

   bool shouldShowVBar() const { return (mScrollBarMode & GuiScrollBarMode_Vertical) && mMaxScrollY > 0; }
   bool shouldShowHBar() const { return (mScrollBarMode & GuiScrollBarMode_Horizontal) && mMaxScrollX > 0; }

   /// Derives a GuiStyleState bitmask for the THUMB specifically
   U32 _getVThumbStyleStateMask(const RectI& ctrlDeviceRect);
   U32 _getHThumbStyleStateMask(const RectI& ctrlDeviceRect);

   static bool setThumbStyleProt(void* object, const char* index, const char* data);

public:

   GuiScrollCtrlNew();

   static void initPersistFields();

   DECLARE_CONOBJECT(GuiScrollCtrlNew);
   DECLARE_CATEGORY("Gui Containers");
   DECLARE_DESCRIPTION("Scrollable viewport around one or more content children, with a drawn scrollbar.");

   bool onWake() override;
   void onSleep() override;
   void onRemove() override;
   void onDeleteNotify(SimObject* object) override;

   void setThumbStyle(GuiStyle* style);
   GuiStyle* getThumbStyle() const { return mThumbStyle; }

   void onChildAdded(GuiControlNew* child) override;
   void onChildRemoved(GuiControlNew* child) override;

   bool resize(const Point2I& newPosition, const Point2I& newExtent) override;

   /// The region a child should resolve auto/percent width or height
   /// against -- this control's own extent MINUS whichever scrollbar
   /// gutter(s) are reserved by mScrollBarMode (see this method's own
   /// .cpp doc comment for why the reservation is unconditional on mode
   /// rather than keyed off shouldShowVBar()/shouldShowHBar()). This is
   /// what makes a plain, unconfigured child (auto width -- the default
   /// every control starts with) genuinely confined to the visible/
   /// reachable viewport instead of extending underneath this control's
   /// own scrollbar track/thumb.
   Point2I getClientExtent() const override;

   /// @name Script/engine scroll API
   /// @{

   /// Absolute scroll position, logical units, clamped to
   /// [0, maxScroll]
   void setScrollOffset(S32 x, S32 y);
   Point2I getScrollOffset() const { return Point2I(mScrollOffsetX, mScrollOffsetY); }

   /// Scrolls the vertical offset to its maximum (bottom) -- the
   /// common "follow new content" case. Horizontal is left alone;
   /// there's no equivalent "follow the right edge" use case this
   /// needs to support.
   void scrollToBottom() { setScrollOffset(mScrollOffsetX, mMaxScrollY); }
   void scrollToTop() { setScrollOffset(mScrollOffsetX, 0); }

   Point2I getMaxScrollOffset() const { return Point2I(mMaxScrollX, mMaxScrollY); }

   /// Content extent as last measured by _updateScrollRanges() -- see
   /// mContentWidth/mContentHeight's doc comments.
   Point2I getContentExtent() const { return Point2I(mContentWidth, mContentHeight); }
   /// @}

   bool onKeyDown(const GuiEvent& event) override;
   bool onMouseWheelUp(const GuiEvent& event) override;
   bool onMouseWheelDown(const GuiEvent& event) override;

   void onMouseDown(const GuiEvent& event) override;
   void onMouseDragged(const GuiEvent& event) override;
   void onMouseUp(const GuiEvent& event) override;

   void onRender(Point2I offset, const RectI& updateRect) override;
};

#endif // _GUISCROLLCTRLNEW_H_
