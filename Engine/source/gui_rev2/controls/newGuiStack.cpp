//-----------------------------------------------------------------------------
// gui_rev2/controls/newGuiStack.cpp
//-----------------------------------------------------------------------------
#include "platform/platform.h"
#include "console/console.h"
#include "console/consoleTypes.h"
#include "console/consoleInternal.h"
#include "console/engineAPI.h"
#include "gui_rev2/controls/newGuiStack.h"

IMPLEMENT_CONOBJECT(NewGuiStack);

NewGuiStack::NewGuiStack()
   : mAxis(StackAxis_Vertical),
   mAlign(StackAlign_Start),
   mJustify(StackJustify_Start),
   mSpacing(0),
   mWrap(false)
{
}

NewGuiStack::~NewGuiStack()
{
}

bool NewGuiStack::_setAxis(void* obj, const char* index, const char* data)
{
   NewGuiStack* stack = static_cast<NewGuiStack*>(obj);

   StackAxis axis = StackAxis_Vertical;
   if (dStricmp(data, "horizontal") == 0)
      axis = StackAxis_Horizontal;
   else if (dStricmp(data, "vertical") == 0)
      axis = StackAxis_Vertical;
   else
      axis = StackAxis_Vertical;

   stack->mAxis = axis;

   // Stacking direction changes every child's slot - re-measure (cross/stacking axis union
   // swaps) and re-arrange.
   stack->setContentDirty();
   stack->setArrangementDirty();
   return false;
}

bool NewGuiStack::_setSpacing(void* obj, const char* index, const char* data)
{
   NewGuiStack* stack = static_cast<NewGuiStack*>(obj);

   stack->mSpacing = getMax(0, dAtoi(data));

   stack->setContentDirty();
   stack->setArrangementDirty();
   return false;
}

bool NewGuiStack::_setAlign(void* obj, const char* index, const char* data)
{
   NewGuiStack* stack = static_cast<NewGuiStack*>(obj);

   StackAlign align = StackAlign_Start;
   if (dStricmp(data, "center") == 0)
      align = StackAlign_Center;
   else if (dStricmp(data, "end") == 0)
      align = StackAlign_End;
   else if (dStricmp(data, "start") == 0)
      align = StackAlign_Start;
   else
      align = StackAlign_Start;

   stack->mAlign = align;

   // Only repositions children within space they don't fill - doesn't change any child's own
   // resolved size, so arrangement alone needs redoing, not measurement.
   stack->setArrangementDirty();
   return false;
}

bool NewGuiStack::_setJustify(void* obj, const char* index, const char* data)
{
   NewGuiStack* stack = static_cast<NewGuiStack*>(obj);

   StackJustify justify = StackJustify_Start;
   if (dStricmp(data, "center") == 0)
      justify = StackJustify_Center;
   else if (dStricmp(data, "end") == 0)
      justify = StackJustify_End;
   else if (dStricmp(data, "spaceBetween") == 0)
      justify = StackJustify_SpaceBetween;
   else if (dStricmp(data, "spaceAround") == 0)
      justify = StackJustify_SpaceAround;
   else if (dStricmp(data, "start") == 0)
      justify = StackJustify_Start;
   else
      justify = StackJustify_Start;

   stack->mJustify = justify;

   // Same reasoning as _setAlign() - redistributes leftover space, doesn't change any child's
   // own resolved size.
   stack->setArrangementDirty();
   return false;
}

bool NewGuiStack::_setWrap(void* obj, const char* index, const char* data)
{
   NewGuiStack* stack = static_cast<NewGuiStack*>(obj);

   stack->mWrap = dAtob(data);

   // Changes how many lines exist and which children land on which line - this stack's own
   // cross-axis preferred size (sum of every line's thickness) can change too, not just placement.
   stack->setContentDirty();
   stack->setArrangementDirty();
   return false;
}

