//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiScroll.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUISCROLL_H_
#define _NEWGUISCROLL_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#include <functional>   // std::function - RecomputeContentExtentAndReclamp()'s reArrangeSelf callback.

/// A scrollable viewport: children lay out normally (and may exceed the
/// viewport), while GetClientRect() shifts by -scrollOffset and narrows
/// for a gutter on whichever axis needs a scrollbar. Both mouse wheel
/// and touch/click-drag scroll the content; the scrollbar thumbs
/// themselves are drawn and hit-tested directly against this control's
/// own bounds, with no separate scrollbar object.
///
/// @code
/// new NewGuiScroll( MyScroll )
/// {
///    width = "100%"; height = "300";
///    scrollBarMode = "vertical";
///    // children added normally - content taller than 300px scrolls.
/// };
/// @endcode
class NewGuiScroll : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

   enum ScrollBarMode : U8
   {
      ScrollBar_None = 0,
      ScrollBar_Vertical,
      ScrollBar_Horizontal,
      ScrollBar_Both,
   };

protected:

   ScrollBarMode mScrollBarMode;    ///< Authored - which axes are allowed to scroll.
   Point2I mScrollOffset;           ///< Runtime scroll position, in content-space pixels. Always clamped non-negative.
   S32 mScrollBarThickness;         ///< Shared thickness (px) for both gutters.

   /// Natural (unclamped) content extent, computed once per
   /// ArrangePass from the union of every child's resolved bounds.
   /// Compared against mBounds.extent to decide scrollbar visibility and mScrollOffset's clamp range.
   Point2I mContentExtent;

   /// Content drag-to-scroll state (mouse-drag or touch on the
   /// content area itself, not the scrollbar). True from onMouseDown()
   /// until the matching onMouseUp(); while true, Move events are
   /// interpreted as drag deltas.
   ///
   /// @note A drag only starts if the press lands directly on this
   /// control's content area - if it lands on a child that claims
   /// the event first, this control never finds out.
   bool mDragInProgress;
   Point2I mDragLastPoint;   ///< screenPoint as of the last drag update; each Move's delta from this is applied, then updated.

   /// Which scrollbar axis (if any) a thumb drag is active on.
   enum ScrollAxis : U8
   {
      ScrollAxis_None = 0,
      ScrollAxis_Vertical,
      ScrollAxis_Horizontal,
   };
   ScrollAxis mThumbDragAxis;

   /// localPoint's component along mThumbDragAxis at drag start - the
   /// total delta is re-derived from this fixed point every Move,
   /// rather than accumulating per-Move deltas, to avoid compounding rounding error.
   S32 mThumbDragStartAxisPos;
   S32 mThumbDragStartScrollOffset;   ///< mScrollOffset's component along mThumbDragAxis, captured at drag start.

   ScrollAxis mThumbHoverAxis;   ///< Which thumb the pointer currently hovers, for hover-cursor push/pop on transition.

   static bool _setScrollBarMode(void* obj, const char* index, const char* data);

   /// Clamps mScrollOffset into [0, max(0, contentExtent - viewportExtent)] on scrollable axes;
   /// zeroes any axis where scrolling is disabled.
   void clampScrollOffset();

   /// Re-hit-tests localPoint against both thumb rects and push/pops the pointer cursor on
   /// hover transition only (never every Move while already inside). Skipped while a drag is active.
   void updateThumbHover(const Point2I& localPoint);

   bool needsVerticalScrollbar() const;
   bool needsHorizontalScrollbar() const;

   /// @return The vertical gutter's rect in this control's local space (0,0 = mBounds.point).
   RectI getVerticalGutterRect() const;

   /// @return The horizontal gutter's rect in this control's local space.
   RectI getHorizontalGutterRect() const;

   /// Proportional thumb rect for a gutter: thumb length / track length == viewport extent / content extent.
   /// @param gutterRect Gutter to compute the thumb within, in the same local space.
   /// @param vertical True for the vertical gutter, false for horizontal.
   RectI computeThumbRect(const RectI& gutterRect, bool vertical) const;

public:

   NewGuiScroll();
   virtual ~NewGuiScroll();

   DECLARE_CONOBJECT(NewGuiScroll);

   static void initPersistFields();

   /// Shifts by -scrollOffset and narrows for any active gutter.
   RectI GetClientRect() const override;

   /// Does not grow to fit content - a scroll viewport's own preferred size stays independent of it.
   Point2I ComputePreferredSize() override;

   /// Recomputes mContentExtent from children's resolved bounds after Parent's own arrangement, then reclamps.
   void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// REQUIRED override - without it, a Stack-managed scroll (placed
   /// via ArrangePassWithFixedExtent(), not ArrangePass()) would never
   /// recompute mContentExtent, leaving it permanently un-scrollable.
   void ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// The mContentExtent recompute-and-reclamp logic shared by
   /// ArrangePass() and ArrangePassWithFixedExtent(), so the two entry
   /// points can never drift apart on it.
   /// @param reArrangeSelf Callback that re-runs whichever entry point called this, if content extent changed.
   void RecomputeContentExtentAndReclamp(const std::function<void()>& reArrangeSelf);

   /// Installs a clip rect for the viewport around Parent's own child recursion.
   void renderChildControls(NewGuiRenderBatch* batch) override;

   /// Draws background/border, then whichever thumb(s) are currently showing.
   void EmitDrawCommands(NewGuiRenderBatch* batch, const RectI& bounds, const NewGuiResolvedStyle& style, S32 layer) override;

   /// Mouse wheel scrolls; Move drives both content-drag and thumb-drag.
   void onInputEvent(NewGuiInputEvent& event) override;

   /// Thumb -> starts a thumb drag. Gutter above/left of thumb -> page toward start; below/right -> page toward end.
   /// Content area -> starts a content drag. A gutter press always claims the event; a content-area press does not.
   void onMouseDown(NewGuiInputEvent& event) override;
   void onMouseUp(NewGuiInputEvent& event) override;

   /// Pops any thumb-hover cursor push if the pointer leaves the control while a thumb was hovered.
   void onMouseLeave(NewGuiInputEvent& event) override;

   void setScrollOffset(const Point2I& offset);
   const Point2I& getScrollOffset() const { return mScrollOffset; }

   const Point2I& getContentExtent() const { return mContentExtent; }

   /// Scrolls one "page" (viewport extent minus a small overlap) in the given direction.
   void pageUp();
   void pageDown();
   void pageLeft();
   void pageRight();
};

#endif // _NEWGUISCROLL_H_
