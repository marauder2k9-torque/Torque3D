//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiStack.h
//-----------------------------------------------------------------------------
#ifndef _NEWGUISTACK_H_
#define _NEWGUISTACK_H_

#ifndef _NEWGUICONTROL_H_
#include "gui_rev2/core/newGuiControl.h"
#endif
#include <functional>   // std::function - RecomputeContentExtentAndReclamp()'s reArrangeSelf callback.

/// Lays children out single-file along one axis (a row or a column).
/// EVERY child's size on BOTH axes - stacking and cross - is resolved
/// from that child's own authored width/height (Auto = preferred size,
/// Percent = share of the stack's own resolved extent on that axis,
/// Pixels = literal), exactly as it would resolve outside a Stack - a
/// Stack changes placement (where the child's rect sits, and which axis
/// consecutive children advance along), never a child's own sizing
/// rules. A child only fills the cross axis if it actually authored
/// "100%" there; height="auto"/width="auto" on the cross axis sizes to
/// that child's own preferred length on that axis, same as anywhere
/// else - it does NOT stretch to fill by default. `spacing` adds a
/// fixed gap between consecutive visible children (never before the
/// first or after the last). An invisible child (mVisible == false) is
/// skipped entirely: no slot, no spacing gap either side.
///
/// Unlike the base GetChildSlot() flow (which offers each child a slot
/// and lets it resolve left/top/right/bottom against that slot), a
/// Stack has already fully decided every child's final rect itself, so
/// children are placed via ArrangePassWithFixedExtent() rather than
/// ArrangePass() - see that method's doc comment on NewGuiControl.
/// left/top/right/bottom are therefore not honored on stacked children.
///
/// @code
/// new NewGuiStack( MyRow )
/// {
///    axis = "horizontal";
///    spacing = "8";
///    width = "100%"; height = "auto";
///
///    new NewGuiControl() { width = "40"; height = "40"; };       // fixed 40px x 40px
///    new NewGuiControl() { width = "auto"; height = "100%"; };   // auto width, fills MyRow's own height
/// };
/// @endcode
///
/// @note There is no "fill remaining space" sizing mode distinct from
/// Percent - Percent along either axis resolves against THIS STACK'S
/// OWN resolved extent on that axis (same as GetChildSlot()'s
/// slot.extent would have given a non-stacked child), not against space
/// left over after siblings. Two 100%-wide children in a horizontal
/// stack will overlap, exactly as two 100%-wide children placed via the
/// base top-to-bottom flow would overlap if that flow were horizontal.
/// This mirrors the rest of the layout system's existing Percent
/// semantics rather than introducing a new flex-distribution concept.
///
/// @note height="auto" (or width="auto") on the STACK ITSELF is
/// resolved from ComputePreferredSize(), which unions children's
/// PREFERRED sizes - independent of this per-child arrange step above.
/// A stack whose own cross-axis dimension is "auto" therefore sizes to
/// its largest child's preferred cross-axis length on its own, with no
/// circular dependency: children needing "100%" cross-fill should sit
/// inside a stack whose own cross-axis size is fixed or Percent, not Auto.
///
/// `align` controls each child's position across the CROSS axis, within
/// whatever cross-axis space it doesn't fill itself: start (default),
/// center, or end. Has no visible effect on a child authoring "100%" on
/// the cross axis, since it already fills the full extent.
///
/// `justify` controls how children are distributed along the STACKING
/// axis: start (default, packed at the leading edge), center, end,
/// spaceBetween (leftover space divided only between children), or
/// spaceAround (leftover space divided around every child, including
/// half-shares at both ends). Only visible when this stack's own
/// stacking-axis extent is Pixels/Percent and ends up larger than its
/// children need - a stacking axis authored "auto" has no leftover
/// space by definition, so justify has nothing to distribute.
///
/// `wrap`, when true, starts a new line (along the cross axis) whenever
/// the next child would overflow this stack's stacking-axis extent,
/// instead of overflowing - like flexbox wrap. `spacing` applies both
/// between children within a line AND between lines. `align`/`justify`
/// both apply per-line: align is each child's position within its own
/// line's cross-axis thickness; justify is each line's own distribution
/// of leftover stacking-axis space, independently of other lines. A
/// wrapped stack's own cross-axis "auto" size is the sum of every
/// line's thickness (plus spacing between lines), not just one line's.
class NewGuiStack : public NewGuiControl
{
public:

   typedef NewGuiControl Parent;

   enum StackAxis : U8
   {
      StackAxis_Vertical = 0,     ///< Children flow top-to-bottom (a column). Default.
      StackAxis_Horizontal,       ///< Children flow left-to-right (a row).
   };

   /// Where each child sits across the CROSS axis
   enum StackAlign : U8
   {
      StackAlign_Start = 0,    ///< Flush to the leading edge of the cross axis (top for a row, left for a column). Default.
      StackAlign_Center,       ///< Centered across the cross axis.
      StackAlign_End,          ///< Flush to the trailing edge of the cross axis (bottom for a row, right for a column).
   };