void NewGuiStack::initPersistFields()
{
   Parent::initPersistFields();

   GROUP_BEGIN("Stack");

   ADD_FIELD("axis", TypeString, 0)
      .onSet(_setAxis)
      .doc("Which way children stack: vertical (default, a column) or horizontal (a row).");

   ADD_FIELD("spacing", TypeS32, Offset(mSpacing, NewGuiStack))
      .onSet(_setSpacing)
      .doc("Fixed pixel gap inserted between consecutive VISIBLE children. Never added before the first or after the last visible child. Also applies between lines when wrap is true.");

   ADD_FIELD("align", TypeString, 0)
      .onSet(_setAlign)
      .doc("Cross-axis position of each child within space it doesn't fill itself: start (default), center, end. No effect on a child authoring \"100%\" on the cross axis.");

   ADD_FIELD("justify", TypeString, 0)
      .onSet(_setJustify)
      .doc("Stacking-axis distribution of children within this stack's own extent: start (default), center, end, spaceBetween, spaceAround. Only visible when the stacking axis is Pixels/Percent and larger than children need - \"auto\" has no leftover space to distribute.");

   ADD_FIELD("wrap", TypeBool, Offset(mWrap, NewGuiStack))
      .onSet(_setWrap)
      .doc("When true, a child that would overflow the stacking axis starts a new line (along the cross axis) instead of overflowing. Off by default.");

   GROUP_END("Stack");
}

// Reimplements NewGuiControl::ComputePreferredSize()'s standard union-of-children loop, but
// generalized to whichever axis is this stack's STACKING axis rather than being fixed to vertical
// - vertical (the default) reduces to exactly the base behavior (sum of children's heights, max
// of their widths); horizontal swaps that (sum of widths, max of heights). Also adds mSpacing
// between children and this control's own padding, same reasoning/fix as the base class's own
// ComputePreferredSize() - neither is part of any child's own preferred size, so both have to be
// added back explicitly or this stack silently under-reports its own preferred extent by exactly
// those amounts.
//
// This is still only an INITIAL guess, same as before - RecomputeContentExtentAndReclamp() is
// what corrects mPreferredSize/mBounds to the real, post-arrange truth (a child correcting its own
// size during arrange, e.g. a wrap="true" NewGuiLabel, can only ever be known accurately AFTER
// arrangement, not at measure time) - but an honest guess here means fewer frames need that
// correction, and gives a sane first-frame result before any arrange pass has run at all.
//
// Deliberately does NOT account for mWrap here - line membership depends on this stack's own
// RESOLVED stacking-axis extent (see splitIntoLines()), which doesn't exist yet at measure time
// (only mPreferredSize does, and only for a stacking axis authored "auto" - a wrapped stack's
// stacking axis is normally Pixels/Percent, not "auto", precisely because wrap needs a real
// extent to wrap children against). This still returns a reasonable single-line guess in that
// case; RecomputeContentExtentAndReclamp() corrects it to the true multi-line sum once real
// arrangement (and therefore real line-splitting) has happened.
Point2I NewGuiStack::ComputePreferredSize()
{
   const bool horizontal = (mAxis == StackAxis_Horizontal);

   S32 crossExtent = 0;
   S32 stackExtent = 0;
   bool sawVisibleChild = false;

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (!child || !child->isVisible())
         continue;

      const Point2I& childPreferred = child->getPreferredSize();

      if (horizontal)
      {
         stackExtent += childPreferred.x;
         crossExtent = getMax(crossExtent, childPreferred.y);
      }
      else
      {
         stackExtent += childPreferred.y;
         crossExtent = getMax(crossExtent, childPreferred.x);
      }

      if (sawVisibleChild)
         stackExtent += mSpacing;
      sawVisibleChild = true;
   }

   Point2I result = horizontal
      ? Point2I(stackExtent, crossExtent)
      : Point2I(crossExtent, stackExtent);

   // This stack's own padding, same as NewGuiControl::ComputePreferredSize()'s own fix - design-
   // space, unscaled, since MeasurePass()/ComputePreferredSize() have no scale factor available.
   const NewGuiEdgeInsets& padding = mResolvedStyle.padding;
   result.x += (S32)padding.horizontal();
   result.y += (S32)padding.vertical();

   return result;
}

void NewGuiStack::ArrangePass(const RectI& slotRect, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   // Calls Parent::ArrangePass() for the same reason ArrangePassWithFixedExtent() below does:
   // its self-resolution (mWidth/mHeight/mLeft/mTop/mRight/mBottom precedence, mPreserveAspect)
   // is real, non-trivial logic worth reusing rather than hand-copying into a fork with no
   // compiler protection against drifting from NewGuiControl's real, current behavior. As an
   // unavoidable side effect, Parent:: also places every child via the base GetChildSlot() flow -
   // wrong for a Stack (no spacing, no stacking-axis distribution) - which layoutChildren() below
   // (via RecomputeContentExtentAndReclamp()) immediately overwrites with the correct placement.
   // That first pass is real, wasted per-child work; same tradeoff NewGuiScroll accepts, and the
   // same one ArrangePassWithFixedExtent() below documents.
   Parent::ArrangePass(slotRect, parentRenderLayer, uiScaleX, uiScaleY);

   RecomputeContentExtentAndReclamp([&]()
   {
      Parent::ArrangePass(slotRect, parentRenderLayer, uiScaleX, uiScaleY);
   });
}

// Calls Parent::ArrangePassWithFixedExtent() for the same reason ArrangePass() does: its
// self-resolution (mBounds/mResolvedUIScaleX/Y/mRenderLayer/mArrangementDirty) is worth reusing
// rather than hand-copying, since a hand-copied fork has no compiler protection against silently
// drifting from NewGuiControl's real, current behavior if it ever changes - GetClientRect() and
// GetChildSlot() are both virtual, so a hand-written copy could also miss whatever a future
// override of either relies on being exercised here. Parent:: additionally places every child via
// the base GetChildSlot() flow, which is wrong for a Stack (no spacing, no stacking-axis
// distribution) - real, wasted per-child work - but layoutChildren() below (via
// RecomputeContentExtentAndReclamp()) immediately overwrites that placement with the correct one,
// same tradeoff NewGuiScroll accepts and the same one ArrangePass() below documents.
void NewGuiStack::ArrangePassWithFixedExtent(const RectI& finalBounds, S32 parentRenderLayer, F32 uiScaleX, F32 uiScaleY)
{
   Parent::ArrangePassWithFixedExtent(finalBounds, parentRenderLayer, uiScaleX, uiScaleY);

   RecomputeContentExtentAndReclamp([&]()
   {
      Parent::ArrangePassWithFixedExtent(finalBounds, parentRenderLayer, uiScaleX, uiScaleY);
   });
}