   /// How children are distributed along the STACKING axis when they don't fill it
   enum StackJustify : U8
   {
      StackJustify_Start = 0,     ///< Children packed at the leading edge; leftover space trails after the last child. Default.
      StackJustify_Center,        ///< Children packed together, centered as a block within the leftover space.
      StackJustify_End,           ///< Children packed at the trailing edge; leftover space leads before the first child.
      StackJustify_SpaceBetween,  ///< Leftover space divided evenly BETWEEN children only - none before the first or after the last. No effect with 0 or 1 visible children.
      StackJustify_SpaceAround,   ///< Leftover space divided evenly around every child, including half-shares before the first and after the last.
   };

protected:

   StackAxis mAxis;         ///< Authored - which way children stack.
   StackAlign mAlign;       ///< Authored - cross-axis alignment of each child within space it doesn't fill itself.
   StackJustify mJustify;   ///< Authored - stacking-axis distribution of children within this stack's own extent.
   S32 mSpacing;            ///< Authored - fixed pixel gap between consecutive visible children (or consecutive lines, when wrap is on).
   bool mWrap;              ///< Authored - when true, a child that would overflow the stacking axis starts a new line instead.

   static bool _setAxis(void* obj, const char* index, const char* data);
   static bool _setAlign(void* obj, const char* index, const char* data);
   static bool _setJustify(void* obj, const char* index, const char* data);
   static bool _setSpacing(void* obj, const char* index, const char* data);
   static bool _setWrap(void* obj, const char* index, const char* data);

   /// One row (horizontal axis) or column (vertical axis) of children when mWrap is on - a
   /// contiguous run of children whose combined stacking-axis length (plus spacing between them)
   /// fits within this stack's own stacking-axis extent. Without mWrap, there is always exactly
   /// one Line containing every visible child.
   struct Line
   {
      U32 firstChildIndex;      ///< Index (among visible children only) of this line's first child.
      U32 childCount;           ///< Number of visible children on this line.
      S32 stackExtent;          ///< Sum of this line's children's stacking-axis lengths, plus spacing between them - NOT including leftover/justify space.
      S32 crossExtent;          ///< Max of this line's children's cross-axis lengths - this line's own thickness.
   };

   /// Shared by ArrangePass()/ArrangePassWithFixedExtent(): splits visible children into one or
   /// more Lines (see Line and mWrap), then places every child within its line
   /// @param clientRect This control's own already-resolved client rect (post padding).
   /// @param uiScaleX Horizontal design-to-device scale, forwarded to children.
   /// @param uiScaleY Vertical design-to-device scale, forwarded to children.
   void layoutChildren(const RectI& clientRect, F32 uiScaleX, F32 uiScaleY);

   /// Splits visible children (in tree order) into one or more Lines: a new line starts whenever
   /// wrap is on and adding the next child would exceed clientRect's stacking-axis extent 
   /// @param clientRect This control's own already-resolved client rect (post padding).
   /// @param uiScaleX Horizontal design-to-device scale.
   /// @param uiScaleY Vertical design-to-device scale.
   /// @param outVisibleChildren Receives every visible child, in tree order - Lines index into this.
   /// @return The computed lines, in stacking order (wrap axis: top-to-bottom or left-to-right).
   Vector<Line> splitIntoLines(const RectI& clientRect, F32 uiScaleX, F32 uiScaleY, Vector<NewGuiControl*>& outVisibleChildren) const;

   /// Mirrors NewGuiScroll::RecomputeContentExtentAndReclamp()'s shape: after children are laid
   /// out, compares what they ACTUALLY resolved to on the stacking axis against what this
   /// control's own mBounds was resolved against
   /// @param reArrangeSelf Callback that re-establishes this control's own mBounds/mRenderLayer
   /// (whatever ArrangePass()/ArrangePassWithFixedExtent() normally does before laying out
   /// children), so layoutChildren() can be safely re-run against corrected state.
   void RecomputeContentExtentAndReclamp(const std::function<void()>& reArrangeSelf);

public:

   NewGuiStack();
   virtual ~NewGuiStack();

   DECLARE_CONOBJECT(NewGuiStack);

   static void initPersistFields();

   /// Reimplements NewGuiControl::ComputePreferredSize()'s standard union-of-children default,
   /// generalized to whichever axis is this stack's STACKING axis (vertical reduces to exactly
   /// the base behavior)
   Point2I ComputePreferredSize() override;

   /// Resolves this control's own bounds normally via Parent::, then lays out children via layoutChildren().
   void ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   /// Mirrors ArrangePass() above for the fixed-extent entry point, so
   /// a Stack nested inside another Stack (or any other fixed-extent
   /// placer) lays out its own children correctly either way.
   void ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY) override;

   StackAxis getAxis() const { return mAxis; }
   StackAlign getAlign() const { return mAlign; }
   StackJustify getJustify() const { return mJustify; }
   S32 getSpacing() const { return mSpacing; }
   bool getWrap() const { return mWrap; }
};

#endif // _NEWGUISTACK_H_