// Mirrors NewGuiScroll::RecomputeContentExtentAndReclamp()'s shape exactly: lay out children,
// then measure what they ACTUALLY resolved to on BOTH axes (not what ComputePreferredSize()
// guessed at measure time, and not just the stacking axis - a child correcting itself inside
// ArrangePassWithFixedExtent(), e.g. a wrap="true" NewGuiLabel, can change either dimension). As
// in NewGuiScroll, childBounds are already resolved relative to mBounds.point (the unpadded outer
// rect) and start from the LEADING padding inset already baked into GetClientRect()'s
// clientRect.point - so the TRAILING padding on the far side of each axis has to be added back
// explicitly here, via mResolvedStyle.padding, or the max extent silently comes up short by
// exactly that amount, same as NewGuiScroll's own trailing-padding comment explains.
//
// Only the axis (or axes) that are themselves authored "auto" get corrected - that's the only
// case where this stack's own resolved extent on that axis actually derives from children's sizes
// at all (via ComputePreferredSize() -> mPreferredSize -> resolveAxis()'s Auto branch); Pixels/
// Percent pin this stack's own extent independent of children, so a mismatch there is expected,
// not something to reconcile.
//
// @param reArrangeSelf Callback that re-establishes this control's own mBounds/mRenderLayer (the
// specific set-up each entry point normally does before laying out children), so layoutChildren()
// can be safely re-run against corrected state without duplicating that set-up here.
void NewGuiStack::RecomputeContentExtentAndReclamp(const std::function<void()>& reArrangeSelf)
{
   RectI clientRect = GetClientRect();
   layoutChildren(clientRect, mResolvedUIScaleX, mResolvedUIScaleY);

   Point2I maxExtent(0, 0);

   for (SimSet::iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (!child || !child->isVisible())
         continue;

      const RectI& childBounds = child->getBounds();

      // Relative to mBounds.point, same convention NewGuiScroll::RecomputeContentExtentAndReclamp()
      // uses (there, further offset by -mScrollOffset; NewGuiStack has no scroll offset of its own).
      S32 right = (childBounds.point.x - mBounds.point.x) + childBounds.extent.x;
      S32 bottom = (childBounds.point.y - mBounds.point.y) + childBounds.extent.y;

      maxExtent.x = getMax(maxExtent.x, right);
      maxExtent.y = getMax(maxExtent.y, bottom);
   }

   // Trailing padding on the far side of each axis - children only ever extend as far as
   // GetClientRect()'s inset allows, so the gap between the last child's far edge and this
   // control's own far edge (mBounds.extent) is exactly this control's own trailing padding,
   // same reasoning as NewGuiScroll's identical addition below.
   maxExtent.x += (S32)(mResolvedStyle.padding.right * mResolvedUIScaleX);
   maxExtent.y += (S32)(mResolvedStyle.padding.bottom * mResolvedUIScaleY);

   bool changed = false;

   if (getAuthoredWidth().isAuto() && maxExtent.x != mBounds.extent.x)
   {
      // Correct mPreferredSize DIRECTLY, right now - the same reasoning layoutChildren() already
      // documents for a wrap="true" NewGuiLabel correcting its own mBounds/mPreferredSize.y in
      // place: MeasurePass() already finished tree-wide before this ArrangePass() started, so
      // setContentDirty() alone would only be picked up NEXT frame, leaving an ancestor that
      // reads getPreferredSize() THIS frame (e.g. an outer stack whose own layoutChildren() runs
      // later in this same tree-walk) looking at the stale value regardless.
      mPreferredSize.x = maxExtent.x;
      mBounds.extent.x = maxExtent.x;
      changed = true;
   }

   if (getAuthoredHeight().isAuto() && maxExtent.y != mBounds.extent.y)
   {
      mPreferredSize.y = maxExtent.y;
      mBounds.extent.y = maxExtent.y;
      changed = true;
   }

   if (changed)
   {
      setContentDirty();
      setArrangementDirty();

      reArrangeSelf();
      layoutChildren(GetClientRect(), mResolvedUIScaleX, mResolvedUIScaleY);
   }
}

// A new line starts whenever wrap is on and adding the next child's stacking-axis length (plus
// spacing) would exceed clientRect's stacking-axis extent - EXCEPT a line always keeps at least
// one child regardless (a single child wider than the whole stack still gets its own line rather
// than producing an empty one). Each child's stacking-axis length here is resolved the same way
// layoutChildren() resolves it for real placement (Auto/Percent/Pixels via resolveAxis()), so line
// membership matches actual rendered size, not just preferred size.
Vector<NewGuiStack::Line> NewGuiStack::splitIntoLines(const RectI& clientRect, F32 uiScaleX, F32 uiScaleY, Vector<NewGuiControl*>& outVisibleChildren) const
{
   const bool horizontal = (mAxis == StackAxis_Horizontal);
   const S32 stackReferenceLength = horizontal ? clientRect.extent.x : clientRect.extent.y;
   const S32 crossReferenceLength = horizontal ? clientRect.extent.y : clientRect.extent.x;

   outVisibleChildren.clear();
   for (SimSet::const_iterator itr = begin(); itr != end(); ++itr)
   {
      NewGuiControl* child = dynamic_cast<NewGuiControl*>(*itr);
      if (child && child->isVisible())
         outVisibleChildren.push_back(child);
   }

   Vector<Line> lines;

   Line current;
   current.firstChildIndex = 0;
   current.childCount = 0;
   current.stackExtent = 0;
   current.crossExtent = 0;

   for (U32 i = 0; i < outVisibleChildren.size(); ++i)
   {
      NewGuiControl* child = outVisibleChildren[i];

      S32 childStackLength = horizontal
         ? resolveAxis(child->getAuthoredWidth(), stackReferenceLength, child->getPreferredSize().x, uiScaleX)
         : resolveAxis(child->getAuthoredHeight(), stackReferenceLength, child->getPreferredSize().y, uiScaleY);
      childStackLength = getMax(0, childStackLength);

      S32 childCrossLength = horizontal
         ? resolveAxis(child->getAuthoredHeight(), crossReferenceLength, child->getPreferredSize().y, uiScaleY)
         : resolveAxis(child->getAuthoredWidth(), crossReferenceLength, child->getPreferredSize().x, uiScaleX);
      childCrossLength = getMax(0, childCrossLength);

      S32 extentWithChild = current.stackExtent + (current.childCount > 0 ? mSpacing : 0) + childStackLength;

      bool startNewLine = mWrap && current.childCount > 0 && extentWithChild > stackReferenceLength;

      if (startNewLine)
      {
         lines.push_back(current);

         current.firstChildIndex = i;
         current.childCount = 0;
         current.stackExtent = 0;
         current.crossExtent = 0;
      }

      current.stackExtent += (current.childCount > 0 ? mSpacing : 0) + childStackLength;
      current.crossExtent = getMax(current.crossExtent, childCrossLength);
      current.childCount++;
   }

   if (current.childCount > 0)
      lines.push_back(current);

   return lines;
}

// Splits visible children into one or more Lines (splitIntoLines() - a single Line, spanning
// every visible child, unless wrap is on and they overflow), then places every child within its
// own line: mJustify distributes each line's stacking-axis leftover space (this stack's own
// stacking-axis extent, minus that line's own stackExtent) among/around its children; mAlign
// positions each child within its line's own crossExtent (that line's thickness - the max of its
// children's cross-axis lengths), not this whole stack's cross-axis extent, so align stays
// meaningful per-line even when lines have different thicknesses. Successive lines advance along
// the CROSS axis, separated by mSpacing - the same field used for spacing within a line, since a
// single "gap" concept covers both without needing a second authored field.
void NewGuiStack::layoutChildren(const RectI& clientRect, F32 uiScaleX, F32 uiScaleY)
{
   const bool horizontal = (mAxis == StackAxis_Horizontal);
   const S32 stackReferenceLength = horizontal ? clientRect.extent.x : clientRect.extent.y;

   Vector<NewGuiControl*> visibleChildren;
   Vector<Line> lines = splitIntoLines(clientRect, uiScaleX, uiScaleY, visibleChildren);

   S32 crossOffset = horizontal ? clientRect.point.y : clientRect.point.x;

   for (U32 lineIndex = 0; lineIndex < lines.size(); ++lineIndex)
   {
      const Line& line = lines[lineIndex];

      if (lineIndex > 0)
         crossOffset += mSpacing;

      // Leftover stacking-axis space THIS LINE doesn't use, for mJustify to distribute. Never
      // negative - a line whose children overflow the stack (only possible with wrap off, or a
      // single over-wide child even with wrap on) simply has zero leftover, same as any other
      // justify mode collapsing to "start" when there's nothing to distribute.
      S32 leftover = getMax(0, stackReferenceLength - line.stackExtent);

      S32 runningOffset = horizontal ? clientRect.point.x : clientRect.point.y;
      S32 betweenExtra = 0;   // Extra gap inserted BETWEEN children (spaceBetween/spaceAround), beyond mSpacing.

      switch (mJustify)
      {
      case StackJustify_Center:
         runningOffset += leftover / 2;
         break;
      case StackJustify_End:
         runningOffset += leftover;
         break;
      case StackJustify_SpaceBetween:
         if (line.childCount > 1)
            betweenExtra = leftover / (S32)(line.childCount - 1);
         else
            runningOffset += leftover / 2;   // A single child: spaceBetween has nothing to space between, so center it instead of pinning to start.
         break;
      case StackJustify_SpaceAround:
         if (line.childCount > 0)
         {
            S32 aroundShare = leftover / (S32)line.childCount;
            betweenExtra = aroundShare;
            runningOffset += aroundShare / 2;
         }
         break;
      case StackJustify_Start:
      default:
         break;   // runningOffset already starts at the leading edge.
      }

      bool placedOnLine = false;

      for (U32 i = 0; i < line.childCount; ++i)
      {
         NewGuiControl* child = visibleChildren[line.firstChildIndex + i];

         if (placedOnLine)
            runningOffset += mSpacing + betweenExtra;

         const S32 stackRef = horizontal ? clientRect.extent.x : clientRect.extent.y;
         const S32 crossRef = horizontal ? clientRect.extent.y : clientRect.extent.x;

         S32 stackingLength = horizontal
            ? resolveAxis(child->getAuthoredWidth(), stackRef, child->getPreferredSize().x, uiScaleX)
            : resolveAxis(child->getAuthoredHeight(), stackRef, child->getPreferredSize().y, uiScaleY);
         stackingLength = getMax(0, stackingLength);

         S32 crossLength = horizontal
            ? resolveAxis(child->getAuthoredHeight(), crossRef, child->getPreferredSize().y, uiScaleY)
            : resolveAxis(child->getAuthoredWidth(), crossRef, child->getPreferredSize().x, uiScaleX);
         crossLength = getMax(0, crossLength);

         // mAlign positions this child within ITS LINE's own crossExtent (thickness), not this
         // whole stack's cross-axis extent - a child filling less than the line's thickness (the
         // common case: the line's thickness is set by its TALLEST/WIDEST child) gets positioned
         // per mAlign within that leftover; a child authoring "100%" cross-fill already consumed
         // the full line thickness via crossLength above, leaving nothing to align.
         S32 crossLeftoverInLine = getMax(0, line.crossExtent - crossLength);
         S32 crossPos = crossOffset;
         switch (mAlign)
         {
         case StackAlign_Center:
            crossPos += crossLeftoverInLine / 2;
            break;
         case StackAlign_End:
            crossPos += crossLeftoverInLine;
            break;
         case StackAlign_Start:
         default:
            break;
         }

         RectI childBounds = horizontal
            ? RectI(Point2I(runningOffset, crossPos), Point2I(stackingLength, crossLength))
            : RectI(Point2I(crossPos, runningOffset), Point2I(crossLength, stackingLength));

         child->ArrangePassWithFixedExtent(childBounds, mRenderLayer, uiScaleX, uiScaleY);

         // Use the child's ACTUAL resolved extent along the stacking axis (a wrap="true" label,
         // for instance, may correct its own extent.y inside ArrangePassWithFixedExtent()), not
         // the stackingLength we offered, so the next child's offset stays accurate.
         const RectI& resolvedChildBounds = child->getBounds();
         runningOffset += horizontal ? resolvedChildBounds.extent.x : resolvedChildBounds.extent.y;

         placedOnLine = true;
      }

      crossOffset += line.crossExtent;
   }
}
